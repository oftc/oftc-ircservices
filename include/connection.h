#ifndef CONNECTIONH
#define CONNECTIONH

#include <deque>
#include <string>
#include "parse.h"
#include "interface.h"

using std::deque;
using std::string;

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
  void set_client(Client *c) { client = c; };
  void connected() { protocol->connected(); };
  void send(string const&);
  const string& password() { return client->password(); };
  
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
  Parser *parser;
  Protocol *protocol;
  Client *client;
};

#endif
