#ifndef INCLUDED_kill_h
#define INCLUDED_kill_h 

void kill_user(struct Service *, struct Client *, const char *);

struct KillRequest
{
  struct Service *service;
  struct Client *client;
  char *reason;
};

#endif
