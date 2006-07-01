#ifndef INTERFACE_H
#define INTERFACE_H

struct Service;
extern dlink_list services_list;
extern struct Callback *newuser_cb;
extern struct Callback *privmsg_cb;
extern struct Callback *notice_cb;

struct Service *make_service(char *name);
void introduce_service(struct Service *service);
void reply_user(struct Service *service, struct Client *client, char *text);
void init_interface();

struct Service
{
  dlink_node node;
  struct Service *hnext;

  char name[NICKLEN+1];
  struct ServiceMessageTree msg_tree;
};


#endif
