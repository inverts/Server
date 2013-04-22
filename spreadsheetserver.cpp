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
      int msg_length = strlen(buffer.data());
      int i;
      //Clean the buffer.
      for (i=0; i<msg_length; i++)
	buffer[i] = '\0';
      cout << "Server is sending an acknowledgement response." << endl;
      boost::asio::async_write(sock, boost::asio::buffer("Thanks for the message!"), boost::bind(&client_connection::write_handler, shared_from_this(), boost::asio::placeholders::error)); 
    } else {
      cout << "The host terminated the connection or there was an error. No more data will be read." << endl;
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

struct spreadsheet_comparator {
  bool operator() (const spreadsheet& lhs, const spreadsheet& rhs) const
  {return lhs.get_name() == rhs.get_name();}
};

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
