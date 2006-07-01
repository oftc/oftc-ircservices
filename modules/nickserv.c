#include "stdinc.h"

static struct Service *nickserv = NULL;

static void m_register(struct Service *, struct Client *, int, char *[]);

static struct ServiceMessage register_msgtab = {
  "REGISTER", 0, 2,
  { m_register, m_servignore, m_servignore, m_servignore }
};

INIT_MODULE(nickserv, "$Revision: 470 $")
{
  nickserv = make_service("NickServ");
  clear_serv_tree_parse(&nickserv->msg_tree);
  dlinkAdd(nickserv, &nickserv->node, &services_list);
  hash_add_service(nickserv);
  introduce_service(nickserv);

  mod_add_servcmd(&nickserv->msg_tree, &register_msgtab);
}

CLEANUP_MODULE
{
}

static void *
ns_find_nick(char *nick)
{
  printf("We try to find %s\n", nick);
  return (void *) 1;  // XXX FIXME XXX
}

static void 
m_register(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  printf("ZOMG %s wants to register with %s\n", client->name, service->name);
  if (ns_find_nick(client->name) != NULL)
  {
    reply_user(service, client, "NICK EXISTS");
    return;
  }
}

