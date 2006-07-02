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
  char *mbn;
  struct Module *module;

  if (parc == 2) {
    parm = parv[2];
  }
  
  // FIXME insecure strcmp
  if (strcmp(action, "LOAD") == 0)
  {
    mbn = basename(parm);

    if (find_module(mbn, 0) != NULL)
    {
      reply_user(service, client, "Module already loaded");
      return;
    }

    if (parm == NULL)
    {
      reply_user(service, client, "You need to specify the modules name");
      return;
    }

    global_notice(service, "Loading %s by request of %s",
      parm, client->name);
    if (load_module(parm) == 1)
    {
      global_notice(service, "Module %s loaded", parm);
    }
    else
    {
      global_notice(service, "Module %s could not be loaded!", parm);
    }
  } 
  else if (strcmp(action, "UNLOAD") == 0)
  {
    mbn = basename(parm);
    module = find_module(mbn, 0);
    if (module == NULL)
    {
      global_notice(service, 
        "Module %s unload requested by %s, but failed because not loaded", 
        parm, client->name);
    }
    global_notice(service, "Unloading %s by request of %s", parm, client->name);
    unload_module(module);
  } 
  else if (strcmp(action, "LIST") == 0)
  {
  }
  else
  {
    reply_user(service, client, "SYNTAX ERROR");
  }
}

