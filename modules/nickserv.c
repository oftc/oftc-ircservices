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
static dlink_node *ns_newuser_hook;

static void *ns_on_umode_change(va_list);
static void *ns_on_newuser(va_list);
static void *ns_on_nick_change(va_list);
static void m_drop(struct Service *, struct Client *, int, char *[]);
static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_identify(struct Service *, struct Client *, int, char *[]);
static void m_register(struct Service *, struct Client *, int, char *[]);
static void m_set(struct Service *, struct Client *, int, char *[]);
static void m_access(struct Service *, struct Client *, int, char *[]);
static void m_ghost(struct Service *, struct Client *, int, char *[]);

static void m_set_language(struct Service *, struct Client *, int, char *[]);
static void m_set_password(struct Service *, struct Client *, int, char *[]);
static void m_set_url(struct Service *, struct Client *, int, char *[]);
static void m_set_email(struct Service *, struct Client *, int, char *[]);
static void m_access_add(struct Service *, struct Client *, int, char *[]);
static void m_access_list(struct Service *, struct Client *, int, char *[]);
static void m_access_del(struct Service *, struct Client *, int, char *[]);

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
  { "PASSWORD", 0, 1, -1, -1, m_set_password },
  { "URL"     , 0, 0, -1, -1, m_set_url},
  { "EMAIL"   , 0, 0, -1, -1, m_set_email},
  { "NULL"    , 0, 0, 0, 0, NULL }
};

static struct ServiceMessage set_msgtab = {
  set_sub, "SET",  0, 0, NS_HELP_SHORT, NS_HELP_LONG,
  { m_unreg, m_set, m_set, m_set }
};

static struct SubMessage access_sub[4] = {
  { "ADD", 0, 1, -1, -1, m_access_add },
  { "LIST", 0, 0, -1, -1, m_access_list },
  { "DEL", 0, 0, -1, -1, m_access_del },
  { "NULL", 0, 0, 0, 0, NULL }
};

static struct ServiceMessage access_msgtab = {
  access_sub, "ACCESS", 0, 0, NS_HELP_SHORT, NS_HELP_LONG,
  { m_unreg, m_access, m_access, m_access }
};

static struct ServiceMessage ghost_msgtab = {
  NULL, "GHOST", 0, 2, NS_HELP_GHOST_SHORT, NS_HELP_GHOST_LONG,
  { m_ghost, m_ghost, m_ghost, m_ghost }
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
  mod_add_servcmd(&nickserv->msg_tree, &access_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &ghost_msgtab);
  
  ns_umode_hook       = install_hook(on_umode_change_cb, ns_on_umode_change);
  ns_nick_hook        = install_hook(on_nick_change_cb, ns_on_nick_change);
  ns_newuser_hook     = install_hook(on_newuser_cb, ns_on_newuser);
}

CLEANUP_MODULE
{
  exit_client(find_client(nickserv->name), &me, "Service unloaded");
  hash_del_service(nickserv);
  dlinkDelete(&nickserv->node, &services_list);
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
    reply_user(service, client, _L(nickserv, client, NS_ALREADY_REG), 
        client->name); 
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
  if (!IsIdentified(client)) 
  {
    reply_user(service, client, _L(nickserv, client, NS_NEED_IDENTIFY), client->name);
  }

  // XXX add some safetynet here XXX

  if (db_delete_nick(client->name) == 0) 
  {
    client->service_handler = UNREG_HANDLER;
    ClearIdentified(client);
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
    reply_user(service, client, _L(nickserv, client, NS_REG_FIRST), 
        client->name);
    return;
  }

  if(strncmp(nick->pass, servcrypt(parv[1], nick->pass), 
    sizeof(nick->pass)) != 0)
  {
    MyFree(nick);
    reply_user(service, client, _L(nickserv, client, NS_IDENT_FAIL), 
        client->name);
    return;
  }

  client->nickname = nick;

  identify_user(client);
  reply_user(service, client, _L(nickserv, client, NS_IDENTIFIED), client->name);
}

static void
m_help(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  do_help(service, client, parv[1], parc, parv);
}

static void
m_set(struct Service *service, struct Client *client, int parc, char *parv[])
{
  reply_user(service, client, "Unknown SET option");
}

static void
m_set_password(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  struct Nick *nick;
  
  nick = client->nickname;

  if(db_nick_set_string(client->nickname->id, "password", 
        crypt_pass(parv[1])) == 0)
  {
    reply_user(service, client, _L(nickserv, client, NS_SET_SUCCESS), 
        "PASSWORD", "hidden");
  }
  else
  {
    reply_user(service, client, _L(nickserv, client, NS_SET_FAILED),
        "PASSWORD", "hidden");
  }
  strlcpy(nick->pass, crypt_pass(parv[1]), sizeof(nick->pass));
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
    
    if(db_nick_set_number(client->nickname->id, "language", lang) == 0)
    {
      client->nickname->language = lang;
      reply_user(service, client, _L(nickserv, client, NS_LANGUAGE_SET),
          service->language_table[lang][0], lang); 
    }
    else
      reply_user(service, client, ":(");
  }
}

static void
m_set_url(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  struct Nick *nick = client->nickname;
  
  if(parc == 0)
  {
    reply_user(service, client, _L(nickserv, client, NS_SET_VALUE), 
        "URL", nick->url);
    return;
  }
  
  if(db_nick_set_string(nick->id, "url", parv[1]) == 0)
  {
    nick->url = replace_string(nick->url, parv[1]);
    reply_user(service, client, _L(nickserv, client, NS_SET_SUCCESS), "URL",
        nick->url);
  }
  else
    reply_user(service, client, _L(nickserv, client, NS_SET_FAILED), "URL",
        parv[1]);
}

static void
m_set_email(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  struct Nick *nick = client->nickname;
  
  if(parc == 0)
  {
    reply_user(service, client, _L(nickserv, client, NS_SET_VALUE), "EMAIL", 
        nick->email);
    return;
  }
    
  if(db_nick_set_string(nick->id, "email", parv[1]) == 0)
  {
    nick->email = replace_string(nick->email, parv[1]);
    reply_user(service, client, _L(nickserv, client, NS_SET_SUCCESS),
        "EMAIL", nick->email);
  }
  else
    reply_user(service, client, _L(nickserv, client, NS_SET_FAILED), 
        "EMAIL", parv[1]);
}

static void
m_set_cloak(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
/*
 * XXX
 * char *nick = parv[1];
  char *cloak = parv[2];

  struct Nick *nick_p;
  nick_p = db_find_nick(nick);

  if (nick_p != NULL) {
    if (db_set_cloak(nick_p, cloak) == 0) {
      reply_user(service, client, _L(operserv, client, OS_CLOAK_SET), cloak);
    } else {
      reply_user(service, client, _L(operserv, client, OS_CLOAK_FAILED), cloak, nick);
    }
    MyFree (nick_p);
  } else {
    reply_user(service, client, _L(operserv, client, OS_NICK_NOT_REG), nick);
  }*/
}

static void
m_access(struct Service *service, struct Client *client, int parc, char *parv[])
{
  reply_user(service, client, "Syntax: ACCESS {ADD | DEL | LIST} [mask | list]");
}

static void
m_access_add(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Nick *nick = client->nickname;

  if(strchr(parv[1], '@') == NULL)
  {
    reply_user(service, client, _L(nickserv, client, NS_ACCESS_INVALID), parv[1]);
    return;
  }
  
  if(db_list_add("nickname_access", nick->id, parv[1]) == 0)
  {
    reply_user(service, client, _L(nickserv, client, NS_ACCESS_ADD), parv[1]);
  }
  else
  {
    reply_user(service, client, _L(nickserv, client, NS_ACCESS_ADDFAIL),
        parv[1]);
  }
}

static void
m_access_list(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Nick *nick;
  struct AccessEntry entry;
  void *listptr;
  int i = 1;

  nick = client->nickname;

  reply_user(service, client, _L(nickserv, client, NS_ACCESS_START));
 
  if((listptr = db_list_first("nickname_access", nick->id, &entry)) == NULL)
  {
    return;
  }

  while(listptr != NULL)
  {
    reply_user(service, client, _L(nickserv, client, NS_ACCESS_ENTRY),
        i++, entry.value);
    MyFree(entry.value);
    listptr = db_list_next(listptr, &entry);
  }

  db_list_done(listptr);
}

static void
m_access_del(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Nick *nick = client->nickname;
  int index, ret;

  index = atoi(parv[1]);
  if(index > 0)
    ret = db_list_del_index("nickname_access", nick->id, index);
  else
    ret = db_list_del("nickname_access", nick->id, parv[1]);
  
  reply_user(service, client, _L(nickserv, client, NS_ACCESS_DEL), ret);
}

static void
m_ghost(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Nick *nick;

  if((nick = db_find_nick(parv[1])) == NULL)
  {
    reply_user(service, client, _L(nickserv, client, NS_REG_FIRST), parv[1]);
    return;
  }

  if(strncmp(client->name, parv[1], NICKLEN) == 0)
  {
    MyFree(nick);
    reply_user(service, client, _L(nickserv, client, NS_GHOST_NOSELF), parv[1]);
    return;
  }

  if(strncmp(nick->pass, servcrypt(parv[2], nick->pass),
        sizeof(nick->pass)) != 0)
  {
    MyFree(nick);
    reply_user(service, client, _L(nickserv, client, NS_GHOST_FAILED), parv[1]);   
    return;
  }

  reply_user(service, client, _L(nickserv, client, NS_GHOST_SUCCESS), parv[1]);
  /* XXX Turn this into send_kill */
  sendto_server(me.uplink, ":%s KILL %s :%s (GHOST Command recieved from %s)", 
      service->name, parv[1], "services.oftc.net", client->name);
}

static void*
ns_on_umode_change(va_list args) 
{
/*  struct Client *client_p = va_arg(args, struct Client*);
  struct Client *source_p = va_arg(args, struct Client*);
  int            parc     = va_arg(args, int);
  char         **parv     = va_arg(args, char**);
  */  
  /// actually do stuff....
    
  // last function to call to pass the hook further to other hooks
  return pass_callback(ns_umode_hook);
}

static void *
ns_on_nick_change(va_list args)
{
  struct Client *user = va_arg(args, struct Client *);
  char *oldnick       = va_arg(args, char *);
  struct Nick *nick_p;
  char userhost[USERHOSTLEN+1]; 

  printf("%s changing nick to %s\n", oldnick, user->name);
 
  if(IsIdentified(user))
  {
    /* XXX Check links */
    user->service_handler = UNREG_HANDLER;
    ClearIdentified(user);
    /* XXX Use unidentify event */
    send_umode(nickserv, user, "-R");
  }

  if((nick_p = db_find_nick(user->name)) == NULL)
  {
    printf("Nick Change: %s->%s(nick not registered)\n", oldnick, user->name);
    return pass_callback(ns_nick_hook);
  }

  snprintf(userhost, USERHOSTLEN, "%s@%s", user->username, user->host);
  if(check_list_entry("nickname_access", nick_p->id, userhost))
  {
    printf("%s changed nick to %s(found access entry)\n", oldnick, user->name);
    user->nickname = nick_p;
    identify_user(user);
  }
  else
    printf("%s changed nick to %s(no access entry)\n", oldnick, user->name);
  
  return pass_callback(ns_nick_hook);
}

static void *
ns_on_newuser(va_list args)
{
  struct Client *newuser = va_arg(args, struct Client*);
  struct Nick *nick_p;
  char userhost[USERHOSTLEN+1];
  
  printf("New User: %s!\n", newuser->name);

  if((nick_p = db_find_nick(newuser->name)) == NULL)
  {
    printf("New user: %s(nick not registered)\n", newuser->name);
    return pass_callback(ns_nick_hook);
  }

  snprintf(userhost, USERHOSTLEN, "%s@%s", newuser->username, 
      newuser->host);
  if(check_list_entry("nickname_access", nick_p->id, userhost))
  {
    printf("new user: %s(found access entry)\n", newuser->name);
    newuser->nickname = nick_p;
    identify_user(newuser);
  }
  else
    printf("new user:%s(no access entry)\n", newuser->name);
  
  return pass_callback(ns_nick_hook);
}
