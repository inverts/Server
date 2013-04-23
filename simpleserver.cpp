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

/*
 * This represents a single client connection.
 */
class client_connection
  : public boost::enable_shared_from_this<client_connection> 
{
public:
  boost::asio::ip::tcp::socket sock; 
  boost::array<char, 1024> buffer; //This is the buffer we use to read in messages.
  std::string updatedCell;
  std::string name;
  std::string password;
  int version;
  //spreadsheet ss(std::string buffer.data());
  //spreadsheet::spreadsheet userSp(name,password);

  client_connection(boost::asio::io_service& io_service)
    : sock(io_service)
  {
    //Creates new socket.
    //Clean buffer
    int i=0;
    for (i=0; i<1024; i++)
      buffer[i] = '\0';
  }

  boost::asio::ip::tcp::socket& socket()
  {
    return sock; //Have a sock.
  }

  void start() {
    boost::asio::async_write(sock, boost::asio::buffer("You have successfully connected to the server."), boost::bind(&client_connection::write_handler, shared_from_this(), boost::asio::placeholders::error)); 
  }

  void read_handler(const boost::system::error_code &ec/*, std::size_t bytes_transferred*/) 
  { 
    if (!ec) {
      cout << "The server read an incoming message:" << endl; 
      cout << "---------------- Message Received ----------------" << endl;
      cout << string(buffer.data()) << endl; 
      cout << "------------------ End Message -------------------" << endl;
      readData();
      int msg_length = strlen(buffer.data());
      int i;
      //Clean the buffer.
      for (i=0; i<msg_length; i++)
	buffer[i] = '\0';
      cout << "Server is sending an acknowledgement response." << endl;
      boost::asio::async_write(sock, boost::asio::buffer("Welcome"), boost::bind(&client_connection::write_handler, shared_from_this(), boost::asio::placeholders::error)); 
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

    std::string msg;
    std::string name = "";
    std::string pass = "";  
    boost::system::error_code ec;  
   
    /* parse Name */
    std::size_t pos1 = 1+data.find("Name:");
    std::size_t pos2 = data.find(' ', pos1);
    name = data.substr(pos1, pos2-pos1);
    cout << "Name " << name << endl;

    /* parse password */
    std::size_t p1 = 1+data.find("Password:");
    std::size_t p2 = data.find(' ', p1);
    pass = data.substr(p1, p2-p1);
    cout << "Password " << pass << endl;
   
    std::string LF = "\n";
    if (name == this->name && pass == this->password)  
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
    std::size_t pos1 = 1+data.find("Name:");
    std::size_t pos2 = data.find(' ', pos1);
    name = data.substr(pos1, pos2-pos1);
    cout << "Name " << name << endl;

    /* parse password */
    std::size_t p1 = 1+data.find("Password:");
    std::size_t p2 = data.find(' ', p1);
    pass = data.substr(p1, p2-p1);
    cout << "Password " << pass << endl;


    /* if the request succeeded */
    if (name == this->name && pass == this->password) 
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

  void readData()
{  
  std::string message = buffer.data();
  std::string lineData;

  /*read a single line from a socket and into a string */ 
  cout << "Line data from socket " << lineData << endl;

  int buffSize = sizeof(buffer);

  int msg_length = strlen(buffer.data());
  int i;
  //Clean the buffer.
  for (i=0; i<msg_length; i++)
    buffer[i] = '\0';

  /* asynchronously read data into a streambuf until a LF 
     buffer b contains the data
     getline extracts the data into lineData
  */
  boost::asio::streambuf b;
  std::istream is(&b);
  std::getline(is, lineData);
  //boost::asio::async_read_until(sock, b, '\n', read_handler);

  sock.async_read_some(boost::asio::buffer(buffer), boost::bind(&client_connection::read_handler, shared_from_this(), boost::asio::placeholders::error));

  cout << "Line data " << lineData << endl;
  
  /* call methods to parse data */
  if (lineData != "\n" || message != "\n")
    {
     for(int i = 0; i < buffSize; i++)
	{
	   message += lineData[i];
	}
     
     cout << "Current line msg " << message << endl;
     
      if(message == "CREATE")
	{
	  sendCreate(lineData);
	}
      else if (message == "JOIN")
      {
	 sendJoin(lineData);
      }
      else if(message == "CHANGE")
      {
	 sendChange(lineData);
      }
      else if(message == "UNDO")
      {
	 sendUndo(lineData);
      }
      else if(message == "UPDATE")
      {
	 sendUpdate(lineData);
      }
      else if(message == "SAVE")
      {
	 sendSave(lineData);
      }
      else if (message == "LEAVE")
      {
	 /* do not respond */
      }
      else
      {
	 sendError(lineData);
      }

      // boost::asio::async_read_some(sock, b, '\n', read_handler);
	 sock.async_read_some(boost::asio::buffer(buffer), boost::bind(&client_connection::read_handler, shared_from_this(), boost::asio::placeholders::error));
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

typedef boost::unordered_map<std::string, spreadsheet> spreadsheet_map;
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
	spreadsheet ss(file);
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
  client_connection_ptr new_connection(new client_connection(io_service));
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
