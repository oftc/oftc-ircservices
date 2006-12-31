#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>
#include <tr1/unordered_map>

class Client;
class Connection;

#include "connection.h"

using std::string;
using std::vector;
using std::tr1::unordered_map;


extern vector<Client *> GlobalClientList;
extern unordered_map<string, Client *> GlobalClientHash;

class Client
{
public:
  Client();
  Client(string const& n) : name(n.substr(0, NICKLEN)) {};
  Client(string const&, string const&, string const&, string const&);
  void introduce();
  void kill();
  bool is_banned(struct Ban *) const;
  void change_nick(string const&);
  void change_umode(string const&);

  string nuh() const;
  const char *c_nuh()   const { return nuh().c_str(); };
  const char *c_name()  const { return name.c_str(); };
  const char *c_id()    const { return id.c_str();   };
  const char *c_user()  const { return user.c_str(); };
  const char *c_host()  const { return host.c_str(); };

  const string& s_name() const { return name; };
  const string& s_info() const { return info; };

  void set_ts(time_t ts) { tsinfo = ts; };
  void set_name(string const& n) { name = n; };

  static Client *find(string const& name) 
  {
    return GlobalClientHash[name];
  }

protected:
  string name;
  string host;
  string sockhost;
  string id;
  string info;
  string user;

  time_t tsinfo;
  time_t enforce_time;
  time_t release_time;
};

class Server : public Client
{
public:
  Server() : Client() {};
  Server(string const& a, string const& b, string const& c, string const& d) :
    Client(a, b, c, d) {};
  void set_connection(Connection *c) { connection = c; };
  void send(string const &m) { connection->send(m); };
  
private:
  Connection *connection;
};

#endif
