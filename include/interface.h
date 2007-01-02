#ifndef INTERFACE_H
#define INTERFACE_H

#include <tr1/unordered_map>

using std::string;
using std::vector;
using std::tr1::unordered_map;

class ServiceMessage;
class Connection;
class Parser;
class Client;
class Server;
class Service;
class Protocol;

extern vector<Service *> service_list;
extern vector<Protocol *> protocol_list;
extern unordered_map<string, Service *> service_hash;

class Protocol
{
public:
  // Constructors
  Protocol() : _name("IRC"), parser(0), connection(0) {}; 
  Protocol(string const& n) : _name(n) {};

  virtual ~Protocol() {};

  // Members
  virtual void init(Parser *, Connection *);
  virtual void connected(bool=true);
  virtual void introduce_client(Server *);
  virtual void introduce_client(Client *);

  // Property Accessors
  const string& name() const { return _name; };
protected:
  string _name;
  Parser *parser;
  Connection *connection;
};

class Service
{
public:
  // Constructors
  Service() : _name(""), _client(0) {};
  Service(string const& name) : _name(name), _client(0) {};

  virtual ~Service() {};

  // Static members
  static Service *find(const string& name) { return service_hash[name]; };

  // Members
  virtual void init();
  virtual void handle_message(Connection *, Client *, string const&);

  // Property Accessors
  Client *client () const { return _client; };
protected:
  string _name;
  unordered_map<string, ServiceMessage *> message_map;
  void add_message(ServiceMessage *message);
  Client *_client;
};

enum ServiceBanType
{
  AKICK_BAN = 0,
  AKILL_BAN
};

struct ServiceBan
{
  unsigned int id;
  unsigned int type;
  char *channel;
  unsigned int target;
  unsigned int setter;
  char *mask;
  char *reason;
  time_t time_set;
  time_t duration;
};

struct ModeList
{
  unsigned int mode;
  unsigned char letter;
};

extern struct ModeList *ServerModeList;

void init_interface();
void cleanup_interface();

extern struct LanguageFile ServicesLanguages[LANG_LAST];

#endif
