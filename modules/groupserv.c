/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  groupserv.c: A C implementation of Group Services 
 *
 *  Copyright (C) 2009 Stuart Walsh and the OFTC Coding department
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
 *  $Id: groupserv.c 1556 2009-02-23 18:38:57Z tjfontaine $
 */

#include "stdinc.h"
#include "client.h"
#include "group.h"
#include "group.h"
#include "dbm.h"
#include "language.h"
#include "parse.h"
#include "msg.h"
#include "interface.h"
#include "channel_mode.h"
#include "channel.h"
#include "conf/modules.h"
#include "conf/servicesinfo.h"
#include "hash.h"
#include "groupserv.h"
#include "crypt.h"
#include "dbmail.h"
#include "groupaccess.h"

static struct Service *groupserv = NULL;
static struct Client *groupserv_client = NULL;

static void m_drop(struct Service *, struct Client *, int, char *[]);
static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_register(struct Service *, struct Client *, int, char *[]);
static void m_info(struct Service *,struct Client *, int, char *[]);
static void m_sudo(struct Service *, struct Client *, int, char *[]);
static void m_list(struct Service *, struct Client *, int, char *[]);

static void m_access_add(struct Service *, struct Client *, int, char *[]);
static void m_access_del(struct Service *, struct Client *, int, char *[]);
static void m_access_list(struct Service *, struct Client *, int, char *[]);

static void m_set_url(struct Service *, struct Client *, int, char *[]);
static void m_set_email(struct Service *, struct Client *, int, char *[]);
static void m_set_private(struct Service *, struct Client *, int, char *[]);

static int m_set_flag(struct Service *, struct Client *, char *, char *,
    unsigned char (*)(Group *), int (*)(Group *, unsigned char));
static int m_set_string(struct Service *, struct Client *, const char *,
    const char *, const char *, int, const char *(*)(Group *),
    int(*)(Group *, const char*));

static struct ServiceMessage register_msgtab = {
  NULL, "REGISTER", 0, 2, 2, SFLG_UNREGOK|SFLG_NOMAXPARAM, GRPIDENTIFIED_FLAG, 
  GS_HELP_REG_SHORT, GS_HELP_REG_LONG, m_register 
};

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, 2, SFLG_UNREGOK, GRPUSER_FLAG, GS_HELP_SHORT, 
  GS_HELP_LONG, m_help
};

static struct ServiceMessage drop_msgtab = {
  NULL, "DROP", 0, 1, 2, SFLG_GROUPARG, GRPMASTER_FLAG, GS_HELP_DROP_SHORT, 
  GS_HELP_DROP_LONG, m_drop
};

static struct ServiceMessage set_sub[] = {
  { NULL, "URL", 0, 1, 2, SFLG_KEEPARG|SFLG_GROUPARG, GRPMASTER_FLAG,
    GS_HELP_SET_URL_SHORT, GS_HELP_SET_URL_LONG, m_set_url },
  { NULL, "EMAIL", 0, 1, 2, SFLG_KEEPARG|SFLG_GROUPARG, GRPMASTER_FLAG,
    GS_HELP_SET_EMAIL_SHORT, GS_HELP_SET_EMAIL_LONG, m_set_email },
  { NULL, "PRIVATE", 0, 1, 2, SFLG_KEEPARG|SFLG_GROUPARG, GRPMASTER_FLAG,
    GS_HELP_SET_PRIVATE_SHORT, GS_HELP_SET_PRIVATE_LONG, m_set_private },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage set_msgtab = {
  set_sub, "SET", 0, 2, 2, SFLG_KEEPARG|SFLG_GROUPARG, GRPMASTER_FLAG,
  GS_HELP_SET_SHORT, GS_HELP_SET_LONG, NULL
};


static struct ServiceMessage access_sub[6] = {
  { NULL, "ADD", 0, 3, 3, SFLG_KEEPARG|SFLG_GROUPARG, GRPMASTER_FLAG,
    GS_HELP_ACCESS_ADD_SHORT, GS_HELP_ACCESS_ADD_LONG, m_access_add },
  { NULL, "DEL", 0, 2, 2, SFLG_KEEPARG|SFLG_GROUPARG, GRPMEMBER_FLAG,
    GS_HELP_ACCESS_DEL_SHORT, GS_HELP_ACCESS_DEL_LONG, m_access_del },
  { NULL, "LIST", 0, 1, 1, SFLG_KEEPARG|SFLG_GROUPARG, GRPUSER_FLAG,
    GS_HELP_ACCESS_LIST_SHORT, GS_HELP_ACCESS_LIST_LONG, m_access_list },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage access_msgtab = {
  access_sub, "ACCESS", 0, 2, 2, SFLG_KEEPARG|SFLG_GROUPARG, GRPMASTER_FLAG,
    GS_HELP_ACCESS_SHORT, GS_HELP_ACCESS_LONG, NULL
};

static struct ServiceMessage info_msgtab = {
  NULL, "INFO", 0, 1, 1, SFLG_GROUPARG, GRPUSER_FLAG, GS_HELP_INFO_SHORT, 
  GS_HELP_INFO_LONG, m_info
};

static struct ServiceMessage sudo_msgtab = {
  NULL, "SUDO", 0, 2, 2, SFLG_NOMAXPARAM, ADMIN_FLAG, GS_HELP_SUDO_SHORT,
  GS_HELP_SUDO_LONG, m_sudo
};

static struct ServiceMessage list_msgtab = {
  NULL, "LIST", 0, 1, 2, 0, USER_FLAG, GS_HELP_LIST_SHORT,
  GS_HELP_LIST_LONG, m_list
};

INIT_MODULE(groupserv, "$Revision: 1556 $")
{
  groupserv = make_service("GroupServ");
  clear_serv_tree_parse(&groupserv->msg_tree);
  dlinkAdd(groupserv, &groupserv->node, &services_list);
  hash_add_service(groupserv);
  groupserv_client = introduce_client(groupserv->name, groupserv->name, TRUE);
  load_language(groupserv->languages, "groupserv.en");

  mod_add_servcmd(&groupserv->msg_tree, &drop_msgtab);
  mod_add_servcmd(&groupserv->msg_tree, &help_msgtab);
  mod_add_servcmd(&groupserv->msg_tree, &register_msgtab);
  mod_add_servcmd(&groupserv->msg_tree, &set_msgtab);
  mod_add_servcmd(&groupserv->msg_tree, &sudo_msgtab);
  mod_add_servcmd(&groupserv->msg_tree, &list_msgtab);
  mod_add_servcmd(&groupserv->msg_tree, &info_msgtab);
  mod_add_servcmd(&groupserv->msg_tree, &access_msgtab);
  
  return groupserv;
}

CLEANUP_MODULE
{
  serv_clear_messages(groupserv);
  exit_client(groupserv_client, &me, "Service unloaded");
  unload_languages(groupserv->languages);
  hash_del_service(groupserv);
  ilog(L_DEBUG, "Unloaded groupserv");
}

static void 
m_register(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  Group *group;

  if(*parv[1] != '@')
  {
    reply_user(service, service, client, GS_NAMESTART_AT);
    return;
  }

  if((group = group_find(parv[1])) != NULL)
  {
    reply_user(service, service, client, GS_ALREADY_REG, parv[1]);
    group_free(group);
    return;
  }

  group = group_new();

  group_set_name(group, parv[1]);
  group_set_desc(group, parv[2]);

  if(group_register(group, client->nickname))
  {
    reply_user(service, service, client, GS_REG_COMPLETE, parv[1]);
    global_notice(NULL, "%s!%s@%s registered group %s\n", client->name, 
        client->username, client->host, group_get_name(group));

    execute_callback(on_group_reg_cb, client);
    return;
  }
  reply_user(service, service, client, GS_REG_FAIL, parv[1]);
}

static void
m_drop(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  Group *group;

  assert(parv[1]);

  group = group_find(parv[1]);

  if(group_delete(group))
  {
    reply_user(service, service, client, GS_DROPPED, parv[1]);
    ilog(L_NOTICE, "%s!%s@%s dropped group %s",
      client->name, client->username, client->host, parv[1]);

    group_free(group);
    group = NULL;
  }
  else
  {
    ilog(L_DEBUG, "GRoup DROP failed for %s on %s", client->name, parv[1]);
    reply_user(service, service, client, GS_DROP_FAILED, parv[1]);
  }

  if(group != NULL)
    group_free(group);
}

static void
m_help(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  do_help(service, client, parv[1], parc, parv);
}

static void
m_set_url(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  m_set_string(service, client, "URL", parv[1], parv[2], parc,
    &group_get_url, &group_set_url);
}

static void
m_set_email(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  m_set_string(service, client, "EMAIL", parv[1], parv[2], parc,
    &group_get_email, &group_set_email);
}

static void
m_set_private(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], "PRIVATE",
    &group_get_priv, &group_set_priv);
}

static void
m_info(struct Service *service, struct Client *client, int parc, char *parv[])
{
  Group *group;
  char buf[IRC_BUFSIZE+1] = {0};
  char *nick;
  struct GroupAccess *access = NULL;
  dlink_node *ptr;
  dlink_list list = { 0 };

  group = group_find(parv[1]);

  if(client->nickname != NULL)
  {
    access = groupaccess_find(group_get_id(group), 
        nickname_get_id(client->nickname));
  }

  reply_user(service, service, client, GS_INFO_START, group_get_name(group));
  reply_time(service, client, GS_INFO_REGTIME_FULL, group_get_regtime(group));

  if(IsOper(client) || (access != NULL && access->level >= GRPMEMBER_FLAG))
  {
    const char *ptr = group_get_desc(group);
    ptr = group_get_url(group);
    ptr = group_get_email(group);
    reply_user(service, service, client, GS_INFO,
        group_get_desc(group),
        group_get_url(group) == NULL ? "Not Set" : group_get_url(group),
        group_get_email(group) == NULL ? "Not Set" : group_get_email(group));
  }

  if(group_masters_list(group_get_id(group), &list))
  {
    int comma = 0;

    DLINK_FOREACH(ptr, list.head)
    {
      nick = (char *)ptr->data;
      if(comma)
        strlcat(buf, ", ", sizeof(buf));
      strlcat(buf, nick, sizeof(buf));
      if(!comma)
        comma = 1;
    }
    group_masters_list_free(&list);
  }

  reply_user(service, service, client, GS_INFO_MASTERS, buf);

  reply_user(service, service, client, GS_INFO_OPTION, "PRIVATE",
      group_get_priv(group) ? "ON" : "OFF");

  if(access != NULL)
    MyFree(access);

  group_free(group);
}

static void 
m_sudo(struct Service *service, struct Client *client, int parc, char *parv[])
{
#if 0
  Group *oldgroup, *group;
  char **newparv;
  char buf[IRC_BUFSIZE] = { '\0' };
  int oldaccess;

  oldgroup = client->groupname;
  oldaccess = client->access;

  group = group_find(parv[1]);
  if(group == NULL)
  {
    reply_user(service, service, client, GS_REG_FIRST, parv[1]);
    return;
  }

  client->groupname = group;
  if(group_get_admin(group))
    client->access = ADMIN_FLAG;
  else
    client->access = IDENTIFIED_FLAG;

  newparv = MyMalloc(4 * sizeof(char*));

  newparv[0] = parv[0];
  newparv[1] = service->name;

  join_params(buf, parc-1, &parv[2]);

  DupString(newparv[2], buf);

  ilog(L_INFO, "%s executed %s SUDO on %s: %s", client->name, service->name, 
      group_get_name(group), newparv[2]);

  process_privmsg(1, me.uplink, client, 3, newparv);
  MyFree(newparv[2]);
  MyFree(newparv);

  group_free(client->groupname);
  client->groupname = oldgroup;
  client->access = oldaccess;
#endif
}

static void
m_list(struct Service *service, struct Client *client, int parc, char *parv[])
{
#if 0
  char *group;
  int count = 0;
  int qcount = 0;
  dlink_node *ptr;
  dlink_list list = { 0 };

  if(parc == 2 && client->access >= OPER_FLAG)
  {
    if(irccmp(parv[2], "FORBID") == 0)
      qcount = group_list_forbid(&list);
    else
    {
      reply_user(service, service, client, GS_LIST_INVALID_OPTION, parv[2]);
      return;
    }
  }

  if(qcount == 0 && client->access >= OPER_FLAG)
    qcount = group_list_all(&list);
  else if(qcount == 0)
    qcount = group_list_regular(&list);

  if(qcount == 0)
  {
    reply_user(service, service, client, GS_LIST_NO_MATCHES, parv[1]);
    /* TODO XXX FIXME use proper free */
    db_string_list_free(&list);
    return;
  }

  DLINK_FOREACH(ptr, list.head)
  {
    group = (char *)ptr->data;
    if(match(parv[1], group))
    {
      count++;
      reply_user(service, service, client, GS_LIST_ENTRY, group);
    }
    if(count == 50)
      break;
  }

  /* TODO XXX FIXME use proper free */
  db_string_list_free(&list);

  reply_user(service, service, client, GS_LIST_END, count);
#endif
}

static int
m_set_flag(struct Service *service, struct Client *client,
           char *toggle, char *flagname,
           unsigned char (*get_func)(Group *),
           int (*set_func)(Group *, unsigned char))
{
#if 0
  Group *group = client->groupname;
  int on = FALSE;

  if(toggle == NULL)
  {
    on = get_func(group);
  unregister_callback(on_chan_reg_cb);
    reply_user(service, service, client, GS_SET_VALUE, flagname,
      on ? "ON" : "OFF");
    return TRUE;
  }

  if(strncasecmp(toggle, "ON", 2) == 0)
    on = TRUE;
  else if (strncasecmp(toggle, "OFF", 3) == 0)
    on = FALSE;
  else
  {
    reply_user(service, service, client, GS_SET_VALUE, flagname,
      on ? "ON" : "OFF");
    return TRUE;
  }

  if(set_func(group, on))
    reply_user(service, service, client, GS_SET_SUCCESS, flagname,
      on ? "ON" : "OFF");
  else
    reply_user(service, service, client, GS_SET_FAILED, flagname, on);

#endif
  return TRUE;
}

static int
m_set_string(struct Service * service, struct Client *client,
             const char *field, const char *group_name, const char *value,
             int parc, const char *(*get_func)(Group *),
             int(*set_func)(Group *, const char*))
{
  Group *group = group_find(group_name);
  int ret = FALSE;

  if(parc == 1)
  {
    const char *resp = get_func(group);
    reply_user(service, service, client, GS_SET_VALUE, field,
      resp == NULL ? "Not Set" : resp);
    group_free(group);
    return TRUE;
  }

  if(irccmp(value, "-") == 0)
    value = NULL;

  ret = set_func(group, value);

  reply_user(service, service, client, ret ? GS_SET_SUCCESS : GS_SET_FAILED, field,
      value == NULL ? "Not Set" : value);

  group_free(group);
  return ret;
}

static void
m_access_add(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  Group *group;
  struct GroupAccess *access, *oldaccess;
  unsigned int account, level;
  char *level_added = "MEMBER";

  group = group_find(parv[1]);

  if((account = nickname_id_from_nick(parv[2], TRUE)) <= 0)
  {
    reply_user(service, service, client, GS_REGISTER_NICK, parv[2]);
    group_free(group);
    return;
  }

  if(irccmp(parv[3], "MASTER") == 0)
  {
    level = GRPMASTER_FLAG;
    level_added = "MASTER";
  }
  else if(irccmp(parv[3], "MEMBER") == 0)
    level = GRPMEMBER_FLAG;
  else
  {
    reply_user(service, service, client, GS_ACCESS_BADLEVEL, parv[3]);
    group_free(group);
    return;
  }

  access = MyMalloc(sizeof(struct GroupAccess));
  access->group = group_get_id(group);
  access->account = account;
  access->level   = level;

  if((oldaccess = groupaccess_find(access->group, access->account)) != NULL)
  {
    int mcount = -1;

    if(oldaccess->account == access->account && oldaccess->level == access->level)
    {
      reply_user(service, service, client, GS_ACCESS_ALREADY_ON, parv[2],
          group_get_name(group));
      group_free(group);
      MyFree(oldaccess);
      MyFree(access);
      return;
    }
    group_masters_count(group_get_id(group), &mcount);
    if(oldaccess->level == GRPMASTER_FLAG && mcount <= 1)
    {
      reply_user(service, service, client, GS_ACCESS_NOMASTERS, parv[2],
          group_get_name(group));
      group_free(group);
      MyFree(oldaccess);
      MyFree(access);
      return;
    }
    groupaccess_remove(oldaccess);
    MyFree(oldaccess);
  }

  if(groupaccess_add(access))
  {
    reply_user(service, service, client, GS_ACCESS_ADDOK, parv[2], parv[1],
        level_added);
    ilog(L_DEBUG, "%s (%s@%s) added AE %s(%d) to %s", client->name,
        client->username, client->host, parv[2], access->level, parv[1]);
  }
  else
    reply_user(service, service, client, GS_ACCESS_ADDFAIL, parv[2], parv[1],
        parv[3]);

  group_free(group);

  MyFree(access);
}

static void
m_access_del(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  Group *group;
  struct GroupAccess *access, *myaccess;
  unsigned int nickid;
  int mcount = -1;

  group = group_find(parv[1]);

  if((nickid = nickname_id_from_nick(parv[2], TRUE)) <= 0)
  {
    reply_user(service, service, client, GS_ACCESS_NOTLISTED, parv[2], parv[1]);
    group_free(group);
    return;
  }

  access = groupaccess_find(group_get_id(group), nickid);
  if(access == NULL)
  {
    reply_user(service, service, client, GS_ACCESS_NOTLISTED, parv[2], parv[1]);
    group_free(group);
    return;
  }

  if(nickid != nickname_get_id(client->nickname) && client->access != SUDO_FLAG)
  {
    if(client->access != SUDO_FLAG)
    {
      myaccess = groupaccess_find(group_get_id(group), nickname_get_id(client->nickname));
      if(myaccess->level != GRPMASTER_FLAG)
      {
        reply_user(service, NULL, client, SERV_NO_ACCESS_CHAN, "DEL",
            group_get_name(group));
        group_free(group);
        MyFree(access);
        return;
      }
    }
  }

  group_masters_count(group_get_id(group), &mcount);
  if(access->level == GRPMASTER_FLAG && mcount <= 1)
  {
    reply_user(service, service, client, GS_ACCESS_NOMASTERS, parv[2],
        group_get_name(group));
    group_free(group);
    MyFree(access);
    return;
  }

  if(groupaccess_remove(access))
  {
    reply_user(service, service, client, GS_ACCESS_DELOK, parv[2], parv[1]);
    ilog(L_DEBUG, "%s (%s@%s) removed AE %s from %s",
      client->name, client->username, client->host, parv[2], parv[1]);
  }
  else
    reply_user(service, service, client, GS_ACCESS_DELFAIL, parv[2], parv[1]);

  MyFree(access);

  group_free(group);
}

static void
m_access_list(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct GroupAccess *access = NULL;
  Group *group;
  char *nick;
  int i = 1;
  dlink_node *ptr;
  dlink_list list = { 0 };

  group = group_find(parv[1]);

  if(groupaccess_list(group_get_id(group), &list) == 0)
  {
    reply_user(service, service, client, GS_ACCESS_LISTEND, 
        group_get_name(group));
    MyFree(access);
    group_free(group);
    return;
  }

  DLINK_FOREACH(ptr, list.head)
  {
    char *level;
    access = (struct GroupAccess *)ptr->data;

    switch(access->level)
    {
      /* XXX Some sort of lookup table maybe, but we only have these 3 atm */
      case GRPMEMBER_FLAG:
        level = "MEMBER";
        break;
      case GRPMASTER_FLAG:
        level = "MASTER";
        break;
      default:
        level = "UNKNOWN";
        break;
    }

    nick = nickname_nick_from_id(access->account, TRUE);
    reply_user(service, service, client, GS_ACCESS_LIST, i++, nick, level);

    MyFree(nick);
  }

  reply_user(service, service, client, GS_ACCESS_LISTEND, group_get_name(group));

  groupaccess_list_free(&list);

  group_free(group);
}

