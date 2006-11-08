#ifndef INTERFACE_H
#define INTERFACE_H

struct Service;
struct Client;

extern dlink_list services_list;
extern struct Callback *send_newuser_cb;
extern struct Callback *send_privmsg_cb;
extern struct Callback *send_notice_cb;
extern struct Callback *send_gnotice_cb;
extern struct Callback *send_umode_cb;
extern struct Callback *send_cloak_cb;

extern struct Callback *on_umode_change_cb;
extern struct Callback *on_cmode_change_cb;
extern struct Callback *on_squit_cb;
extern struct Callback *on_quit_cb;
extern struct Callback *on_part_cb;
extern struct Callback *on_nick_change_cb;
extern struct Callback *on_identify_cb;
extern struct Callback *on_newuser_cb;

struct Service *make_service(char *);
void introduce_service(struct Service *);
void reply_user(struct Service *, struct Client *, const char *, ...);
void global_notice(struct Service *, char *, ...);
void cloak_user(struct Client *, char *);
void send_umode(struct Service *, struct Client *, const char *);
void init_interface();
void do_help(struct Service *, struct Client *, const char *, int, char **);
void identify_user(struct Client *);

void chain_umode(struct Client *, struct Client *, int, char **);
void chain_cmode(struct Client *, struct Client *, struct Channel *, int, char **);
void chain_squit(struct Client *, struct Client *, char *);
void chain_quit(struct Client *, char *);
void chain_part(struct Client *, struct Client *, char *);
void chain_nick(struct Client *, struct Client *, int, char **, int, char *, char *);

char *replace_string(char *, const char *);
int check_list_entry(const char *, unsigned int, const char *);

struct Service
{
  dlink_node node;
  struct Service *hnext;

  char name[NICKLEN+1];
  struct ServiceMessageTree msg_tree;
  char *last_command;
  char *language_table[LANG_LAST][LANG_TABLE_SIZE];
  void *data;
};

enum identify_errors
{
  ERR_ID_NOERROR = 0,
  ERR_ID_NONICK,
  ERR_ID_WRONGPASS
};

#endif
