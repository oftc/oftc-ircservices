#ifndef CONNECTIONH
#define CONNECTIONH

#include <deque>
#include <string>

using std::deque;
using std::string;

class Client;
class Parser;
class Server;
class Protocol;

extern Client *me;

class Connection
{
public:
  // Members
  Connection() : parser(0) {};
  Connection(Parser *p) : parser(p) {};
  void connect();
  void read();
  void process_read_queue();
  void process_send_queue();
  void setup_read() 
  { 
    comm_setselect(&fd, COMM_SELECT_READ, read_callback, this, 0); 
  };
  void set_server(Server *s) { server = s; };
  void set_password(string const& p) { pass = p; };
  void connected() { protocol->connected(); };
  void send(string const&);
  const string& password() const { return pass; };
  Server *serv() const { return server; };
  
  // Static members (callbacks)
  static void read_callback(fde_t *fd, void *data) 
  {
    Connection *connection = static_cast<Connection*>(data);

    connection->read();
  }
  static void connect_callback(fde_t *, int, void *);
  
private:
  fde_t fd;
  deque<string> read_queue;
  deque<string> send_queue;
  string pass;
  Parser *parser;
  Protocol *protocol;
  Server *server;
};

#endif
