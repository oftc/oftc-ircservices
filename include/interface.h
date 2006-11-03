#ifndef INTERFACE_H
#define INTERFACE_H

struct Service;
struct Client;

extern dlink_list services_list;
extern struct Callback *newuser_cb;
extern struct Callback *privmsg_cb;
extern struct Callback *notice_cb;
extern struct Callback *gnotice_cb;
extern struct Callback *umode_cb;
extern struct Callback *cloak_cb;

extern struct Callback *umode_hook;
extern struct Callback *cmode_hook;
extern struct Callback *squit_hook;
extern struct Callback *quit_hook;
extern struct Callback *part_hook;
extern struct Callback *nick_hook;

struct Service *make_service(char *);
void introduce_service(struct Service *);
void reply_user(struct Service *, struct Client *, const char *, ...);
void global_notice(struct Service *, char *, ...);
void cloak_user(struct Client *, char *);
void send_umode(struct Service *, struct Client *, const char *);
void init_interface();
void do_help(struct Service *, struct Client *, const char *, int, char **);
int identify_user(struct Client *, const char *);

void chain_umode(struct Client *, struct Client *, int, char **);
void chain_cmode(struct Client *, struct Client *, struct Channel *, int, char **);
void chain_squit(struct Client *, struct Client *, char *);
void chain_quit(struct Client *, char *);
void chain_part(struct Client *, struct Client *, char *);
void chain_nick(struct Client *, struct Client *, int, char **, int, char *, char *);

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
