/*
 * This is a multi-client spreadsheet server written according to the Cloud Spreadsheet specification defined at:
 *     https://docs.google.com/file/d/0B62qeGOpcKp4aEZLaXRneUs2QzQ/edit
 *
 * Design Notes:
 * Optimally, the free-floating methods defined in this file should be a part of a server object. This would allow us
 * to pass a parent server object to each new client connection, and would allow us to place the server communications
 * outside of the client_connection object, where they likely don't belong (especially things like send_message_all).
 * Instead, each client_connection object holds references to the list of available spreadsheets as well as a list of
 * all connected peers.
 *
 * Authors: Dave Wong, Lynn Gao, Natalie McMullen, Aaron Zettler.
 * Course: CS 3505 - Spring 2013 - Peter Jensen
 * University of Utah
 */
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp> 
#include <boost/array.hpp>
#include <boost/unordered_map.hpp>
#include <dirent.h>  
#include "spreadsheet.hpp"
#include <iostream>  
#include <string> 
#include <cstdlib>
#include <sstream>
#include <boost/lexical_cast.hpp>

//Set to 1 to display debugging output.
#define DEBUG_MODE 1

using namespace std;

typedef boost::unordered_map<std::string, spreadsheet> spreadsheet_map;

/*
 * This represents a single client connection.
 */
class client_connection
  : public boost::enable_shared_from_this<client_connection> 
{
private:
typedef boost::shared_ptr<client_connection> client_connection_ptr;
  std::string current_spreadsheet; //The name of the current spreadsheet.
  std::string LF;
  boost::asio::ip::tcp::socket sock; 
  boost::array<char, 1024> buffer; //This is the buffer we use to read in messages.
  spreadsheet_map *spreadsheets; //This is a hashmap of all the available spreadsheets.
  std::list<client_connection_ptr> *peers;
  client_connection_ptr self;

  //Creates a new spreadsheet
  bool create_spreadsheet(string name, string password)
  {
    if (spreadsheets->find(name) != spreadsheets->end())
      return false; //This spreadsheet already exists.
    spreadsheet ss(name,password);
    spreadsheets->insert(std::make_pair(name,ss));
    ss.save_spreadsheet(); //We need to save it so it persists after a crash, since it doesn't exist yet.
    return true;
  }

  bool join_spreadsheet(string name, string password, string & fail_message) {
    if(DEBUG_MODE)
      cout << "Initial current spreadsheet: " << current_spreadsheet << endl;
    if (spreadsheets->find(name) == spreadsheets->end()) {
      fail_message = "That spreadsheet does not exist.";
      return false;
    }
    if(!spreadsheets->at(name).check_password(password)) {
      fail_message = "Incorrect password for spreadsheet.";
      return false; 
    }
    current_spreadsheet = name;
    if(spreadsheets->at(name).get_active_clients() < 1)
      spreadsheets->at(name).reset_version();
    spreadsheets->at(name).add_active_client();
    if(DEBUG_MODE)
      cout << "Current spreadsheet changed to: " << current_spreadsheet << endl;
    return true;
  }

  enum {CHANGE_SUCCESS, CHANGE_WAIT, CHANGE_FAIL};
  int change_spreadsheet(string name, int version, string cellname, string data)
  {
    if (this->current_spreadsheet != name)
      return CHANGE_FAIL; //This client doesn't have a session with that spreadsheet.
    if (version != spreadsheets->at(name).get_version())
      return CHANGE_WAIT; //Incorrect version number.
    if(!spreadsheets->at(name).try_update_cell(cellname, data))
      return CHANGE_FAIL; //The cell couldn't be updated for some reason.
    spreadsheets->at(name).increment_version();
    return CHANGE_SUCCESS;
  }
  bool save_spreadsheet(string name, string & fail_message)
  {
    if (name != this->current_spreadsheet) {
      fail_message = "You are not currently editing that spreadsheet.";
      return false;
    }
    if (spreadsheets->find(name) == spreadsheets->end()) {
      fail_message = "Spreadsheet does not exist.";
      return false;
    }
    spreadsheets->at(name).save_spreadsheet();
    return true;
  }

  void leave_spreadsheet(string name){
    //What do we do if spreadsheet doesn't exist?
    if (spreadsheets->find(name) != spreadsheets->end()) {
      spreadsheets->at(name).remove_active_client();
      peers->remove(this->self);
    }
  }


public:

  client_connection(boost::asio::io_service& io_service, spreadsheet_map *sheets, std::list<client_connection_ptr> *connections)
    : sock(io_service)
  {
    int i=0;
    for (i=0; i<1024; i++) //Completely clean buffer.
      buffer[i] = '\0';
    //Save the pointers to all spreadsheets and all server clients.
    spreadsheets = sheets;
    peers = connections;
    client_connection_ptr *selftemp = new client_connection_ptr(this);
    self = *selftemp;
    LF =  "\n";
  }

  //Returns the number of peers connected to the server.
  int num_peers() {
    return peers->size();
  }

  boost::asio::ip::tcp::socket& socket()
  {
    return sock; //Have a sock.
  }

  //Starts this client connection.
  void start() {
    sock.async_read_some(boost::asio::buffer(buffer), boost::bind(&client_connection::read_handler, shared_from_this(), boost::asio::placeholders::error)); //Start listening for messages coming through.
  }

  //Cleans the buffer only up to the amount of discernable data found within it.
  void clean_buffer() {
    int length = strlen(buffer.data());
    int i;
    for (i=0; i<length; i++)
      buffer[i] = '\0'; 
  }
  /*
   * Handles async_read calls.
   */
  void read_handler(const boost::system::error_code &ec) 
  { 
    if (!ec) {
      string message(buffer.data());
      if (DEBUG_MODE) {
	cout << "The server read an incoming message:" << endl; 
	cout << "---------- Message Received ---------" << endl << message << endl << "------------ End Message ------------" << endl;
      }
      parse_message(message);
      clean_buffer();
      sock.async_read_some(boost::asio::buffer(buffer), boost::bind(&client_connection::read_handler, shared_from_this(), boost::asio::placeholders::error)); //Start listening for messages coming through.

    } else {
      std::string fail;

      if (DEBUG_MODE) 
	cout << "Client terminating - Saving spreadsheet" << this->current_spreadsheet << "." << endl;
      save_spreadsheet(this->current_spreadsheet, fail);
      if (DEBUG_MODE) 
	cout << "Client terminating - Removing client from editing session." << endl;
      if(spreadsheets->find(this->current_spreadsheet) != spreadsheets->end())
	spreadsheets->at(this->current_spreadsheet).remove_active_client();
      peers->remove((this->self));
      if (DEBUG_MODE) 
	cout << "The host terminated the connection or there was an error. No more data will be read." << endl;
    }
  }

  /*
   * Handles tcp::socket::async_write() calls.
   */
  void write_handler(const boost::system::error_code &ec) 
  { 
    //sock.async_read_some(boost::asio::buffer(buffer), boost::bind(&client_connection::read_handler, shared_from_this(), boost::asio::placeholders::error)); //Continue reading message(s) from the socket.
  } 

  void write_message_all(std::string message)
  {
    std::list<client_connection_ptr>::iterator it;
    for (it=peers->begin(); it != peers->end(); ++it)
      (*it)->writeMessage(message);
  }

  /*
   * Writes a message and calls write_handler upon completion.
   */
  void writeMessage(std::string message)
  {
    if (DEBUG_MODE)
      cout << "Message being sent to client: " << endl << message << endl << "-----------------" << endl;
    boost::asio::async_write(sock, boost::asio::buffer(message), boost::bind(&client_connection::write_handler, shared_from_this(), boost::asio::placeholders::error)); 
  }

  void sendCreate(std::string data)
  {
    std::string msg;
    std::string name;
    std::string pass;  

    /* parse Name */
    std::size_t pos1 = 5 + data.find("Name:");
    std::size_t pos2 = data.find('\n', pos1);
    name = data.substr(pos1, pos2-pos1);
    if (DEBUG_MODE)
      cout << "Name: " << name << endl;

    /* parse password */
    std::size_t p1 = 9 + data.find("Password:");
    std::size_t p2 = data.find('\n', p1);
    pass = data.substr(p1, p2-p1);
    if (DEBUG_MODE)
      cout << "Password: " << pass << endl; 

    bool create_success = create_spreadsheet(name, pass);
    if (create_success)  
      {
	msg.append("CREATE SP OK");
	msg.push_back('\0');
	msg.append("Name:");
	msg.append(name);
	msg.push_back('\0');
	msg.append("Password:");
	msg.append(pass);
	msg.push_back('\0');
	writeMessage(msg);
      }
    else
      {
	msg.append("CREATE SP FAIL");
	msg.push_back('\0');
	msg.append("Name:");
	msg.append(name);
	msg.push_back('\0');
	msg.append("Sorry, that spreadsheet already exists.");
	msg.push_back('\0');
	writeMessage(msg);
      }
  }

  void sendJoin(std::string data)
  {

    std::string name;
    std::string pass;

    /* parse name */
    std::size_t pos1 = 5+data.find("Name:");
    std::size_t pos2 = data.find('\n', pos1);
    name = data.substr(pos1, pos2-pos1);
    if (DEBUG_MODE)
      cout << "Name: " << name << endl;

    /* parse password */
    std::size_t p1 = 9+data.find("Password:");
    std::size_t p2 = data.find('\n', p1);
    pass = data.substr(p1, p2-p1);
    if (DEBUG_MODE)
      cout << "Password: " << pass << endl;

    std::string fail_message;
    bool create_success = join_spreadsheet(name, pass, fail_message);
    std::string msg;
    if (create_success) 
      {
	cout << "Enter create success." << endl;
	std::stringstream ss;
	if(DEBUG_MODE)
	  cout << "Num sheets: " << spreadsheets->size() << endl;
	ss << spreadsheets->at(name).get_version();
	std::string version = ss.str();
	ss.flush();

	std::string xml = spreadsheets->at(name).generate_xml_to_spec();

	ss << strlen(xml.c_str());
	std::string length = ss.str();

	msg.append("JOIN SP OK");
	msg.push_back('\0');
	msg.append("Name:");
	msg.append(name);
	msg.push_back('\0');
	msg.append("Version:");
	msg.append(version);
	msg.push_back('\0');
	msg.append("Length:");
	msg.append(length);
	msg.push_back('\0');
	msg.append("xml:");
	msg.append(xml);
	msg.push_back('\0');

	writeMessage(msg);
      }
    else
      {
	msg.append("JOIN SP FAIL");
	msg.push_back('\0');
	msg.append("Name:");
	msg.append(name);
	msg.push_back('\0');
	msg.append(fail_message);
	msg.push_back('\0');
	writeMessage(msg);
      }  

  }

  void sendChange(std::string data)
  {
    std::string name; 
    std::string version;
    std::string cellname;
    std::string length;
    std::string msg;

    /* parse Name */
    std::size_t pos1 = 5+data.find("Name:");
    std::size_t pos2 = data.find('\n', pos1);
    name = data.substr(pos1, pos2-pos1);
    if(DEBUG_MODE)
      cout << "Name: " << name << endl;

    /*parse version */
    std::size_t pv = 8+data.find("Version:");
    std::size_t pv2 = data.find('\n', pv);
    version = data.substr(pv, pv2-pv);

    /*First check if version is a non-number, send error message if true */
    //check for if version is a non-number
    std::stringstream stream;
    stream << version;
    int n;
    if(!(stream >> n))
      {
	msg.append("CHANGE SP FAIL");
	msg.push_back('\0');
	msg.append("Name:");
	msg.append(name);
	msg.push_back('\0');
	msg.append("FAIL MESSAGE");
	msg.push_back('\0');       
	writeMessage(msg);     
      }
    //another check for non-number version input  
    int checker;
    try
      {
	checker = boost::lexical_cast<int>(version);
      }
    catch(const boost::bad_lexical_cast &)
    {
       msg.append("CHANGE SP FAIL");
       msg.push_back('\0');
       msg.append("Name:");
       msg.append(name);
       msg.push_back('\0');
       msg.append("FAIL MESSAGE");
       msg.push_back('\0');       
       writeMessage(msg);
    }
 
    
    //Convert the version string to an integer.
    string str = version;
    int versNum;
    istringstream (str) >> versNum;
      

    if(DEBUG_MODE)
      cout << "Version: " << version << endl;

    /*parse cell */
    std::size_t pc = 5+data.find("Cell:");
    std::size_t pc2 = data.find('\n', pc);
    cellname = data.substr(pc, pc2-pc);
    if(DEBUG_MODE)
      cout << "Cell: " << cellname << endl;

    /*parse length */
    std::size_t pl = 7+data.find("Length:");
    std::size_t pl2 = data.find('\n', pl);
    length = data.substr(pl, pl2-pl);
    if(DEBUG_MODE)
      cout << "Length: " << length << endl;
   
    std::string celldata = data.substr(pl2);
    if(DEBUG_MODE)
      cout << "Cell data: " << celldata << endl;

    int chg_success(change_spreadsheet(name, versNum, cellname,  celldata));

    if (chg_success==CHANGE_SUCCESS) {
       //msg = "CHANGE SP OK " + LF + "Name:" + name + " " + LF + "Version:" + version + " " + LF;       
       msg.append("CHANGE SP OK");
       msg.push_back('\0');
       msg.append("Name:");
       msg.append(name);
       msg.push_back('\0');
       msg.append("Version:");
       msg.append(version);
       msg.push_back('\0');
       writeMessage(msg);
       writeMessage(msg);
    } else if (chg_success==CHANGE_WAIT) {
       // msg = "CHANGE SP WAIT " + LF + "Name:" + name + " " + LF + "Version:" + version + " " + LF;

       msg.append("CHANGE SP WAIT");
       msg.push_back('\0');
       msg.append("Name:");
       msg.append(name);
       msg.push_back('\0');
       msg.append("Version:");
       msg.append(version);     
       msg.push_back('\0');
       writeMessage(msg);
    } else {
       //msg = "CHANGE SP FAIL " + LF + "Name:" + name + " " + LF + "FAIL MESSAGE " + LF;
       msg.append("CHANGE SP FAIL");
       msg.push_back('\0');
       msg.append("Name:");
       msg.append(name);
       msg.push_back('\0');
       msg.append("You do not have a session with that spreadsheet.");
       msg.push_back('\0');
       writeMessage(msg);
    }	
    //Send update to all clients
    send_update(name, versNum, cellname, celldata);
  }

  /*
   * Parses an undo message and makes the appropriate changes. Then sends a
   * response to the client.
   */
  void sendUndo(std::string data)
  {
    
   std::string name;
   std::string version;
   std::string cell;
   std::string length;  
   std::string content;
   std::string msg;
   boost::system::error_code ec;
   
   /* parse Name */
   std::size_t pos1 = 5+data.find("Name:");
   std::size_t pos2 = data.find('\n', pos1);
   name = data.substr(pos1, pos2-pos1);
   if(DEBUG_MODE)
     cout << "Name " << name << endl;

   /*parse version */
   std::size_t pv = 8+data.find("Version:");
   std::size_t pv2 = data.find('\n', pv);
   version = data.substr(pv, pv2-pv);


       /*First check if version is a non-number, send error message if true */
    //check for if version is a non-number
    std::stringstream stream;
    stream << version;
    int n;
    if(!(stream >> n))
    {      
       msg.append("CHANGE SP FAIL");
       msg.push_back('\0');
       msg.append("Name:");
       msg.append(name);
       msg.push_back('\0');
       msg.append("FAIL MESSAGE");
       msg.push_back('\0');       
       writeMessage(msg);
    }
    //another check for non-number version input  
    int checker;
    try
    {
       checker = boost::lexical_cast<int>(version);
    }
    catch(const boost::bad_lexical_cast &)
    {
       msg.append("CHANGE SP FAIL");
       msg.push_back('\0');
       msg.append("Name:");
       msg.append(name);
       msg.push_back('\0');
       msg.append("FAIL MESSAGE");
       msg.push_back('\0');           
       writeMessage(msg);
    }
 
    // Convert string to int
   string str = version;
   int versNum;
   istringstream (str) >> versNum;

   if(DEBUG_MODE)
     cout << "Version " << version << endl;
  
   //get cell name
   spreadsheet *ssCell = &spreadsheets->at(name);
   pair<string, string> p = ssCell->undo();
   cell = p.first;
   content = ssCell->get_cell_data(cell); 
   //get length of contents
   length = strlen(content.c_str());

   ///***************************************************************************************************************
   //if the request succeeded
   if (this->current_spreadsheet == name && ssCell->get_version() == versNum)
     {	
       if(ssCell->try_update_cell(cell, content));
       {	
	 msg.append("UNDO SP OK");
	 msg.push_back('\0');
	 msg.append("Name:");
	 msg.append(name);
	 msg.push_back('\0');
	 msg.append("Version:");
	 msg.append(version);  
	 msg.append("Cell:");
	 msg.append(cell);
	 msg.push_back('\0');
	 msg.append("Length:");
	 msg.append(length);
	 msg.push_back('\0');
	 msg.append(content);
	 msg.push_back('\0');
	 writeMessage(msg);
       }
     }
 
   /* if no unsaved changes */ 
   else if ((p.first == "") && (p.second == ""))
     {
       msg.append("UNDO SP END");
       msg.push_back('\0');
       msg.append("Name:");
       msg.append(name);
       msg.push_back('\0');
       msg.append("Version:");
       msg.append(version);
       msg.push_back('\0');
       writeMessage(msg);
     }

   /* if version is out of date */
   else if (versNum != ssCell->get_version())
     {
       msg.append("UNDO SP WAIT");
       msg.push_back('\0');
       msg.append("Name:");
       msg.append(name);
       msg.push_back('\0');
       msg.append("Version:");
       msg.append(version);        
       msg.push_back('\0');
       writeMessage(msg);
     }
  
   else
     {
       msg.append("UNDO SP FAIL");
       msg.push_back('\0');
       msg.append("Name:");
       msg.append(name);
       msg.push_back('\0');
       msg.append("FAIL MESSAGE");
       msg.push_back('\0');
       writeMessage(msg);
     }   
  }

  void send_update(std::string spreadsheet_name, int version, std::string cellname, std::string celldata)
  {
    std::stringstream ss;
    ss << spreadsheets->at(spreadsheet_name).get_version();
    std::string version_string = ss.str();
    ss.flush();
    ss << strlen(celldata.c_str());
    std::string length_string = ss.str();
    /*
      std::string msg = "UPDATE " + LF + 
      "Name:" + this->current_spreadsheet + " " + LF + 
      "Version:" + version_string + " " + LF + 
      "Cell:" + cellname + " " + LF + 
      "Length:" + length_string + " " + LF + 
      celldata + " " + LF;*/
    std::string msg;
    msg.append("UPDATE");
    msg.push_back('\0');
    msg.append("Name:");
    msg.append(this->current_spreadsheet);
    msg.push_back('\0');
    msg.append("Version:");
    msg.append(version_string);
    msg.push_back('\0');
    msg.append("Cell:");
    msg.append(cellname);
    msg.push_back('\0');
    msg.append("Length:");
    msg.append(length_string);
    msg.push_back('\0');
    msg.append(celldata);
    msg.push_back('\0');
    write_message_all(msg);
  }


  void sendSave(std::string data)
  {
    std::string name = "";
    boost::system::error_code ec;
    std::string msg;
   
    /* parse Name */
    std::size_t pos1 = 5+data.find("Name:");
    std::size_t pos2 = data.find('\n', pos1);
    name = data.substr(pos1, pos2-pos1);
    if (DEBUG_MODE)
      cout << "Name " << name << endl;

    std::string fail_message = "";
    /* if request succeeded */
    if (save_spreadsheet(name, fail_message))
      {
	msg.append("SAVE SP OK");
	msg.push_back('\0');
	msg.append("Name:");
	msg.append(name);
	msg.push_back('\0');    
	writeMessage(msg);
      }
    /* if the request fails */
    else
      {
	msg.append("SAVE SP FAIL");
	msg.push_back('\0');
	msg.append("Name:");
	msg.append(name);
	msg.push_back('\0');       
	msg.append("Request failed ");
	msg.push_back('\0');
	writeMessage(msg);
      }
  }

  void sendError()
  {
      std::string msg;
      msg.append("ERROR");
      msg.push_back('\0');
      writeMessage(msg);
  }

  void parse_message(std::string message)
  {  
    /* call methods to parse data */
    if ( message != "\n")
      {
	if (DEBUG_MODE)
	  cout << "Message: " << message << endl;
	int loc = message.find("\n");
	if (loc ==  string::npos)
	  return; //No newline found?
	std::string command = message.substr(0, loc);
	if (DEBUG_MODE)
	  cout << "Command: " << command << endl;

	if(command == "CREATE")
	  sendCreate(message);
	else if (command == "JOIN")
	  sendJoin(message);
	else if(command == "CHANGE")
	  sendChange(message);
	else if(command == "UNDO")
	  sendUndo(message);
	else if(command == "SAVE")
	  sendSave(message);
	else if (command == "LEAVE")
	  {/* No response. */}
	else
	  sendError();
      }
  }


};



boost::asio::io_service io_service; 
boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 1984); 
boost::asio::ip::tcp::acceptor *acceptorptr; 
spreadsheet_map spreadsheets;

typedef boost::shared_ptr<client_connection> client_connection_ptr;
std::list<client_connection_ptr> active_connections;

/*
 * Loads in all spreadsheets in the "spreadsheets" directory.
 */
void load_spreadsheets() {
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir ("spreadsheets")) != NULL) {
    if (DEBUG_MODE)
      cout << "Spreadsheets loaded/available:" << endl;
    while ((ent = readdir(dir)) != NULL) {
      string file = ent->d_name;
      if (file!="." && file != "..") {
	int dot = file.find(".");
	file = file.substr(0, dot); //Remove .* at the end
	spreadsheet ss(file, "");
	spreadsheets.insert(std::make_pair(file, ss));
	spreadsheet s1 = spreadsheets.at(file);
	string sheetname = s1.get_name();
	if (DEBUG_MODE)
	  cout << sheetname << endl; 
      }
    }
    closedir (dir);
  } else {
    cerr << "Spreadsheet file directory does not exist! Spreadsheets could not be loaded." << endl;
  }
}

//Forward declaration for accept_handler.
void start_accept(); 

/*
 * Handles acceptor::async_accept calls.
 */
void accept_handler(const boost::system::error_code &ec, client_connection_ptr connection) 
{ 
  if (!ec) 
    { 
      if (DEBUG_MODE)
	cout << "New incoming connection received." << endl;
      if (DEBUG_MODE)
	cout << "Peers before: " << connection->num_peers() << endl;
      active_connections.push_back(connection);
      connection->start();
      if (DEBUG_MODE)
	cout << "Peers after: " << connection->num_peers() << endl;
    } 
  start_accept(); //Listen for more connections.
} 

/*
 * Begins listening for new client connections.
 */
void start_accept() {
  client_connection_ptr new_connection(new client_connection(io_service, &spreadsheets, &active_connections));
  acceptorptr->listen();
  if (DEBUG_MODE)
    cout << "This server is now listening for connections." << endl; 
  acceptorptr->async_accept(new_connection->socket(), boost::bind(accept_handler, boost::asio::placeholders::error, new_connection));
}

int main(int argc, char* argv[]) 
{ 

  //If a port number is specified, use that instead.
  if (argc == 2) {
    int port = atoi(argv[1]); //Load in the port number.
    endpoint.port(port); //Edit the endpoint to the correct port
  }

  load_spreadsheets(); 
  boost::asio::ip::tcp::acceptor acceptor(io_service, endpoint); //Create acceptor with endpoint
  acceptorptr = &acceptor; //Save pointer to acceptor.
  start_accept(); //Begin listening.
  io_service.run(); 
}
