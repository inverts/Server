/*
 * This needs to be edited/altered for our server service.
 */
#include <boost/asio.hpp> 
#include <boost/array.hpp>
#include <iostream>  
#include <string> 

using namespace std;

boost::asio::io_service io_service; 
boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 1984); 
boost::asio::ip::tcp::socket sock(io_service); 
boost::array<char, 2048> buffer; //This is the buffer we use to read in messages.

void write_handler(const boost::system::error_code &ec, std::size_t bytes_transferred); //Forward declaration.

void read_handler(const boost::system::error_code &ec, std::size_t bytes_transferred) 
{ 
  if (!ec) {
    cout << "The server read an incoming message:" << endl; 
    cout << "---------------- Message Received ----------------" << endl;
    cout << string(buffer.data(), bytes_transferred) << endl; 
    cout << "------------------ End Message -------------------" << endl;
    cout << "Server is sending an acknowledgement response." << endl;
    boost::asio::async_write(sock, boost::asio::buffer("Thanks for the message!"), write_handler); 
  } else {
    cout << "The host terminated the connection or there was an error. No more data will be read." << endl;
  }
}

void write_handler(const boost::system::error_code &ec, std::size_t bytes_transferred) 
{ 
  sock.async_read_some(boost::asio::buffer(buffer), read_handler); //Continue reading message(s) from the socket.
} 

void accept_handler(const boost::system::error_code &ec) 
{ 
  if (!ec) 
    { 
      cout << "New incoming connection received." << endl;
      boost::asio::async_write(sock, boost::asio::buffer("You have successfully connected to the server."), write_handler); 
    } 
} 

int main(int argc, char* argv[]) 
{ 
  if (argc != 2) {
    cout << "You must specify a port you would like this server to run on." << endl;
    return 1;
  }
  int port = atoi(argv[1]); //Load in the port number.

  endpoint.port(port); 
  boost::asio::ip::tcp::acceptor acceptor(io_service, endpoint); 
  acceptor.listen();
  cout << "The socket has been opened and this server is now listening for connections." << endl; 
  acceptor.async_accept(sock, accept_handler); 
  io_service.run(); 
} 
