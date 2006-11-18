/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  operserv.c: A C implementation of Operator Services
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

static struct Service *operserv = NULL;

static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_raw(struct Service *, struct Client *, int, char *[]);
static void m_mod(struct Service *, struct Client *, int, char *[]);
static void m_mod_list(struct Service *, struct Client *, int, char *[]);
static void m_mod_load(struct Service *, struct Client *, int, char *[]);
static void m_mod_reload(struct Service *, struct Client *, int, char *[]);
static void m_mod_unload(struct Service *, struct Client *, int, char *[]);
static void m_operserv_notimp(struct Service *, struct Client *, int, char *[]);
static void m_admin(struct Service *, struct Client *, int, char *[]);
static void m_admin_add(struct Service *, struct Client *, int, char *[]);
static void m_admin_list(struct Service *, struct Client *, int, char *[]);
static void m_admin_del(struct Service *, struct Client *, int, char *[]);

// FIXME wrong type of clients may execute
//

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, OS_HELP_SHORT, OS_HELP_LONG,
  { m_help, m_help, m_help, m_help}
};

static struct SubMessage mod_subs[5] = {
  { "LIST", 0, 0, OS_MOD_LIST_HELP_SHORT, OS_MOD_LIST_HELP_LONG, m_mod_list },
  { "LOAD", 0, 1, OS_MOD_LOAD_HELP_SHORT, OS_MOD_LOAD_HELP_LONG, m_mod_load },
  { "RELOAD", 0, 1, OS_MOD_RELOAD_HELP_SHORT, OS_MOD_RELOAD_HELP_LONG, m_mod_reload },
  { "UNLOAD", 0, 1, OS_MOD_UNLOAD_HELP_SHORT, OS_MOD_UNLOAD_HELP_LONG, m_mod_unload },
  { NULL, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage mod_msgtab = {
  mod_subs, "MOD", 0, 1, OS_MOD_HELP_SHORT, OS_MOD_HELP_LONG,
  { m_mod, m_servignore, m_servignore, m_servignore }
};

static struct ServiceMessage raw_msgtab = {
  NULL, "RAW", 1, 1, OS_RAW_HELP_SHORT, OS_RAW_HELP_LONG,
  { m_raw, m_servignore, m_servignore, m_servignore }
};

static struct ServiceMessage global_msgtab = {
  NULL, "GLOBAL", 1, 1, OS_GLOBAL_HELP_SHORT, OS_GLOBAL_HELP_LONG,
  { m_operserv_notimp, m_operserv_notimp, m_operserv_notimp, m_operserv_notimp }
};

static struct SubMessage admin_subs[4] = {
  { "ADD", 0, 0, OS_ADMIN_ADD_HELP_SHORT, OS_ADMIN_ADD_HELP_LONG, m_admin_add },
  { "LIST", 0, 0, OS_ADMIN_LIST_HELP_SHORT, OS_ADMIN_LIST_HELP_LONG, m_admin_list },
  { "DEL", 0, 0, OS_ADMIN_DEL_HELP_SHORT, OS_ADMIN_DEL_HELP_LONG, m_admin_del }
};

static struct ServiceMessage admin_msgtab = {
  admin_subs, "ADMIN", 1, 1, OS_ADMIN_HELP_SHORT, OS_ADMIN_HELP_LONG,
  { m_admin, m_admin, m_admin, m_admin}
};

static struct ServiceMessage session_msgtab = {
  NULL, "SESSION", 1, 1, OS_SESSION_HELP_SHORT, OS_SESSION_HELP_LONG,
  { m_operserv_notimp, m_operserv_notimp, m_operserv_notimp, m_operserv_notimp }
};

static struct ServiceMessage akill_msgtab = {
  NULL, "AKILL", 1, 1, OS_AKILL_HELP_SHORT, OS_AKILL_HELP_LONG,
  { m_operserv_notimp, m_operserv_notimp, m_operserv_notimp, m_operserv_notimp }
};

static struct ServiceMessage exceptions_msgtab = {
  NULL, "EXCEPTIONS", 1, 1, OS_EXCEPTIONS_HELP_SHORT, OS_EXCEPTIONS_HELP_LONG,
  { m_operserv_notimp, m_operserv_notimp, m_operserv_notimp, m_operserv_notimp }
};

static struct ServiceMessage jupe_msgtab = {
  NULL, "JUPE", 1, 1, OS_JUPE_HELP_SHORT, OS_JUPE_HELP_LONG,
  { m_operserv_notimp, m_operserv_notimp, m_operserv_notimp, m_operserv_notimp }
};

static struct ServiceMessage set_msgtab = {
  NULL, "SET", 1, 1, OS_SET_HELP_SHORT, OS_SET_HELP_LONG,
  { m_operserv_notimp, m_operserv_notimp, m_operserv_notimp, m_operserv_notimp }
};

static struct ServiceMessage shutdown_msgtab = {
  NULL, "SHUTDOWN", 1, 1, OS_SHUTDOWN_HELP_SHORT, OS_SHUTDOWN_HELP_LONG,
  { m_operserv_notimp, m_operserv_notimp, m_operserv_notimp, m_operserv_notimp }
};

static struct ServiceMessage quarentine_msgtab = {
  NULL, "QUARENTINE", 1, 1, OS_QUARENTINE_HELP_SHORT, OS_QUARENTINE_HELP_LONG,
  { m_operserv_notimp, m_operserv_notimp, m_operserv_notimp, m_operserv_notimp }
};

INIT_MODULE(operserv, "$Revision$")
{
  operserv = make_service("OperServ");
  clear_serv_tree_parse(&operserv->msg_tree);
  dlinkAdd(operserv, &operserv->node, &services_list);
  hash_add_service(operserv);
  introduce_service(operserv);

  load_language(operserv, "operserv.en");

  mod_add_servcmd(&operserv->msg_tree, &help_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &mod_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &raw_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &global_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &admin_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &session_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &akill_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &exceptions_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &jupe_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &set_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &raw_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &shutdown_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &quarentine_msgtab);
}

CLEANUP_MODULE
{
}

static void
m_help(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  do_help(service, client, parv[1], parc, parv);
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
m_mod_reload(struct Service *service, struct Client *client,
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
        "Module %s reload requested by %s, but failed because not loaded",
        parm, client->name);
    reply_user(service, client,  
        "Module %s reload requested by %s, but failed because not loaded",
        parm, client->name);
    return;
  }
  global_notice(service, "Reloading %s by request of %s", parm, client->name);
  reply_user(service, client, "Reloading %s", parm, client->name);
  unload_module(module);
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
    return;
  }
  global_notice(service, "Unloading %s by request of %s", parm, client->name);
  reply_user(service, client, "Unloading %s", parm, client->name);
  unload_module(module);
}

static void
m_raw(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  char buffer[IRC_BUFSIZE+1];
  int i;

  memset(buffer, 0, sizeof(buffer));
  
  for(i = 1; i <= parc; i++)
  {
    strlcat(buffer, parv[i], sizeof(buffer));
    strlcat(buffer, " ", sizeof(buffer));
  }
  if(buffer[strlen(buffer)-1] == ' ')
    buffer[strlen(buffer)-1] = '\0';
  sendto_server(me.uplink, buffer);
  ilog(L_DEBUG, "Executing RAW: \"%s\"", buffer);
}

static void
m_admin(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  reply_user(service, client, "ADMIN, what?");
}

static void
m_admin_add(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Nick *nick = db_find_nick(parv[1]);

  if(nick == NULL)
  {
    reply_user(service, client, _L(service, client, OS_NICK_NOTREG), parv[1]);
    return;
  }
  nick->flags |= NS_FLAG_ADMIN;
  db_nick_set_number(nick->id, "flags", nick->flags);
  reply_user(service, client, _L(service, client, OS_ADMIN_ADDED), nick->nick);
  free_nick(nick);
}

static void
m_admin_list(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Nick *currnick;
  void *handle;
  int i = 0;

  handle = db_nick_list_flags_first(NS_FLAG_ADMIN, &currnick);
  while(currnick != NULL)
  {
    reply_user(service, client, _L(service, client, OS_ADMIN_LIST),
        i++, currnick->nick);
    free_nick(currnick);
    currnick = db_nick_list_flags_next(handle);
  }
  db_nick_list_flags_done(handle);
}

static void
m_admin_del(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  reply_user(service, client, "Del?");
}

static void m_operserv_notimp(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  reply_user(service, client, "This isnt implemented yet.");
}
