#ifndef PARSE_H
#define PARSE_H

#include <string>
#include <vector>
#include <tr1/unordered_map>

using std::string;
using std::vector;
using std::tr1::unordered_map;

class Server;
class BaseClient;

class Message
{
public:
  Message() : count(0) {};
  Message(string const& n) : name(n), count(0) {};

  const char *c_name() { return name.c_str(); };

  virtual ~Message() = 0;
  virtual void handler(Server *, BaseClient *, vector<string>) = 0;
protected:
  string name;
  unsigned int count;
};

class IgnoreMessage : public Message
{
public:
  IgnoreMessage(string const& n) : Message(n) {};
  IgnoreMessage() { };
  void handler(Server *uplink, BaseClient *source, vector<string>) {};
};

class ErrorMessage : public Message
{
public:
  ErrorMessage() : Message("ERROR") {};
  ~ErrorMessage() {};
  void handler(Server *, BaseClient *, vector<string>);
};

class PingMessage : public Message
{
public:
  PingMessage() : Message("PING") {};
  ~PingMessage() {};
  void handler(Server *, BaseClient *, vector<string>);
};

class ServerMessage : public Message
{
public:
  ServerMessage() : Message("SERVER"), ts(5) {};
  ServerMessage(int ts) : Message("SERVER"), ts(6) {};
  ~ServerMessage() {};
  void handler(Server *, BaseClient *, vector<string>);
private:
  int ts;
};

class NickMessage : public Message
{
public:
  NickMessage() : Message("NICK") {};
  ~NickMessage() {};
  void handler(Server *, BaseClient *, vector<string>);
};

class PrivmsgMessage : public Message
{
public:
  PrivmsgMessage() : Message("PRIVMSG") {};
  ~PrivmsgMessage() {};

  void handler(Server *, BaseClient *, vector<string>);
};

class Parser
{
public:
  Parser() {};
  void add_message(Message *);
  void parse_line(Connection *, string const&);
private:
  unordered_map<string, Message *> message_map;
};

class irc_string : private string
{
public:
  irc_string(const string& s) : string(s) {};
  vector<string> split(const string&, size_type=0);
  char& operator[]( size_type index )
  {
    return string::operator[](index);
  };
  string substr(size_type index, size_type num = npos)
  {
    return string::substr(index, num);
  }
};

#endif
