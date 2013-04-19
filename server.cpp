/*
 * This is the server end of the spreadsheet client.
 *
 */
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;
using namespace std;

class ss_server {
private:
  //tcp::socket socket_;
  std::string client;
  std::string spreadsheetDirectory;
  std::string default_port;
  
public:
  /* 
   * Instantiate server. Load spreadsheet files. Start/Prepare for accepting client requests. Load socket.
   */
  ss_server() {
    boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);
    tcp::endpoint endpoint(tcp::v4(), 1984);
    tcp::acceptor acceptor_(io_service, endpoint);
    tcp::socket socket_(io_service);
    boost::asio::async_write(socket_, boost::asio::buffer("test", 15), boost::bind(&ss_server::sendCompleteHandler, this, boost::asio::placeholders::error));
  }

  /*
   *
   */
  void sendMessage(std::string message) { /*
    std::size_t size_in_bytes = message::size(); //Is message:size() not automatically convertible to size_t?
    boost::asio::async_write(socket_, boost::asio::buffer(message, size_in_bytes), sendCompleteHandler);
					  */
  }
  
  void receiveMessage() {
    /*
     try
      {
	boost::asio::io_service io_service;
	tcp::endpoint endpoint(tcp::v4(), default_port); //Set up an endpoint with IPv4 on our default port.
	tcp::acceptor acceptor(io_service, endpoint); //Create a socket acceptor

	for (;;)
	  {
	    tcp::iostream stream;
	    boost::system::error_code ec;
	    acceptor.accept(*stream.rdbuf(), ec);
	    if (!ec)
	      {
		stream << make_daytime_string();
	      }
	  }
      }
    catch (std::exception& e)
      {
	std::cerr << e.what() << std::endl;
      }

    */
  }

  /*
   * error: Result of operation
   * bytes_transferred: Number of bytes written. This should be checked to make sure the entire message was sent.
   */
  void sendCompleteHandler(const boost::system::error_code& error/*, std::size_t bytes_transferred*/) {
    cout << "Send was completed successfully.";
  }
};

int main() {
  cout << "Welcome to the server." << endl;
  ss_server s;
  cout << "Function completed" << endl;
}
