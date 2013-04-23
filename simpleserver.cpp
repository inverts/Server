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
  std::string current_spreadsheet; //The name of the current spreadsheet.
  std::string LF;

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

  bool join_spreadsheet(string name, string password) {
    cout << "Initial current spreadsheet: " << current_spreadsheet << endl;
    if (spreadsheets->find(name) != spreadsheets->end())
      return false; //This spreadsheet already exists.
    if(!spreadsheets->at(name).check_password(password))
      return false; //Wrong password.
    current_spreadsheet = name;
    cout << "Current spreadsheet changed to: " << current_spreadsheet << endl;
  }

  enum {CHANGE_SUCCESS, CHANGE_WAIT, CHANGE_FAIL};
  int change_spreadsheet(string name, int version, string cellname, string data)
  {
    if (this->name != name)
      return CHANGE_FAIL; //This client doesn't have a session with that spreadsheet.
    spreadsheet *ss = &spreadsheets->at(name);
    if (version != ss->get_version())
      return CHANGE_WAIT; //Incorrect version number.
    if(!ss->try_update_cell(cellname, data))
      return CHANGE_FAIL; //The cell couldn't be updated for some reason.
    ss->increment_version();
    return CHANGE_SUCCESS;
  }

public:
  boost::asio::ip::tcp::socket sock; 
  boost::array<char, 1024> buffer; //This is the buffer we use to read in messages.
  //std::string updatedCell;
  std::string name;
  std::string password; //DELETE
  //  int version;
  spreadsheet_map *spreadsheets; //This is a hashmap of all the available spreadsheets.

  client_connection(boost::asio::io_service& io_service, spreadsheet_map *sheets)
    : sock(io_service)
  {
    //Creates new socket.
    //Clean buffer
    int i=0;
    for (i=0; i<1024; i++)
      buffer[i] = '\0';
    spreadsheets = sheets;

    LF =  "\n";
  }

  boost::asio::ip::tcp::socket& socket()
  {
    return sock; //Have a sock.
  }

  void start() {
    //THIS NEEDS TO BE REMOVED.
    //boost::asio::async_write(sock, boost::asio::buffer("You have successfully connected to the server."), boost::bind(&client_connection::write_handler, shared_from_this(), boost::asio::placeholders::error)); 

    sock.async_read_some(boost::asio::buffer(buffer), boost::bind(&client_connection::read_handler, shared_from_this(), boost::asio::placeholders::error)); //Start listening for messages coming through.
  }

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

    } else {
      if (DEBUG_MODE) 
	cout << "The host terminated the connection or there was an error. No more data will be read." << endl;
    }
  }

  /*
   * Handles tcp::socket::async_write() calls.
   */
  void write_handler(const boost::system::error_code &ec) 
  { 
    sock.async_read_some(boost::asio::buffer(buffer), boost::bind(&client_connection::read_handler, shared_from_this(), boost::asio::placeholders::error)); //Continue reading message(s) from the socket.
  } 

  /*
   * Writes a message and calls write_handler upon completion.
   */
  void writeMessage(std::string message)
  {
    boost::asio::async_write(sock, boost::asio::buffer(message), boost::bind(&client_connection::write_handler, shared_from_this(), boost::asio::placeholders::error)); 
  }

  void sendCreate(std::string data)
  {
    cout << "Enter sendCreate." << endl;
    std::string msg;
    std::string name;
    std::string pass;  
    boost::system::error_code ec;  
   
    /* parse Name */
    std::size_t pos1 = 5 + data.find("Name:");
    std::size_t pos2 = data.find('\n', pos1);
    name = data.substr(pos1, pos2-pos1);
    cout << "Name: " << name << endl; //REMOVE THIS

    /* parse password */
    std::size_t p1 = 9 + data.find("Password:");
    std::size_t p2 = data.find('\n', p1);
    pass = data.substr(p1, p2-p1);
    cout << "Password: " << pass << endl; //REMOVE THIS

    bool create_success = create_spreadsheet(name, pass);
    if (create_success)  
      {
	msg = "CREATE SP OK " + LF + "Name:" + name + " " + LF + "Password:" + pass + " " + LF;
	writeMessage(msg);
      }
    else
      {
	msg = "CREATE SP FAIL " + LF + "Name:" + name + LF + "Request failed " + LF;
	writeMessage(msg);
      }
  }

  void sendJoin(std::string data)
  {

    std::string name;
    std::string pass;
    std::string version;
    std::string length;
    std::string msg;
  
    boost::system::error_code ec;

    /* parse Name */
    std::size_t pos1 = 5+data.find("Name:");
    std::size_t pos2 = data.find('\n', pos1);
    name = data.substr(pos1, pos2-pos1);
    cout << "Name: " << name << endl;

    /* parse password */
    std::size_t p1 = 9+data.find("Password:");
    std::size_t p2 = data.find('\n', p1);
    pass = data.substr(p1, p2-p1);
    cout << "Password: " << pass << endl;

    bool create_success = join_spreadsheet(name, pass);
    if (create_success) 
      {
	std::string xml = spreadsheets->at(name).generate_xml();
	msg = "JOIN SP OK " + LF + "Name:" + name + " " + LF + "Version:" + version + " " + LF + "Length:" + length + " " + LF + xml + LF;
	writeMessage(msg);
      }
    else
      {
	msg = "JOIN SP FAIL " + LF + "Name:" + name + " " + LF + "FAIL MESSAGE " + LF;
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

    boost::system::error_code ec;

    /* parse Name */
    std::size_t pos1 = 1+data.find("Name:");
    std::size_t pos2 = data.find('\n', pos1);
    name = data.substr(pos1, pos2-pos1);
    if(DEBUG_MODE)
      cout << "Name: " << name << endl;

    /*parse version */
    std::size_t pv = 1+data.find("Version:");
    std::size_t pv2 = data.find('\n', pv);
    version = data.substr(pv, pv2-pv);
    
    //Convert the version string to an integer.
    string str = version;
    int versNum;
    istringstream (str) >> versNum;

    if(DEBUG_MODE)
      cout << "Version: " << version << endl;

    /*parse cell */
    std::size_t pc = 1+data.find("Cell:");
    std::size_t pc2 = data.find('\n', pc);
    cellname = data.substr(pc, pc2-pc);
    if(DEBUG_MODE)
      cout << "Cell: " << cellname << endl;

    //updatedCell = cell;

    /*parse length */
    std::size_t pl = 1+data.find("Length:");
    std::size_t pl2 = data.find('\n', pl);
    length = data.substr(pl, pl2-pl);
    if(DEBUG_MODE)
      cout << "Length: " << length << endl;
   
    std::string celldata = data.substr(pl2);
    if(DEBUG_MODE)
      cout << "Cell data: " << celldata << endl;

    int chg_success(change_spreadsheet(name, versNum, cellname,  celldata));

    if (chg_success==CHANGE_SUCCESS) {
      msg = "CHANGE SP OK " + LF + "Name:" + name + " " + LF + "Version:" + version + " " + LF;
      writeMessage(msg);
    } else if (chg_success==CHANGE_WAIT) {
      msg = "CHANGE SP WAIT " + LF + "Name:" + name + " " + LF + "Version:" + version + " " + LF;
      writeMessage(msg);
    } else {
      msg = "CHANGE SP FAIL " + LF + "Name:" + name + " " + LF + "FAIL MESSAGE " + LF;
      writeMessage(msg);
    }	
      

    //NOTE: If the change is successful, we also need to send an UPDATE to all clients!
  }

  void sendUndo(std::string data)
  {

   std::string name = "";
   std::string version = "";
   std::string cell = "";
   std::string length = "";

   /* NOT COMPLETE */
   std::string content;

   std::string msg;
   boost::system::error_code ec;

   
   /* parse Name */
   std::size_t pos1 = 1+data.find("Name:");
   std::size_t pos2 = data.find('\n', pos1);
   name = data.substr(pos1, pos2-pos1);
   cout << "Name " << name << endl;

   /*parse version */
   std::size_t pv = 1+data.find("Version:");
   std::size_t pv2 = data.find('\n', pv);
   version = data.substr(pv, pv2-pv);
 
    string str = version;
    int versNum;
    istringstream (str) >> versNum;

   cout << "Version " << version << endl;

  //  /***************UPDATE COMMENTED SPREADSHEET ASPECTS***************
//    /* cell to revert */
// /* NOT COMPLETE */
//    cell = updatedCell;
//    //content = spreadsheet::get_cell_data(cell); 
   
   
//    /* if the request succeeded */
//    if (name == this->name && versNum == this->version)
//    {
//       msg = "UNDO SP OK " + LF + "Name:" + name + " " + LF +
// 	 "Version:" + version + " " + LF + "Cell:" + cell + " " +
// 	 LF + "Length:" + length + " " + LF + content + " " + LF;
//       writeMessage(msg);
//    }  
//    /* if no unsaved changes */      
//    else if (spreadsheet::get_cell_data(cell) == content && name == this->name && versNum == this->version)
//    {
//       msg = "UNDO SP END " + LF + "Name:" + name + " " + LF + "Version:" + version + " " + LF;
//       writeMessage(msg);
//    }
//    /* if version is out of date */
//    else if (this-> != versNum && name == this->name)
//    {
//       msg = "UNDO SP WAIT " + LF + "Name:" + name + " " + LF + "Version:" + version + " " + LF;
//       writeMessage(msg); 
//    }   
//    else
//    {
//       msg = "UNDO SP FAIL " + LF + "Name:" + name + " " + LF + "FAIL MESSAGE " + LF;
//       writeMessage(msg);
//    }

  }

  void sendUpdate(std::string data)
  {

   /* NOT COMPLETE - NEED PRIV MEM VARS FOR EACH */
   std::string name = "";
   int version = 0;
   std::string cell = "";
   std::string length = "";
   std::string cellContent = "";
   std::string msg;
   boost::system::error_code ec;

   name = this->name;
   version = spreadsheets->at(name).get_version();

/* NOT COMPLETE */
   //cell = updatedCell;
   //cellContent = spreadsheet::get_cell_data(cell);
   //length = strlen(cellContent);

   // msg = "UPDATE " + LF + "Name:" + name + " " + LF + "Version:" + version + " " 
   //    + LF + "Cell:" + cell + " " + LF + cellContent + " " + LF;
   // writeMessage(msg);
  }


  void sendSave(std::string data)
  {
    std::string name = "";
    boost::system::error_code ec;
    std::string msg;
   
    /* parse Name */
    std::size_t pos1 = 1+data.find("Name:");
    std::size_t pos2 = data.find('\n', pos1);
    name = data.substr(pos1, pos2-pos1);
    cout << "Name " << name << endl;

    /* if request succeeded */
    if (name == this->name)
      {
	msg = "SAVE SP OK " + LF + "Name:" + name + " " + LF;
	writeMessage(msg);
      }
    /* if the request fails */
    else
      {
	msg = "SAVE SP FAIL " + LF + "Name:" + name + " " + LF + "FAIL MESSAGE " + LF;
	writeMessage(msg);
      }
  }

  void sendError(std::string data)
  {
    writeMessage("ERROR \n");
  }

  void parse_message(std::string message)
  {  
    /* call methods to parse data */
    if ( message != "\n")
      {
	cout << "Message: " << message << endl;
	int loc = message.find("\n");
	if (loc ==  string::npos)
	  return; //No newline found?
	std::string command = message.substr(0, loc);
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
	  sendError(command);
      }
  }


};

boost::asio::io_service io_service; 
boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 1984); 
boost::asio::ip::tcp::acceptor *acceptorptr; 
spreadsheet_map spreadsheets;

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
	file = file.substr(0, dot); //Remove .txt at the end
	spreadsheet ss(file, "password");
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

void start_accept(); //Forward declaration
typedef boost::shared_ptr<client_connection> client_connection_ptr;

void accept_handler(const boost::system::error_code &ec, client_connection_ptr connection) 
{ 
  if (!ec) 
    { 
      cout << "New incoming connection received." << endl;
      connection->start();
    } 
  start_accept(); //Listen for more connections.
} 


void start_accept() {
  client_connection_ptr new_connection(new client_connection(io_service, &spreadsheets));
  acceptorptr->listen();
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
