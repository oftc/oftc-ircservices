#ifndef INTERFACE_H
#define INTERFACE_H

struct Service;
extern dlink_list services_list;
extern struct Callback *newuser_cb;

struct Service *make_service(char *name);
void introduce_service(struct Service *service);
void init_interface();

struct Service
{
  dlink_node node;
  struct Service *hnext;

  char name[NICKLEN+1];
  struct ServiceMessageTree msg_tree;
};


#endif
