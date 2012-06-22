#ifndef INCLUDED_kill_h
#define INCLUDED_kill_h 

void add_kill(struct Service *, struct Client *, const char *);

struct KillRequest
{
  struct Service *service;
  struct Client *client;
  char *reason;
};

#endif
