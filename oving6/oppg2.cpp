#include <algorithm>
#include <boost/asio.hpp>
#include <iostream>
#include <string>

using namespace std;
using namespace boost::asio::ip;

class EchoServer {
private:
  class Connection {
  public:
    tcp::socket socket;
    Connection(boost::asio::io_service &io_service) : socket(io_service) {}
  };

  boost::asio::io_service io_service;

  tcp::endpoint endpoint;
  tcp::acceptor acceptor;

  void handle_request(shared_ptr<Connection> connection) {
    auto read_buffer = make_shared<boost::asio::streambuf>();
    // Read from client until newline ("\r\n")
    async_read_until(connection->socket, *read_buffer, "\r\n",
                     [this, connection, read_buffer](const boost::system::error_code &ec, size_t) {
                       // If not error:
                       if (!ec) {
                         // Retrieve message from client as string:
                         istream read_stream(read_buffer.get());
                         std::string message;
                         getline(read_stream, message);

                         cout << "Message from a connected client: " << message << endl;

                         auto write_buffer = make_shared<boost::asio::streambuf>();
                         ostream write_stream(write_buffer.get());

                         vector<string> tokens = parse(&message);

                         if (tokens.size() == 3 && tokens[0] == "GET") {

                           string response;
                           string code;

                           if (tokens[1] == "/") {
                             response = "Dette er hovedsiden";
                             code = "200 OK";
                           } else if (tokens[1] == "/en_side") {
                             response = "Dette er en side";
                             code = "200 OK";
                           } else {
                             response = "<img src='https://i.stack.imgur.com/6M513.png'>";
                             code = "404 Not Found";
                           }

                           write_stream
                               << "HTTP/1.1 " << code << "\n"
                               << "Content-Type: text/html\n"
                               << "Content-Length: " << response.size() << "\n"
                               << "\n"
                               << response
                               << endl;
                         }

                         // Write to client
                         async_write(connection->socket, *write_buffer,
                                     [this, connection, write_buffer](const boost::system::error_code &ec, size_t) {
                                       // If not error:
                                       if (!ec)
                                         handle_request(connection);
                                     });
                       }
                     });
  }

  vector<string> parse(string *message) {
    vector<string> tokens;
    string delimiter = " ";

    size_t pos = 0;
    while ((pos = message->find(delimiter)) != std::string::npos) {
      tokens.emplace_back(message->substr(0, pos));
      message->erase(0, pos + delimiter.length());
    }
    tokens.emplace_back(message->substr(0, message->find("\n") - 1));
    return tokens;
  }

  // void get(string *request) {
  // }

  void accept() {
    // The (client) connection is added to the lambda parameter and handle_request
    // in order to keep the object alive for as long as it is needed.
    auto connection = make_shared<Connection>(io_service);

    // Accepts a new (client) connection. On connection, immediately start accepting a new connection
    acceptor.async_accept(connection->socket, [this, connection](const boost::system::error_code &ec) {
      accept();
      // If not error:
      if (!ec) {
        handle_request(connection);
      }
    });
  }

public:
  EchoServer() : endpoint(tcp::v4(), 8080), acceptor(io_service, endpoint) {}

  void start() {
    accept();

    io_service.run();
  }
};

int main() {
  EchoServer echo_server;

  cout << "Starting HTTP server: http://localhost:8080/" << endl;

  echo_server.start();
}
