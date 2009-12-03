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
#include "client.h"
#include "nickname.h"
#include "dbchannel.h"
#include "dbm.h"
#include "language.h"
#include "parse.h"
#include "msg.h"
#include "interface.h"
#include "conf/modules.h"
#include "conf/servicesinfo.h"
#include "operserv.h"
#include "jupe.h"
#include "akill.h"
#include "send.h"
#include "hash.h"
#include "servicemask.h"

static struct Service *operserv = NULL;

static dlink_node *os_newuser_hook;
static dlink_node *os_burst_done_hook;
static dlink_node *os_quit_hook;

static void *os_on_newuser(va_list);
static void *os_on_burst_done(va_list);
static void *os_on_quit(va_list);

static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_raw(struct Service *, struct Client *, int, char *[]);
static void m_mod_list(struct Service *, struct Client *, int, char *[]);
static void m_mod_load(struct Service *, struct Client *, int, char *[]);
static void m_mod_reload(struct Service *, struct Client *, int, char *[]);
static void m_mod_unload(struct Service *, struct Client *, int, char *[]);
static void m_operserv_notimp(struct Service *, struct Client *, int, char *[]);
static void m_admin_add(struct Service *, struct Client *, int, char *[]);
static void m_admin_list(struct Service *, struct Client *, int, char *[]);
static void m_admin_del(struct Service *, struct Client *, int, char *[]);
static void m_akill_add(struct Service *, struct Client *, int, char *[]);
static void m_akill_list(struct Service *, struct Client *, int, char *[]);
static void m_akill_del(struct Service *, struct Client *, int, char *[]);
static void m_jupe_add(struct Service *, struct Client *, int, char *[]);
static void m_jupe_list(struct Service *, struct Client *, int, char *[]);
static void m_jupe_del(struct Service *, struct Client *, int, char *[]);

static void expire_akills(void *);
static void check_akills(void *);

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, 2, 0, OPER_FLAG, OS_HELP_SHORT, OS_HELP_LONG, m_help
};

static struct ServiceMessage mod_subs[] = {
  { NULL, "LIST", 0, 0, 0, 0, ADMIN_FLAG, OS_MOD_LIST_HELP_SHORT, 
    OS_MOD_LIST_HELP_LONG, m_mod_list },
  { NULL, "LOAD", 0, 1, 1, 0, ADMIN_FLAG, OS_MOD_LOAD_HELP_SHORT, 
    OS_MOD_LOAD_HELP_LONG, m_mod_load },
  { NULL, "RELOAD", 0, 1, 1, 0, ADMIN_FLAG, OS_MOD_RELOAD_HELP_SHORT, 
    OS_MOD_RELOAD_HELP_LONG, m_mod_reload },
  { NULL, "UNLOAD", 0, 1, 1, 0, ADMIN_FLAG, OS_MOD_UNLOAD_HELP_SHORT, 
    OS_MOD_UNLOAD_HELP_LONG, m_mod_unload },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage mod_msgtab = {
  mod_subs, "MOD", 0, 1, 1, 0, ADMIN_FLAG, OS_MOD_HELP_SHORT, OS_MOD_HELP_LONG,
  NULL
};

static struct ServiceMessage raw_msgtab = {
  NULL, "RAW", 1, 1, 1, SFLG_NOMAXPARAM, ADMIN_FLAG, OS_RAW_HELP_SHORT, OS_RAW_HELP_LONG, m_raw
};

static struct ServiceMessage admin_subs[] = {
  { NULL, "ADD", 0, 1, 1, 0, ADMIN_FLAG, OS_ADMIN_ADD_HELP_SHORT, 
    OS_ADMIN_ADD_HELP_LONG, m_admin_add },
  { NULL, "LIST", 0, 0, 0, 0, ADMIN_FLAG, OS_ADMIN_LIST_HELP_SHORT, 
    OS_ADMIN_LIST_HELP_LONG, m_admin_list },
  { NULL, "DEL", 0, 1, 1, 0, ADMIN_FLAG, OS_ADMIN_DEL_HELP_SHORT, 
    OS_ADMIN_DEL_HELP_LONG, m_admin_del },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage admin_msgtab = {
  admin_subs, "ADMIN", 0, 1, 1, 0, ADMIN_FLAG, OS_ADMIN_HELP_SHORT, 
  OS_ADMIN_HELP_LONG, NULL
};

static struct ServiceMessage akill_subs[] = {
  { NULL, "ADD", 0, 2, 3, SFLG_NOMAXPARAM, OPER_FLAG, OS_AKILL_ADD_HELP_SHORT, 
    OS_AKILL_ADD_HELP_LONG, m_akill_add },
  { NULL, "LIST", 0, 0, 0, 0, OPER_FLAG, OS_AKILL_LIST_HELP_SHORT, 
    OS_AKILL_LIST_HELP_LONG, m_akill_list },
  { NULL, "DEL", 0, 1, 1, 0, OPER_FLAG, OS_AKILL_DEL_HELP_SHORT, 
    OS_AKILL_DEL_HELP_LONG, m_akill_del },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage akill_msgtab = {
  akill_subs, "AKILL", 0, 2, 2, 0, OPER_FLAG, OS_AKILL_HELP_SHORT, 
  OS_AKILL_HELP_LONG, NULL
};

static struct ServiceMessage set_msgtab = {
  NULL, "SET", 1, 1, 1, 0, ADMIN_FLAG, OS_SET_HELP_SHORT, OS_SET_HELP_LONG,
  m_operserv_notimp
};

static struct ServiceMessage jupe_subs[] = {
  { NULL, "ADD", 0, 1, 3, SFLG_NOMAXPARAM, OPER_FLAG, OS_JUPE_ADD_HELP_SHORT,
    OS_JUPE_ADD_HELP_LONG, m_jupe_add },
  { NULL, "LIST", 0, 0, 0, 0, OPER_FLAG, OS_JUPE_LIST_HELP_SHORT,
    OS_JUPE_LIST_HELP_LONG, m_jupe_list },
  { NULL, "DEL", 0, 1, 1, 0, OPER_FLAG, OS_JUPE_DEL_HELP_SHORT,
    OS_JUPE_DEL_HELP_LONG, m_jupe_del },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage jupe_msgtab = {
  jupe_subs, "JUPE", 0, 2, 2, 0, OPER_FLAG, OS_JUPE_HELP_SHORT,
  OS_JUPE_HELP_LONG, NULL
};

INIT_MODULE(operserv, "$Revision$")
{
  operserv = make_service("OperServ");
  clear_serv_tree_parse(&operserv->msg_tree);
  dlinkAdd(operserv, &operserv->node, &services_list);
  hash_add_service(operserv);
  introduce_client(operserv->name, operserv->name, TRUE);

  load_language(operserv->languages, "operserv.en");

  os_newuser_hook = install_hook(on_newuser_cb, os_on_newuser);
  os_burst_done_hook = install_hook(on_burst_done_cb, os_on_burst_done);
  os_quit_hook = install_hook(on_quit_cb, os_on_quit);

  mod_add_servcmd(&operserv->msg_tree, &help_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &mod_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &raw_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &admin_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &akill_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &set_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &raw_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &jupe_msgtab);

  eventAdd("Expire akills", expire_akills, NULL, 60);
  eventAdd("Check akills", check_akills, NULL, OPERSERV_AKILL_CHECK_TIME);

  return operserv;
}

CLEANUP_MODULE
{
  uninstall_hook(on_newuser_cb, os_on_newuser);
  uninstall_hook(on_burst_done_cb, os_on_burst_done);
  uninstall_hook(on_quit_cb, os_on_quit);

  serv_clear_messages(operserv);

  eventDelete(expire_akills, NULL);
  eventDelete(check_akills, NULL);

  unload_languages(operserv->languages);
  ilog(L_DEBUG, "Unloaded operserv");
}

static void *
os_on_burst_done(va_list param)
{
  struct JupeEntry *jupe;
  struct Client *target;
  int ret;
  dlink_list list;
  dlink_node *ptr;

  jupe_list(&list);

  DLINK_FOREACH(ptr, list.head)
  {
    jupe = (struct JupeEntry *)ptr->data;
    if((target = find_client(jupe->name)) != NULL && IsServer(target))
    {
      ilog(L_DEBUG, "JUPE Server %s already exists, removing jupe", jupe->name);
      ret = jupe_delete(jupe->name);
      if(ret <= 0)
        ilog(L_INFO, "Failed to remove existing jupe for existing server %s", 
            jupe->name);
    }
    else
    {
      introduce_server(jupe->name, jupe->reason);
      ilog(L_DEBUG, "JUPE %s [%s]", jupe->name, jupe->reason);
    }
  }

  free_jupe_list(&list);

  return pass_callback(os_burst_done_hook, param);
}

static void
m_help(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  do_help(service, client, parv[1], parc, parv);
}

static void
m_mod_list(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  dlink_node *ptr;

  DLINK_FOREACH(ptr, loaded_modules.head)
  {
    struct Module *mod = ptr->data;

    reply_user(service, service, client, OS_MOD_LIST, mod->name, mod->version,
        mod->type == MODTYPE_RUBY ? "Ruby" : "so");
  }

  reply_user(service, service, client, OS_MOD_LIST_END);
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
    reply_user(service, service, client, OS_MOD_ALREADYLOADED, parm);
    return;
  }

  if (parm == NULL)
  {
    reply_user(service, service, client, 0, "You need to specify the modules name");
    return;
  }

  ilog(L_NOTICE, "Loading %s by request of %s",
      parm, client->name);
  if (load_module(parm) != NULL)
  {
    ilog(L_NOTICE, "Module %s loaded", parm);
    reply_user(service, service, client, OS_MOD_LOADED, parm);
  }
  else
  {
    ilog(L_NOTICE, "Module %s could not be loaded!", parm);
    reply_user(service, service, client, OS_MOD_LOADFAIL, parm);
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
    ilog(L_NOTICE, "Module %s reload requested by %s, but failed because not loaded",
        parm, client->name);
    reply_user(service, service, client, OS_MOD_NOTLOADED, parm, client->name);
    return;
  }
  if(irccmp(module->name, service->name) == 0)
  {
    ilog(L_NOTICE, "%s tried to reload %s.  Can't be done because it's me!",
        client->name, service->name);
    reply_user(service, service, client, OS_MOD_CANTRELOAD, parm);
    return;
  }
  ilog(L_NOTICE, "Reloading %s by request of %s", parm, client->name);
  reply_user(service, service, client, OS_MOD_RELOADING, parm, client->name);
  unload_module(module);
  if (load_module(parm) != NULL)
  {
    ilog(L_NOTICE, "Module %s loaded", parm);
    reply_user(service, service, client, OS_MOD_LOADED,parm);
  }
  else
  {
    ilog(L_NOTICE, "Module %s could not be loaded!", parm);
    reply_user(service, service, client, OS_MOD_LOADFAIL, parm);
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
    ilog(L_NOTICE, "Module %s unload requested by %s, but failed because not "
        "loaded", parm, client->name);
    reply_user(service, service, client, OS_MOD_UNLOAD_NOTLOADED, parm); 
    return;
  }
  ilog(L_NOTICE, "Unloading %s by request of %s", parm, client->name);
  reply_user(service, service, client, OS_MOD_UNLOAD, parm, client->name);
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
  ilog(L_NOTICE, "%s Executed RAW: \"%s\"", client->name, buffer);
}

static void
m_admin_add(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  Nickname *nick = nickname_find(parv[1]);
  struct Client *target;

  if(nick == NULL)
  {
    reply_user(service, service, client, OS_NICK_NOTREG, parv[1]);
    return;
  }
  nickname_set_admin(nick, TRUE);
  reply_user(service, service, client, OS_ADMIN_ADDED, nickname_get_nick(nick));
  nickname_free(nick);

  /* Actively enforce the admin add in case the nick is online right now */
  if((target = find_client(parv[1])) != NULL)
  {
    if(target->nickname != NULL)
    {
      nickname_set_admin(target->nickname, TRUE);
      if(IsOper(target))
        target->access = ADMIN_FLAG;
      else
        target->access = IDENTIFIED_FLAG;
    }
  }
}

static void
m_admin_list(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  char *currnick;
  int i = 1;
  dlink_node *ptr;
  dlink_list list = { 0 };

  nickname_list_admins(&list);
  DLINK_FOREACH(ptr, list.head)
  {
    currnick = (char *)ptr->data;
    reply_user(service, service, client, OS_ADMIN_LIST, i++, currnick);
  }
  nickname_list_admins_free(&list);

  reply_user(service, service, client, OS_ADMIN_LIST_END);
}

static void
m_admin_del(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  Nickname *nick;
  struct Client *target;
    
  nick = nickname_find(parv[1]);
    
  if(nick == NULL || !nickname_get_admin(nick))
  {
    reply_user(service, service, client, OS_ADMIN_NOTADMIN, parv[1]);
    return;
  }
  reply_user(service, service, client, OS_ADMIN_DEL, nickname_get_nick(nick));
  nickname_set_admin(nick, FALSE);

  nickname_free(nick);

  /* Actively enforce the admin removal in case the nick is online right now */
  if((target = find_client(parv[1])) != NULL)
  {
    if(target->nickname != NULL)
    {
      nickname_set_admin(target->nickname, FALSE);
      if(IsOper(target))
        target->access = OPER_FLAG;
      else
        target->access = IDENTIFIED_FLAG;
    }
  }
}

/* AKILL ADD [+duration] user@host reason */
static void
m_akill_add(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct ServiceMask *akill, *tmp;
  char reason[IRC_BUFSIZE+1] = "\0";
  char mask_buf[IRC_BUFSIZE+1] = "\0";
  int para_start = 2;
  char *mask = parv[1];
  char duration_char = '\0';
  int duration = -1;
  int input_dur;

  if(*parv[1] == '+')
  {
    char *ptr = parv[1];

    para_start = 3;
    mask = parv[2];
    parc--;

    ptr++;
    while(*ptr != '\0')
    {
      if(!IsDigit(*ptr))
      {
        duration_char = *ptr;
        *ptr = '\0';
        duration = atoi(parv[1]);
        input_dur = duration;
        break;
      }
      ptr++;
    }
  }

  if(duration != -1)
  {
    switch(duration_char)
    {
      case 'm':
        duration *= 60;
        break;
      case 'h':
        duration *= 3600;
        break;
      case 'd':
      case '\0': /* default is days */
        duration *= 86400; 
        duration_char = 'd';
        break;
      default:
        reply_user(service, service, client, OS_AKILL_BAD_DURATIONCHAR, 
            duration_char);
        return;
    }
  }
  else 
  {
    duration_char = 'd';
    input_dur = duration = ServicesInfo.def_akill_dur;
    input_dur /= 86400;
  }

  if(duration == -1)
    duration = 0;

  if(strchr(mask, '@') == NULL)
    snprintf(mask_buf, sizeof(mask_buf), "*@%s", mask);
  else
    strlcpy(mask_buf, mask, sizeof(mask_buf));

  if((tmp = akill_find(mask_buf)) != NULL)
  {
    reply_user(service, service, client, OS_AKILL_ALREADY, mask_buf);
    free_servicemask(tmp);
    return;
  }

  if(!valid_wild_card(mask_buf))
  {
    reply_user(service, service, client, OS_AKILL_TOO_WILD, 
        ServicesInfo.min_nonwildcard);
    return;
  }

  join_params(reason, parc-1, &parv[para_start]);

  akill = MyMalloc(sizeof(struct ServiceMask));

  akill->setter = nickname_get_id(client->nickname);
  akill->time_set = CurrentTime;
  akill->duration = duration;
  DupString(akill->mask, mask_buf);
  DupString(akill->reason, reason);

  if(!akill_add(akill))
  {
    reply_user(service, service, client, OS_AKILL_ADDFAIL, mask_buf);
    free_servicemask(akill);
    return;
  }

  ilog(L_NOTICE, "%s Added an akill on %s. Expires %s [%s]", client->name,
    akill->mask, smalldate(akill->time_set + duration), reason);
  send_akill(service, client->name, akill);
  reply_user(service, service, client, OS_AKILL_ADDOK, mask_buf);
  free_servicemask(akill);
}

static void
m_akill_list(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct ServiceMask *akill;
  char *set, *expire;
  int i = 1;
  dlink_node *ptr;
  dlink_list list = { 0 };

  akill_list(&list);

  DLINK_FOREACH(ptr, list.head)
  {
    char *setter;

    akill = (struct ServiceMask *)ptr->data;
    setter = nickname_nick_from_id(akill->setter, TRUE);

    DupString(set, smalldate(akill->time_set));
    DupString(expire, smalldate(akill->time_set+akill->duration));

    reply_user(service, service, client, OS_AKILL_LIST, i++, akill->mask,
        akill->reason, setter, set, akill->duration == 0 ? "N/A" : expire);

    MyFree(setter);
    MyFree(expire);
    MyFree(set);
  }

  akill_list_free(&list);

  reply_user(service, service, client, OS_AKILL_LIST_END);
}

static void
m_akill_del(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  int ret = 0;
  struct ServiceMask *akill;
  char *expire_time, *set_time;

  if((akill = akill_find(parv[1])) == NULL)
  {
    reply_user(service, service, client, OS_AKILL_DEL, 0);
    return;
  }
  
  ret = akill_remove_mask(parv[1]);
  remove_akill(service, akill);
  if(ret)
  {
    /* XXX Have to copy the string since smalldate uses a static buffer! */
    DupString(set_time, smalldate(akill->time_set));
    if(akill->duration > 0)
      DupString(expire_time, smalldate(akill->time_set + akill->duration));
    else
      expire_time = "never";

    ilog(L_NOTICE, "%s removed akill on %s.  Akill was set %s and would have "
        "expired %s. (%s)", client->name, akill->mask, set_time, expire_time,
        akill->reason);

    if(akill->duration > 0)
      MyFree(expire_time);
    MyFree(set_time);
  }
  free_servicemask(akill);
  reply_user(service, service, client, OS_AKILL_DEL, ret);
}

static void m_operserv_notimp(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  reply_user(service, service, client, 0, "This isnt implemented yet.");
}

static void *
os_on_newuser(va_list args)
{
  struct Client *newuser = va_arg(args, struct Client*);
  
  if(IsMe(newuser->from))
    return pass_callback(os_newuser_hook, newuser);

  dlinkAdd(newuser, make_dlink_node(), &delay_akill_list);

  return pass_callback(os_newuser_hook, newuser);
}

static void
m_jupe_add(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Client *target = find_client(parv[1]);
  struct JupeEntry *entry = jupe_find(parv[1]);
  char reason[IRC_BUFSIZE+1] = "Jupitered: No Reason";
  int ret = 0;

  if(entry != NULL)
  {
    reply_user(service, service, client, OS_JUPE_ALREADY, entry->name, entry->reason);
    free_jupeentry(entry);
    return;
  }

  if(target != NULL && IsServer(target))
  {
    reply_user(service, service, client, OS_JUPE_SERVER_EXISTS, parv[1]);
    return;
  }

  if(parc >= 2)
    join_params(reason, parc-1, &parv[2]);

  entry = MyMalloc(sizeof(struct JupeEntry));

  DupString(entry->name, parv[1]);
  DupString(entry->reason, reason);

  entry->setter = nickname_get_id(client->nickname);

  ret = jupe_add(entry);

  if(ret)
  {
    introduce_server(entry->name, entry->reason);
    reply_user(service, service, client, OS_JUPE_ADDED, entry->name, entry->reason);
    ilog(L_NOTICE, "%s Jupitered %s [%s]", client->name, entry->name, entry->reason);
  }
  else
  {
    reply_user(service, service, client, OS_JUPE_ADD_FAILED, entry->name);
    ilog(L_NOTICE, "%s Failed to Jupiter %s", client->name, entry->name);
  }

  free_jupeentry(entry);
}

static void
m_jupe_list(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct JupeEntry *jupe;
  dlink_node *ptr;
  dlink_list list = { 0 };

  int i = 1;

  jupe_list(&list);

  DLINK_FOREACH(ptr, list.head)
  {
    jupe = (struct JupeEntry *)ptr->data;
    char *setter = nickname_nick_from_id(jupe->setter, TRUE);

    reply_user(service, service, client, OS_JUPE_LIST, i++, jupe->name,
      jupe->reason, setter);

    MyFree(setter);
  }

  free_jupe_list(&list);

  reply_user(service, service, client, OS_JUPE_LIST_END);
}

static void
m_jupe_del(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Client *target = find_client(parv[1]);
  if(target != NULL && IsServer(target) && MyConnect(target))
  {
    squit_server(parv[1], "UnJupitered");
    reply_user(service, service, client, OS_JUPE_DELETED, parv[1]);
  }
  else
  {
    reply_user(service, service, client, OS_JUPE_DEL_FAILED, parv[1]);
  }
}

static void *
os_on_quit(va_list param)
{
  struct Client *server = va_arg(param, struct Client *);
  char *reason          = va_arg(param, char *);
  int ret;

  if(IsServer(server) && IsMe(server->servptr))
  {
    ret = jupe_delete(server->name);

    if(ret > 0)
    {
      ilog(L_NOTICE, "Removed Jupiter for %s", server->name);
    }
    else
    {
      ilog(L_NOTICE, "Failed to remove Jupiter for %s", server->name);
    }
  }

  return pass_callback(os_quit_hook, server, reason);
}

static void
expire_akills(void *param)
{
  struct ServiceMask *akill;
  char *setter;
  dlink_list list = { 0 };
  dlink_node *ptr;

  akill_get_expired(&list);

  DLINK_FOREACH(ptr, list.head)
  {
    akill = (struct ServiceMask *)ptr->data;
    setter = nickname_nick_from_id(akill->setter, TRUE);
    ilog(L_NOTICE, "AKill Expired: %s set by %s on %s(%s)",
        akill->mask, setter, smalldate(akill->time_set), akill->reason);
    akill_remove_mask(akill->mask);
    MyFree(setter);
  }

  akill_list_free(&list);
}

static void
check_akills(void *param)
{
  dlink_node *ptr, *nptr;
  unsigned int checked = OPERSERV_AKILL_CHECK_CNT;

  DLINK_FOREACH_SAFE(ptr, nptr, delay_akill_list.head)
  {
    struct Client *target = (struct Client *)ptr->data;
    akill_check_client(operserv, target);
    dlinkDelete(ptr, &delay_akill_list);
    free_dlink_node(ptr);
    --checked;

    if(checked == 0)
      break;
  }
}
