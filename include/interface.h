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
extern struct Callback *send_nick_cb;

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

struct Service *make_service(char *);
void introduce_service(struct Service *);
void reply_user(struct Service *, struct Client *, unsigned int, ...);
void global_notice(struct Service *, char *, ...);
void cloak_user(struct Client *, char *);
void send_umode(struct Service *, struct Client *, const char *);
void init_interface();
void do_help(struct Service *, struct Client *, const char *, int, char **);
void identify_user(struct Client *);
void send_nick_change(struct Service *, struct Client *, const char *);

void chain_cmode(struct Client *, struct Client *, struct Channel *, int, char **);
void chain_squit(struct Client *, struct Client *, char *);
void chain_quit(struct Client *, char *);
void chain_part(struct Client *, struct Client *, char *);
void chain_nick(struct Client *, struct Client *, int, char **, int, char *, char *);
void chain_join(struct Client *, char *);

char *replace_string(char *, const char *);
int check_list_entry(unsigned int, unsigned int, const char *);
int check_nick_pass(struct Nick *, const char *);
void make_random_string(char *, size_t);

void free_nick(struct Nick *);
void free_regchan(struct RegChannel *);
void free_akill(struct AKill *);

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

#endif
