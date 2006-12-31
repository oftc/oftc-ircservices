#ifndef PARSE_H
#define PARSE_H

#include <string>
#include <vector>
#include <tr1/unordered_map>

using std::string;
using std::vector;
using std::tr1::unordered_map;

class Message
{
public:
  Message() : count(0) {};
  Message(string const& n) : name(n), count(0) {};

  const char *c_name() { return name.c_str(); };

  virtual ~Message() = 0;
  virtual void handler(Client *, Client *, vector<string>) = 0;
protected:
  string name;
  unsigned int count;
};

class Parser
{
public:
  Parser() {};
  void add_message(Message *);
  void parse_line(Client *, string const&);
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
