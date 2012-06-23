#ifndef INCLUDED_kill_h
#define INCLUDED_kill_h 

void kill_user(struct Service *, struct Client *, const char *);
void kill_remove_service(struct Service *);
void kill_remove_client(struct Client *);

struct KillRequest
{
  struct Service *service;
  struct Client *client;
  const char *reason;
};

#endif
