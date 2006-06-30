#ifndef INTERFACE_H
#define INTERFACE_H

struct Service;
extern dlink_list services_list;
extern struct Callback *newuser_cb;

struct Service *make_service(char *name);

struct Service
{
  dlink_node node;
  struct Service *hnext;

  char name[NICKLEN+1];
  struct MessageTree msg_tree;
};

void introduce_service(struct Service *service);

#endif
