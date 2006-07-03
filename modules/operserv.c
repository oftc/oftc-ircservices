#include "stdinc.h"

static struct Service *operserv = NULL;

static void m_mod(struct Service *, struct Client *, int, char *[]);
static void m_mod_list(struct Service *, struct Client *, int, char *[]);
static void m_mod_load(struct Service *, struct Client *, int, char *[]);
static void m_mod_unload(struct Service *, struct Client *, int, char *[]);


// FIXME wrong type of clients may execute
//
static struct SubMessage mod_subs[4] = {
  { "LIST", 0, 0, -1, -1, m_mod_list },
  { "LOAD", 0, 1, -1, -1, m_mod_load },
  { "UNLOAD", 0, 1, -1, -1, m_mod_unload },
  { NULL, 0, 0, 0, 0, NULL }
};
static struct ServiceMessage mod_msgtab = {
  mod_subs, "MOD", 0, 1, -1, -1,
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
  reply_user(service, client, "Unknown MOD command");  
}
static void
m_mod_list(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  reply_user(service, client, "LIST not implemented");
}

static void
m_mod_load(struct Service *service, struct Client *client,
            int parc, char *parv[])
{
  char *parm = parv[1];
  char *mbn;

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
    reply_user(service, client,  "Module %s loaded", parm);
  }
  else
  {
    global_notice(service, "Module %s could not be loaded!", parm);
    reply_user(service, client,  "Module %s could not be loaded!", parm);
  }
}

static void
m_mod_unload(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  char *parm = parv[1];
  char *mbn;
  struct Module *module;

  mbn = basename(parm);
  module = find_module(mbn, 0);
  if (module == NULL)
  {
    global_notice(service,
        "Module %s unload requested by %s, but failed because not loaded",
        parm, client->name);
    reply_user(service, client,  
        "Module %s unload requested by %s, but failed because not loaded",
        parm, client->name);
  }
  global_notice(service, "Unloading %s by request of %s", parm, client->name);
  reply_user(service, client, "Unloading %s", parm, client->name);
  unload_module(module);
}
