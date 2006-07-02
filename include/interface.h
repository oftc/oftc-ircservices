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

struct Service *make_service(char *);
void introduce_service(struct Service *);
void reply_user(struct Service *, struct Client *, const char *, ...);
void global_notice(struct Service *, char *, ...);
void send_umode(struct Service *, struct Client *, const char *);
void init_interface();

struct Service
{
  dlink_node node;
  struct Service *hnext;

  char name[NICKLEN+1];
  struct ServiceMessageTree msg_tree;
};


#endif
