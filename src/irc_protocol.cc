#include "stdinc.h"
#include "interface.h"
#include <string>
#include <sstream>
#include <stdexcept>

using std::string;
using std::stringstream;
using std::runtime_error;

class IgnoreMessage : public Message
{
public:
  IgnoreMessage(string const& n) : Message(n) {};
  IgnoreMessage()
  {
  };
  void handler(Server *uplink, Client *source, vector<string> args)
  {
  }
};

class ErrorMessage : public Message
{
public:
  ErrorMessage() : Message("ERROR") {};
  ~ErrorMessage()
  {
  };
  void handler(Server *uplink, Client *source, vector<string> args)
  {
    string arg;

    if(args.size() == 0)
      arg = "<>";
    else
      arg = args[0];

    if(uplink == source)
      ilog(L_DEBUG, "Error :from %s -- %s", source->c_name(), arg.c_str());
    else
      ilog(L_DEBUG, "Error :from %s via %s -- %s", source->c_name(), 
          uplink->c_name(), arg.c_str());
  };
};

class PingMessage : public Message
{
public:
  PingMessage() : Message("PING") {};
  ~PingMessage()
  {
  };
  void handler(Server *uplink, Client *source, vector<string> args)
  {
    stringstream ss;

    ss << ":" << me->s_name() << " PONG " << me->s_name() << " :" << 
      source->s_name();
    uplink->send(ss.str());
  };
};

class ServerMessage : public Message
{
public:
  ServerMessage() : Message("SERVER") {};
  ~ServerMessage()
  {
  };
  void handler(Server *uplink, Client *source, vector<string> args)
  {
    stringstream ss;
  };
};

Protocol::Protocol() : name("IRC"), parser(0), connection(0)
{
}

void
Protocol::init(Parser *p, Connection *c)
{
  PingMessage *ping = new PingMessage();
  ErrorMessage *error = new ErrorMessage();
  ServerMessage *server = new ServerMessage();
  IgnoreMessage *ignore; 

  parser = p;
  connection = c;

  parser->add_message(ping);
  parser->add_message(error);
  parser->add_message(server);
}

void Protocol::connected()
{
  stringstream ss;

  ss << "PASS " << connection->password() << " TS 5";
  connection->send(ss.str());
  connection->send("CAPAB :KLN PARA EOB QS UNKLN GLN ENCAP TBURST CHW IE EX");
  introduce_server(me);

  connection->process_send_queue();
}

void Protocol::introduce_server(Client *client)
{
  stringstream ss;

  ss << "SERVER " << client->s_name() << " 1 :" << client->s_info();
  connection->send(ss.str());
}
