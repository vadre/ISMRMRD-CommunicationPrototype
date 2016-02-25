// server.cpp

#include <cstdlib>
#include <iostream>
#include <thread>
#include <queue>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

using boost::asio::ip::tcp;

const int max_length = 1024;

typedef std::shared_ptr<tcp::socket> socket_ptr;
typedef std::shared_ptr<std::queue<std::string> > str_queue;


/*******************************************************************************
 ******************************************************************************/
void write_socket (socket_ptr sock, str_queue messages, int id)
{
  std::cout << "Writer thread (" << id << ") started\n" << std::flush;

  bool done = false;
  try
  {
    for (;;)
    {
      if (!done && messages->size() == 0)
      {
        sleep (1);
        continue;
      }
      std::string data = std::string ("Server returns: ");
      data.append (messages->front());
      messages->pop();

      if (data.find (std::string ("Client Command Stop")) != std::string::npos)
      {
        data = std::string ("Server Command Done");
        std::cout << "Writer (" << id << "): Sending: [" << data << "]\n";
        boost::asio::write (*sock, boost::asio::buffer (data, data.size()));
        sock->close();
        break;
      }

      std::cout << "Writer (" << id << "): Sending: [" << data << "]\n";
      boost::asio::write (*sock, boost::asio::buffer (data, data.size()));
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Writer (" << id << ") Exception: " << e.what() << "\n";
  }
}


/*******************************************************************************
 ******************************************************************************/
void read_socket (socket_ptr sock, int id)
{
  std::cout << "Reader thread (" << id << ") started\n" << std::flush;

  str_queue messages (new std::queue<std::string>);
  std::thread writer (write_socket, sock, messages, id);

  try
  {
    for (;;)
    {
      char data[max_length];

      boost::system::error_code error;
      size_t length = sock->read_some (boost::asio::buffer (data), error);

      messages->push (std::string (data));

      std::cout << "Reader (" << id << ") received: [" << data << "]" << std::endl;
      //std::cout.write (data, length);

      if (!std::strncmp (data, "Client Command Stop", std::strlen(data)))
      {
        std::cout << "Reader (" << id << ") received the Stop command, stopping reading\n";
        break;
      }

      if (error == boost::asio::error::eof)
      {
        std::cout << "Reader (" << id << "): Num bytes received = " << length << std::endl;
        std::cout << "Reader (" << id << "): Client disconnected, closing socket and exiting\n";
        break; // Connection closed cleanly by peer.
      }
      else if (error)
      {
        throw boost::system::system_error (error); // Some other error.
      }

    }

    writer.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Reader (" << id << "): Exception: " << e.what() << "\n";
  }
}


/*******************************************************************************
 ******************************************************************************/
int main (int argc, char* argv[])
{
  int port;

  try
  {
    if (argc != 2) port = 50050;
    else           port = std::atoi (argv[1]);

    std::cout << "Using port number " << port << std::endl;
    std::cout << "To connect to a different port, restart: iserv <port>\n\n";

    boost::asio::io_service io_service;
    tcp::acceptor a (io_service, tcp::endpoint (tcp::v4(), port));

    for (int id = 1;; ++id)
    {
      socket_ptr sock (new tcp::socket (io_service));
      a.accept (*sock);
      std::cout << "Accepted client (id " << id << "), starting communications...\n";
      std::thread (read_socket, sock, id).detach();
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Main thread Exception: " << e.what() << "\n";
  }

  return 0;
}
