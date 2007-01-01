#ifndef INTERFACE_H
#define INTERFACE_H

using std::string;
using std::vector;
using std::tr1::unordered_map;

class Connection;
class Parser;
class Client;
class Server;
class Service;

extern vector<Service *> service_list;
extern unordered_map<string, Service *> service_hash;

class Protocol
{
public:
  Protocol(); 
  void init(Parser *, Connection *);

  void connected();
  void introduce_client(Server *);
  void introduce_client(Client *);
protected:
  string name;
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
  void init();
  virtual void handle_message(Connection *, Client *, string const&);

  // Property Accessors
  Client *client () const { return _client; };
protected:
  string _name;
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
