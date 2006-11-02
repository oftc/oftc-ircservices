/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  nickserv.c: A C implementation of Nickname Services 
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  Some parts:
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id$
 */

#include "stdinc.h"

static struct Service *nickserv = NULL;

static dlink_node *ns_umode_hook;
static dlink_node *ns_nick_hook;

static void *s_umode(va_list);
static void *s_nick(va_list);
static void m_drop(struct Service *, struct Client *, int, char *[]);
static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_identify(struct Service *, struct Client *, int, char *[]);
static void m_register(struct Service *, struct Client *, int, char *[]);
static void m_set(struct Service *, struct Client *, int, char *[]);

static void m_set_language(struct Service *, struct Client *, int, char *[]);
static void m_set_password(struct Service *, struct Client *, int, char *[]);
static void m_set_url(struct Service *, struct Client *, int, char *[]);
static void m_set_email(struct Service *, struct Client *, int, char *[]);

static void cloak_user(struct Client *);


static struct ServiceMessage register_msgtab = {
  NULL, "REGISTER", 0, 2, NS_HELP_REG_SHORT, NS_HELP_REG_LONG,
  { m_register, m_alreadyreg, m_alreadyreg, m_alreadyreg }
};

static struct ServiceMessage identify_msgtab = {
  NULL, "IDENTIFY", 0, 1, NS_HELP_ID_SHORT, NS_HELP_ID_LONG,
  { m_identify, m_alreadyreg, m_alreadyreg, m_alreadyreg }
};

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, NS_HELP_SHORT, NS_HELP_LONG,
  { m_help, m_help, m_help, m_help }
};

static struct ServiceMessage drop_msgtab = {
  NULL, "DROP", 0, 0, NS_HELP_DROP_SHORT, NS_HELP_DROP_LONG,
  { m_unreg, m_drop, m_drop, m_drop }
};

static struct SubMessage set_sub[5] = {
  { "LANGUAGE", 0, 0, -1, -1, m_set_language },
  { "PASSWORD", 0, 2, -1, -1, m_set_password },
  { "URL"     , 0, 2, -1, -1, m_set_url},
  { "EMAIL"   , 0, 2, -1, -1, m_set_email},
  { "NULL"    , 0, 0, 0, 0, NULL }
};

static struct ServiceMessage set_msgtab = {
  set_sub, "SET",  0, 0, NS_HELP_SHORT, NS_HELP_LONG,
  { m_unreg, m_set, m_set, m_set }
};

INIT_MODULE(nickserv, "$Revision$")
{
  nickserv = make_service("NickServ");
  clear_serv_tree_parse(&nickserv->msg_tree);
  dlinkAdd(nickserv, &nickserv->node, &services_list);
  hash_add_service(nickserv);
  introduce_service(nickserv);
  load_language(nickserv, "nickserv.en");
  load_language(nickserv, "nickserv.rude");
  load_language(nickserv, "nickserv.de");

  mod_add_servcmd(&nickserv->msg_tree, &drop_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &identify_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &help_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &register_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &set_msgtab);
  
  ns_umode_hook = install_hook(umode_hook, s_umode);
  ns_nick_hook  = install_hook(nick_hook, s_nick);
}

CLEANUP_MODULE
{
  exit_client(find_client(nickserv->name), &me, "Service unloaded");
  hash_del_service(nickserv);
  dlinkDelete(&nickserv->node, &services_list);
}

/**
 * do all the necessary work to consider a client identified.
 * Note: at this point it has to already be verified that the client
 * is allowed to do so.
 */
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
cloak_user(struct Client *client_p)
{
  char *cloak;

  cloak = db_find_cloak(client_p->name);
  if (cloak != NULL)
  {
    svscloak_user(client, client_p->name, cloak);
  }

  MyFree(cloak);
}


/**
 * Someone wants to register with nickserv
 */
static void 
m_register(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Nick *nick;
  
  if((nick = db_find_nick(client->name)) != NULL)
  {
    reply_user(service, client, _L(nickserv, client, NS_ALREADY_REG), client->name); 
    MyFree(nick);
    return;
  }

  nick = db_register_nick(client->name, crypt_pass(parv[1]), parv[2]);
  if(nick != NULL)
  {
    client->nickname = nick;
    identify_user(client);
    reply_user(service, client, _L(nickserv, client, NS_REG_COMPLETE), client->name);
    global_notice(NULL, "%s!%s@%s registered nick %s\n", client->name, 
        client->username, client->host, nick->nick);

    return;
  }
  reply_user(service, client, _L(nickserv, client, NS_REG_FAIL), client->name);
}


/**
 * someone wants to drop a nick
 */
static void
m_drop(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  if (!IsRegistered(client)) 
  {
    reply_user(service, client, _L(nickserv, client, NS_NEED_IDENTIFY), client->name);
  }

  // XXX add some safetynet here XXX

  if (db_delete_nick(client->name) == 0) 
  {
    client->service_handler = UNREG_HANDLER;
    ClearRegistered(client);
    send_umode(nickserv, client, "-R");

    reply_user(service, client, _L(nickserv, client, NS_NICK_DROPPED), client->name);
    global_notice(NULL, "%s!%s@%s dropped nick %s\n", client->name, 
      client->username, client->host, client->name);
  } 
  else
  {
    global_notice(NULL, "Error: %s!%s@%s could not DROP nick %s\n", client->name, 
      client->username, client->host, client->name);
    reply_user(service, client, _L(nickserv, client, NS_NICK_DROPFAIL), client->name);
  }
}

/**
 * someone wants to identify with services
 */
static void
m_identify(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Nick *nick;

  if((nick = db_find_nick(client->name)) == NULL)
  {
    reply_user(service, client, _L(nickserv, client, NS_REG_FIRST), client->name);
    MyFree(nick);
    return;
  }

  if(strncmp(nick->pass, servcrypt(parv[1], nick->pass), sizeof(nick->pass)) == 0)
  {
    client->nickname = nick;
    reply_user(service, client, _L(nickserv, client, NS_IDENTIFIED), client->name);
    identify_user(client);
    cloak_user(client);
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

static void
m_set_url(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  char url[255];
  strlcpy(url, parv[1], 200);
    
  db_set_url(client, url);
  reply_user(service, client, _L(nickserv, client, NS_URL_SET), url);
}

static void
m_set_email(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  char email[255];
  strlcpy(email, parv[1], 200);
    
  db_set_email(client, email);
  reply_user(service, client, _L(nickserv, client, NS_EMAIL_SET), email);
}

static void*
s_umode(va_list args) 
{
  struct Client *client_p = va_arg(args, struct Client*);
  struct Client *source_p = va_arg(args, struct Client*);
  int            parc     = va_arg(args, int);
  char         **parv     = va_arg(args, char**);
    
  /// actually do stuff....
    
  // last function to call to pass the hook further to other hooks
  pass_callback(ns_umode_hook);
  return NULL;
}

static void *
s_nick(va_list args)
{
  struct Client *client_p = va_arg(args, struct Client*);
  struct Client *source_p = va_arg(args, struct Client*);
  int            parc     = va_arg(args, int);
  char         **parv     = va_arg(args, char**);
  int            newts    = va_arg(args, int);
  char          *nick     = va_arg(args, char*);
  char          *gecos    = va_arg(args, char*);

  if (IsRegistered(source_p) )
  {
    source_p->service_handler = UNREG_HANDLER;
    ClearRegistered(source_p);
    send_umode(nickserv, source_p, "-R");
  }
  pass_callback(ns_nick_hook);
  return NULL;
}

