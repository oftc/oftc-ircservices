#include "stdinc.h"

static struct Service *operserv = NULL;

static void m_mod(struct Service *, struct Client *, int, char *[]);

// FIXME wrong type of clients may execute
static struct ServiceMessage mod_msgtab = {
  "MOD", 0, 1,
  { m_mod, m_servignore, m_servignore, m_servignore }
};

INIT_MODULE(operserv, "$Revision: 0 $")
{
  operserv = make_service("OperServ");
  clear_serv_tree_parse(&operserv->msg_tree);
  dlinkAdd(operserv, &operserv->node, &services_list);
  hash_add_service(operserv);
  introduce_service(operserv);

  mod_add_servcmd(&operserv->msg_tree, &mod_msgtab);
}

CLEANUP_MODULE
{
}

static void 
m_mod(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  char *action = parv[1];
  char *parm = NULL;

  if (parc == 2) {
    parm = parv[2];
  }
  // FIXME insecure strcmp
  if (strcmp(action, "LOAD") == 0)
  {
  } else if (strcmp(action, "UNLOAD") == 0)
  {
  } else if (strcmp(action, "LIST") == 0)
  {
  }
  else
  {
    reply_user(service, client, "SYNTAX ERROR");
  }
}

