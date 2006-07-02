#include "stdinc.h"

static struct Service *nickserv = NULL;

static void m_register(struct Service *, struct Client *, int, char *[]);
static void m_identify(struct Service *, struct Client *, int, char *[]);

static struct ServiceMessage register_msgtab = {
  "REGISTER", 0, 2,
  { m_register, m_alreadyreg, m_alreadyreg, m_alreadyreg}
};

static struct ServiceMessage identify_msgtab = {
  "IDENTIFY", 0, 1,
  { m_identify, m_identify, m_identify, m_identify }
};


INIT_MODULE(nickserv, "$Revision: 470 $")
{
  nickserv = make_service("NickServ");
  clear_serv_tree_parse(&nickserv->msg_tree);
  dlinkAdd(nickserv, &nickserv->node, &services_list);
  hash_add_service(nickserv);
  introduce_service(nickserv);
  load_language(nickserv, "en");

  mod_add_servcmd(&nickserv->msg_tree, &register_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &identify_msgtab);
}

CLEANUP_MODULE
{
}

static void
identify_user(struct Client *client)
{
  if(IsOper(client))
    client->service_handler = OPER_HANDLER;
  else
    client->service_handler = REG_HANDLER;

  SetRegistered(client);

  send_umode(nickserv, client, "+R");
}

static void 
m_register(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Nick *nick;
  
  if (db_find_nick(client->name) != NULL)
  {
    reply_user(service, client, _N(client, NS_ALREADY_REG), client->name); 
    return;
  }

  nick = MyMalloc(sizeof(struct Nick));
  strlcpy(nick->nick, client->name, sizeof(nick->nick));
  strlcpy(nick->pass, crypt_pass(parv[1]), sizeof(nick->pass));

  client->nickname = nick;
  if(db_register_nick(client, parv[2]) >= 0)
  {
    identify_user(client);
    reply_user(service, client, _N(client, NS_REG_COMPLETE), client->name);
    global_notice(NULL, "%s!%s@%s registered nick %s\n", client->name, 
        client->username, client->host, nick->nick);

    return;
  }
  reply_user(service, client, _N(client, NS_REG_FAIL), client->name);
}

static void
m_identify(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  struct Nick *nick;

  if((nick = db_find_nick(client->name)) == NULL)
  {
    reply_user(service, client, _N(client, NS_REG_FIRST), client->name);
    return;
  }

  client->nickname = nick;

  if(strncmp(nick->pass, servcrypt(parv[1], nick->pass), sizeof(nick->pass)) == 0)
  {
    reply_user(service, client, _N(client, NS_IDENTIFIED), client->name);
    identify_user(client);
  }
  else
  {
    reply_user(service, client, _N(client, NS_IDENT_FAIL), client->name);
  }
}
