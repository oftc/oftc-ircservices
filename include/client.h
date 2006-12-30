#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>
#include <tr1/unordered_map>

using std::string;
using std::vector;
using std::tr1::unordered_map;

class Client;

extern vector<Client *> GlobalClientList;
extern unordered_map<string, Client *> GlobalClientHash;

class Client
{
public:
  Client();
  Client(string const&);
  Client(string const&, string const&, string const&, string const&);
  void introduce();
  void kill();
  bool is_banned(struct Ban *);
  void change_nick(string const&);
  void change_umode(string const&);

  string nuh();
  inline const char *c_nuh() { return nuh().c_str(); };
  inline const char *c_name() { return name.c_str(); };
  inline const char *c_id()   { return id.c_str();   };
  inline const char *c_user() { return user.c_str(); };
  inline const char *c_host() { return host.c_str(); };

  inline void set_ts(time_t ts) { tsinfo = ts; };
  inline void set_name(string const& n) { name = n; };

  inline static Client *find(string const& name) 
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

void init_client();
struct Client *make_client(struct Client*);
struct Server *make_server(struct Client*);
struct Client *find_person(const struct Client *, const char *);
struct Client *find_chasing(struct Client *, const char *, int *);
void dead_link_on_write(struct Client *, int);
void set_user_mode(struct Client *, struct Client *, int, char *[]);
void exit_client(struct Client *, struct Client *, const char *);
int check_clean_nick(struct Client *, struct Client *, char *, char *, 
    struct Client *);
int check_clean_user(struct Client *, char *, char *, struct Client *);
int check_clean_host(struct Client *, char *, char *, struct Client *);
void nick_from_server(struct Client *, struct Client *, int,
                     char *[], time_t, char *, char *);
void register_remote_user(struct Client *, struct Client *,
                         const char *, const char *, const char *, const char *);

#endif
