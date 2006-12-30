#ifndef CONNECTIONH
#define CONNECTIONH

#include <tr1/functional>
#include <deque>
#include "parse.h"

extern Client *me;

class Connection
{
public:
  // Members
  Connection();
  Connection(Parser *);
  void connect();
  void read();
  void process_queue();
  inline void setup_read() 
  { 
    comm_setselect(&fd, COMM_SELECT_READ, read_callback, this, 0); 
  };
  
  // Static members (callbacks)
  inline static void read_callback(fde_t *fd, void *data) 
  {
    Connection *connection = static_cast<Connection*>(data);

    connection->read();
  }
  static void connect_callback(fde_t *, int, void *);
  
private:
  fde_t fd;
  std::deque<std::string> read_queue;
  Parser *parser;
};

#endif
