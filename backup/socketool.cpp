/*
 * A socket tool that allows the user to attempt a connection to any
 * host on any port, and then proceed to send and receive messages with
 * that host, if a connection is made.
 *
 * Author: Dave Wong
 */

#include <boost/asio.hpp> 
#include <boost/array.hpp> 
#include <iostream> 
#include <string> 

using namespace std;

boost::asio::io_service io_service; 
boost::asio::ip::tcp::resolver resolver(io_service); 
boost::asio::ip::tcp::socket sock(io_service); 

boost::array<char, 2048> buffer; //This is the buffer we use to read in messages.

void connected_handler(const boost::system::error_code &ec);

void prompt_send_message();

/*
 * Callback/handler for when a message is read from the socket.
 */
void read_handler(const boost::system::error_code &ec, std::size_t bytes_transferred) 
{ 
  // If there was no error on async_read_some, read the data placed in the buffer.
  // Note that if the message is long enough, it may arrive in chunks, depending on the buffer size.
  if (!ec) 
  { 
    cout << "---------------- Message Received ----------------" << endl;
    cout << string(buffer.data(), bytes_transferred) << endl; 
    cout << "------------------ End Message -------------------" << endl;
    cout << "\nWaiting for more data..." << endl;
    sock.async_read_some(boost::asio::buffer(buffer), read_handler); //Continue reading message(s) from the socket.
    //Allow the user to send some more data:
    prompt_send_message();

  } else {
    cout << "The host terminated the connection or there was an error. No more data will be read." << endl;
  }
} 

void write_handler(const boost::system::error_code &ec, std::size_t bytes_transferred);

void prompt_send_message() {
    cout << "What message would you like to send to the host? (500 char limit)" << endl;
    char message[500];
    cin.getline(message, 500); //NOTE - This will need to be altered if you need to test multi-line messages.
    cout << "Attemping to send your message..." << endl;
    cout << "\"" << message << "\"" << endl;
    boost::asio::async_write(sock, boost::asio::buffer(message, strlen(message)), write_handler); 
}

void write_handler(const boost::system::error_code &ec, std::size_t bytes_transferred) 
{ 
  if (!ec) {
  cout << "Message successfully sent to host. Now waiting for a response..." << endl;
  sock.async_read_some(boost::asio::buffer(buffer), read_handler); //Start reading data.
  } else {
    cout << "Could not send message to host - connection probably terminated." << endl;
  }
} 


/*
 * Callback/handler for when a connection is established
 */
void connected_handler(const boost::system::error_code &ec) 
{ 
  //If there is no error, we can begin communicating through this socket.
  if (!ec) 
    { 
      cout << "A connection was made to the server." << endl;
      sock.async_read_some(boost::asio::buffer(buffer), read_handler); //Start reading data.
    } else {
    cout << "There was an issue connecting to the host." << endl;
  }
} 

/*
 * Callback/handler for when an endpoint is resolved.
 */
void resolve_handler(const boost::system::error_code &ec, boost::asio::ip::tcp::resolver::iterator it) 
{ 
  //If there was no error resolving the destination, attempt to connect.
  if (!ec) 
    sock.async_connect(*it, connected_handler); 
  else
    cout << "Sorry, an endpoint could not be resolved from the host given." << endl;
} 

/*
 * Entry point for the application.
 * Usage:
 * sockettool <host> <port>
 */
int main(int argumentCount, const char* argumentValues[]) 
{ 
  if(argumentCount != 3) {
    cout << "You did not specify enough arguments." << endl;
    cout << "Usage: sockettool <host> <port>" << endl;
    return 1; 
  }
  string host = argumentValues[1];
  string port = argumentValues[2];
  cout << "Attempting to connect to " << host << " on port " << port << "..." << endl;
  //Build a query for connecting to an endpoint.
  boost::asio::ip::tcp::resolver::query query(host, port); 
  //Attempt to asynchronously resolve the endpoint.
  resolver.async_resolve(query, resolve_handler); 
  io_service.run(); 
} 
