/*
 * This is the server end of the spreadsheet client.
 *
 */
#include <boost/asio.hpp>
#include <iostream>
#include <string>

class server {
private:
  tcp::socket socket_;
  std::string client;
  std::string spreadsheetDirectory;
  const int port 1984;

public:
  /* 
   * Instantiate server. Load spreadsheet files. Start/Prepare for accepting client requests. Load socket.
   */
  server() {
  }

  void sendMessage(message) {
    boost::asio::async_write(socket_, 
			     boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
			     boost::bind(&chat_session::handle_write, shared_from_this(),
					 boost::asio::placeholders::error));
  }
  
  void receiveMessage() {
  }
};

int main() {
  cout("Welcome to the server.");
}
