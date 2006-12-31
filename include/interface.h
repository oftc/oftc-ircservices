#ifndef INTERFACE_H
#define INTERFACE_H

#include <string>
#include "language.h"
#include "connection.h"

using std::string;

class Connection;

class Protocol
{
public:
  Protocol(); 
  void init(Parser *, Connection *);

  void connected();
  void introduce_server(Client *);
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
  Service(string const & name) : _name(name), _client(0) {};

  virtual ~Service() = 0;

  void introduce();
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

extern dlink_list services_list;
extern struct Callback *send_newuser_cb;
extern struct Callback *send_privmsg_cb;
extern struct Callback *send_notice_cb;
extern struct Callback *send_gnotice_cb;
extern struct Callback *send_umode_cb;
extern struct Callback *send_cloak_cb;
extern struct Callback *send_nick_cb;
extern struct Callback *send_akill_cb;
extern struct Callback *send_unakill_cb;
extern struct Callback *send_kick_cb;
extern struct Callback *send_cmode_cb;
extern struct Callback *send_invite_cb;
extern struct Callback *send_topic_cb;
extern struct Callback *send_kill_cb;

extern struct Callback *on_umode_change_cb;
extern struct Callback *on_cmode_change_cb;
extern struct Callback *on_squit_cb;
extern struct Callback *on_quit_cb;
extern struct Callback *on_part_cb;
extern struct Callback *on_join_cb;
extern struct Callback *on_nick_change_cb;
extern struct Callback *on_identify_cb;
extern struct Callback *on_newuser_cb;
extern struct Callback *on_channel_destroy_cb;
extern struct Callback *on_nick_drop_cb;
extern struct Callback *on_topic_change_cb;

extern struct ModeList *ServerModeList;

void init_interface();
void cleanup_interface();

extern struct LanguageFile ServicesLanguages[LANG_LAST];

extern vector<Service *> service_list;

#endif
