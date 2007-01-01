#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>
#include <tr1/unordered_map>

class Client;
class Connection;

using std::string;
using std::vector;
using std::tr1::unordered_map;

class BaseClient;

extern vector<BaseClient *> GlobalClientList;
extern unordered_map<string, BaseClient *> GlobalClientHash;

class BaseClient
{
public:
  // Constructors
  BaseClient() : _name(""), _host(""), _gecos("") {};
  BaseClient(string const& name) : _name(name), _host(""), _gecos("") {};
  BaseClient(string const& name, string const& host, string const& gecos) :
    _name(name), _host(host), _gecos(gecos) {};
 
  // Virtual destructor to make this an abstract class
  virtual ~BaseClient() = 0;

  // Static Methods
  static BaseClient *find(const string& str)
  { 
    return GlobalClientHash[str]; 
  };

  // Methods
  virtual void init();

  // Property Accessors
  const string& name()   const { return _name; };
  const string& host()   const { return _host; };
  const string& gecos()  const { return _gecos; };

  // Property Setters
  void set_name(string const& n) { _name = n.substr(0, NICKLEN); };
protected:
  // Properties
  string _name;
  string _host;
  string _gecos;
};

class Client : public BaseClient
{
public:
  // Constructors
  Client() : BaseClient(), _username("") {};
  Client(string const& name) : BaseClient(name), _username("") {};
  Client(string const& name, string const& host, string const& gecos) :
    BaseClient(name, host, gecos), _username("") {};
  Client(string const& name, string const& username, string const& host, 
      string const& gecos) : 
    BaseClient(name, host, gecos), _username(username) {};

  ~Client();

  // Methods
  const string nuh() const;
  void init();

  // Property Accessors
  const string& username() const { return _username; };

  // Property Setters
  void set_ts(time_t ts) { _ts = ts; };
protected:
  string _username;
  time_t _ts;
};

class Server : public BaseClient
{
public:
  // Constructors
  Server() : BaseClient() {};
  Server(string const& name, string const& gecos) : 
    BaseClient(name, name, gecos) {};

  ~Server();

  // Methods 
  void init();
  void send(string const &m) const { connection->send(m); };
  
  // Property Setters
  void set_connection(Connection *c) { connection = c; };
  
private:
  Connection *connection;
};

#endif
