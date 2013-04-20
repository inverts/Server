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
  }

  /*
   * Sends a message.
   */
  void sendMessage(std::string message) { 
    boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);
    tcp::resolver::query query("155.98.111.122", "1984");//155.98.108.62
    tcp::resolver::iterator iterator = resolver.resolve(query);
    cout << "Creating endpoint." << endl;
    tcp::endpoint endpoint = *iterator;
    //tcp::acceptor acceptor_(io_service, endpoint);
    tcp::socket socket_(io_service);
    cout << "About to write." << endl;
    socket_.open(tcp::v4());
    socket_.connect(endpoint); //Connects the socket to this particular endpoint
    //socket_.async_send(boost::asio::buffer(message, message.size()), boost::bind(&ss_server::sendCompleteHandler2, this, boost::asio::placeholders::error));
    boost::asio::async_write(socket_, boost::asio::buffer(message, message.size()), boost::bind(&ss_server::sendCompleteHandler, this, boost::asio::placeholders::error));
    socket_.close();
    std::string input;
    cin >> input;
  }
  
  /*
   * Receives a message.
   */
  void receiveMessage() {
    /*
      std::size_t size_in_bytes = message::size(); //Is message:size() not automatically convertible to size_t?
      boost::asio::async_write(socket_, boost::asio::buffer(message, size_in_bytes), sendCompleteHandler);
    */
  }

  /*
   * Callback function for a complete send operation.
   * error: Result of operation
   * bytes_transferred: Number of bytes written. This should be checked to make sure the entire message was sent.
   */
  void sendCompleteHandler(const boost::system::error_code& error/*, std::size_t bytes_transferred*/) {
    cout << "Send was completed successfully." << endl;
  }

  /*
   * Callback function for a complete send operation.
   * error: Result of operation
   * bytes_transferred: Number of bytes written. This should be checked to make sure the entire message was sent.
   */
  void sendCompleteHandler2(const boost::system::error_code& error, std::size_t bytes_transferred) {
    cout << "Send was completed successfully 2." << endl;
  }
};

int main() {
  cout << "Welcome to the server." << endl;
  ss_server s;
  s.sendMessage("test");
  cout << "Function completed" << endl;
}
