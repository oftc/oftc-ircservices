#include "stdinc.h"

static struct Service *nickserv = NULL;

static void m_register(struct Service *, struct Client *, int, char *[]);
static void m_identify(struct Service *, struct Client *, int, char *[]);
static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_set(struct Service *, struct Client *, int, char *[]);

static void m_set_language(struct Service *, struct Client *, int, char *[]);
static void m_set_password(struct Service *, struct Client *, int, char *[]);

static struct ServiceMessage register_msgtab = {
  NULL, "REGISTER", 0, 2, NS_HELP_REG_SHORT, NS_HELP_REG_LONG,
  { m_register, m_alreadyreg, m_alreadyreg, m_alreadyreg }
};

static struct ServiceMessage identify_msgtab = {
  NULL, "IDENTIFY", 0, 1, NS_HELP_ID_SHORT, NS_HELP_ID_LONG,
  { m_identify, m_identify, m_identify, m_identify }
};

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, NS_HELP_SHORT, NS_HELP_LONG,
  { m_help, m_help, m_help, m_help }
};

static struct SubMessage set_sub[3] = {
  { "LANGUAGE", 0, 0, -1, -1, m_set_language },
  { "PASSWORD", 0, 2, -1, -1, m_set_password },
  { "NULL", 0, 0, 0, 0, NULL }
};

static struct ServiceMessage set_msgtab = {
  set_sub, "SET",  0, 0, NS_HELP_SHORT, NS_HELP_LONG,
  { m_unreg, m_set, m_set, m_set }
};

INIT_MODULE(nickserv, "$Revision: 470 $")
{
  nickserv = make_service("NickServ");
  clear_serv_tree_parse(&nickserv->msg_tree);
  dlinkAdd(nickserv, &nickserv->node, &services_list);
  hash_add_service(nickserv);
  introduce_service(nickserv);
  load_language(nickserv, "nickserv.en");
  load_language(nickserv, "nickserv.rude");
  load_language(nickserv, "nickserv.de");

  mod_add_servcmd(&nickserv->msg_tree, &register_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &identify_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &help_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &set_msgtab);
}

CLEANUP_MODULE
{
  exit_client(find_client(nickserv->name), &me, "Service unloaded");
  hash_del_service(nickserv);
  dlinkDelete(&nickserv->node, &services_list);
}

static void
identify_user(struct Client *client)
{
  if(IsOper(client) && IsServAdmin(client))
  {
    client->service_handler = ADMIN_HANDLER;
    printf("Admin\n");
  }
  else if(IsOper(client))
  {
    printf("Oper\n");
    client->service_handler = OPER_HANDLER;
  }
  else
  {
    printf("Normal\n");
    client->service_handler = REG_HANDLER;
  }

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
    reply_user(service, client, _L(nickserv, client, NS_ALREADY_REG), client->name); 
    return;
  }

  nick = MyMalloc(sizeof(struct Nick));
  strlcpy(nick->nick, client->name, sizeof(nick->nick));
  strlcpy(nick->pass, crypt_pass(parv[1]), sizeof(nick->pass));

  client->nickname = nick;
  if(db_register_nick(client, parv[2]) >= 0)
  {
    identify_user(client);
    reply_user(service, client, _L(nickserv, client, NS_REG_COMPLETE), client->name);
    global_notice(NULL, "%s!%s@%s registered nick %s\n", client->name, 
        client->username, client->host, nick->nick);

    return;
  }
  reply_user(service, client, _L(nickserv, client, NS_REG_FAIL), client->name);
}

static void
m_identify(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  struct Nick *nick;

  if((nick = db_find_nick(client->name)) == NULL)
  {
    reply_user(service, client, _L(nickserv, client, NS_REG_FIRST), client->name);
    return;
  }

  if(strncmp(nick->pass, servcrypt(parv[1], nick->pass), sizeof(nick->pass)) == 0)
  {
    reply_user(service, client, _L(nickserv, client, NS_IDENTIFIED), client->name);
    identify_user(client);
    client->nickname = nick;
  }
  else
  {
    reply_user(service, client, _L(nickserv, client, NS_IDENT_FAIL), client->name);
  }
}

static void
m_help(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  do_help(service, client, parv[1], parc, parv);
}

static void
m_set(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  reply_user(service, client, "Unknown SET option");
}

static void
m_set_password(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  struct Nick *nick;
  char cryptpass[255]; // XXX
  
  if (client->nickname == NULL) {
    reply_user(service, client, _L(nickserv, client, NS_REG_FIRST), client->name);
    return;
  }

  nick = client->nickname;

  if(strncmp(nick->pass, servcrypt(parv[1], nick->pass), sizeof(nick->pass)) != 0)
  {
    reply_user(service, client, _L(nickserv, client, NS_SET_FAILED));
    return;
  }

  strlcpy(cryptpass, crypt_pass(parv[2]), sizeof(cryptpass));
  if(db_set_password(client, cryptpass) >= 0)
  {
    reply_user(service, client, _L(nickserv, client, NS_SET_SUCCESS));
  }
  else
  {
    reply_user(service, client, _L(nickserv, client, NS_SET_FAILED));
  }
}

static void
m_set_language(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  /* No paramters, list languages */
  if(parc == 0)
  {
    int i;

    reply_user(service, client, _L(nickserv, client, NS_CURR_LANGUAGE),
        service->language_table[client->nickname->language][0],
        client->nickname->language);

    reply_user(service, client, _L(nickserv, client, NS_AVAIL_LANGUAGE));

    for(i = 0; i < LANG_LAST; i++)
    {
      reply_user(service, client, "%d: %s", i, service->language_table[i][0]);
    }
  }
  else
  {
    int lang = atoi(parv[1]);
    
    db_set_language(client, lang);
    reply_user(service, client, _L(nickserv, client, NS_LANGUAGE_SET),
        service->language_table[lang][0], lang); 
  }
}
