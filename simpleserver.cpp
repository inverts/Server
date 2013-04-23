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

  //Creates a new spreadsheet
  bool create_spreadsheet(string name, string password)
  {
    if (spreadsheets->find(name) != spreadsheets->end())
      return false; //This spreadsheet already exists.
    spreadsheet ss(name,password);
    spreadsheets->insert(std::make_pair(name,ss));
    ss.save_spreadsheet();
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

public:
  boost::asio::ip::tcp::socket sock; 
  boost::array<char, 1024> buffer; //This is the buffer we use to read in messages.
  std::string updatedCell;
  std::string name;
  std::string password; //DELETE
  int version;
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
  }

  boost::asio::ip::tcp::socket& socket()
  {
    return sock; //Have a sock.
  }

  void start() {
    //THIS NEEDS TO BE REMOVED.
    boost::asio::async_write(sock, boost::asio::buffer("You have successfully connected to the server."), boost::bind(&client_connection::write_handler, shared_from_this(), boost::asio::placeholders::error)); 
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
    string message(buffer.data());
    if (!ec) {

      //REMOVE (possibly)
      cout << "The server read an incoming message:" << endl; 
      cout << "---------------- Message Received ----------------" << endl;
      cout << message << endl; 
      cout << "------------------ End Message -------------------" << endl;

      parse_message(message);

      clean_buffer();

    } else {
      cout << "The host terminated the connection or there was an error. No more data will be read." << endl;
    }
  }

  void writeMessage(std::string message)
  {
    boost::asio::async_write(sock, boost::asio::buffer(message), boost::bind(&client_connection::write_handler, shared_from_this(), boost::asio::placeholders::error)); 
  }

  void sendCreate(std::string data)
  {
    cout << "Enter sendCreate." << endl;
    std::string msg;
    std::string name = "";
    std::string pass = "";  
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
    std::string LF = "\n";
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

    std::string name = "";
    std::string pass = "";
    std::string version = "TEMP VERSION";
    std::string length = "TEMP LENGTH";
    std::string xml = "TEMP";
    std::string msg;
    std::string LF = "\n";
  
    boost::system::error_code ec;

    /* parse Name */
    std::size_t pos1 = 5+data.find("Name:");
    std::size_t pos2 = data.find('\n', pos1);
    name = data.substr(pos1, pos2-pos1);
    cout << "Name " << name << endl;

    /* parse password */
    std::size_t p1 = 9+data.find("Password:");
    std::size_t p2 = data.find('\n', p1);
    pass = data.substr(p1, p2-p1);
    cout << "Password " << pass << endl;


    bool create_success = join_spreadsheet(name, pass);
    if (create_success) 
      {
	msg = "JOIN SP OK " + LF + "Name:" + name + " " + LF + "Version:"
	  + version + " " + LF + "Length:" + length + " " + LF + xml + LF;
      
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

    std::string name = ""; 
    std::string version = "";
    std::string cell = "";
    std::string length = "";
    std::string LF = "\n";
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

    /*parse cell */
    std::size_t pc = 1+data.find("Cell:");
    std::size_t pc2 = data.find('\n', pc);
    cell = data.substr(pc, pc2-pc);
    cout << "Cell " << cell << endl;

    updatedCell = cell;

    /*parse length */
    std::size_t pl = 1+data.find("Length:");
    std::size_t pl2 = data.find('\n', pl);
    length = data.substr(pl, pl2-pl);
    cout << "Length " << length << endl;
   
    /* If request succeeded */
    if (name == this->name && versNum == this->version && cell == updatedCell)
    {      
       msg = "CHANGE SP OK " + LF + "Name:" + name + " " + LF + "Version:" + version + " " + LF;
       writeMessage(msg);	
    }
    /* if the client's version is out of date */
    else if (versNum != this->version && name == this->name)
    {
       msg = "CHANGE SP WAIT " + LF + "Name:" + name + " " + LF + "Version:" + version + " " + LF;
       writeMessage(msg);
    }
    /* if there are any other errors */
    else
    {
       msg = "CHANGE SP FAIL " + LF + "Name:" + name + " " + LF + "FAIL MESSAGE " + LF;
       writeMessage(msg);
    }

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
   std::string LF = "\n";
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
   std::string LF = "\n";
   std::string msg;
   boost::system::error_code ec;

   name = this->name;
   version = this->version;
/* NOT COMPLETE */
   cell = updatedCell;
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
    std::string LF = "\n";
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
    std::string toSend = "ERROR \n";
    writeMessage(toSend);
  }

  void parse_message(std::string message)
  {  
    /*
    std::string lineData;

    boost::asio::streambuf b;
    std::istream is(&b);
    std::getline(is, lineData);
    */

    /*read a single line from a socket and into a string */ 

    int buffSize = strlen(message.c_str());

    /* asynchronously read data into a streambuf until a LF 
       buffer b contains the data
       getline extracts the data into lineData
    */

    /* call methods to parse data */
    if ( message != "\n")
      {/*
	for(int i = 0; i < buffSize; i++)
	  message += lineData[i];
       */
	  
     
	cout << "Message: " << message << endl;
	//cout << lineData << endl;

	int loc = message.find("\n");
	if (loc ==  string::npos)
	  return; //No SP found?
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
	else if(command == "UPDATE")
	    sendUpdate(message);
	else if(command == "SAVE")
	    sendSave(message);
	else if (command == "LEAVE")
	  {/* do not respond */}
	else
	    sendError(command);

	/*
	if(message == "CREATE")
	  sendCreate(lineData);
	else if (message == "JOIN")
	  sendJoin(lineData);
	else if(message == "CHANGE")
	  sendChange(lineData);
	else if(message == "UNDO")
	    sendUndo(lineData);
	else if(message == "UPDATE")
	    sendUpdate(lineData);
	else if(message == "SAVE")
	    sendSave(lineData);
	else if (message == "LEAVE")
	  {}
	else
	    sendError(lineData);
*/

	//sock.async_read_some(boost::asio::buffer(buffer), boost::bind(&client_connection::read_handler, shared_from_this(), boost::asio::placeholders::error));
      }
  }

  void write_handler(const boost::system::error_code &ec/*, std::size_t bytes_transferred*/) 
  { 
    sock.async_read_some(boost::asio::buffer(buffer), boost::bind(&client_connection::read_handler, shared_from_this(), boost::asio::placeholders::error)); //Continue reading message(s) from the socket.
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
    while ((ent = readdir(dir)) != NULL) {
      string file = ent->d_name;
      if (file!="." && file != "..") {
	cout << file << endl; //REMOVE BEFORE DEPLOY
	int dot = file.find(".");
	file = file.substr(0, dot); //Remove .txt at the end
	spreadsheet ss(file, "password");
	spreadsheets.insert(std::make_pair(file, ss));
	spreadsheet s1 = spreadsheets.at(file);
	string sheetname = s1.get_name();
	cout << sheetname << " - echo" <<endl; //REMOVE BEFORE DEPLOY
      }
    }
    closedir (dir);
  } else {
    //Directory fail
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
  if (argc != 2) {
    cout << "You must specify a port you would like this server to run on." << endl;
    return 1;
  }
  load_spreadsheets(); //This may be moved..
  int port = atoi(argv[1]); //Load in the port number.
  endpoint.port(port); //Edit the endpoint to the correct port
  boost::asio::ip::tcp::acceptor acceptor(io_service, endpoint); //Create acceptor with endpoint
  acceptorptr = &acceptor; //Save pointer to acceptor.
  start_accept(); //Begin listening.
  io_service.run(); 
}
