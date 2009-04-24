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

static void m_set_url(struct Service *, struct Client *, int, char *[]);
static void m_set_email(struct Service *, struct Client *, int, char *[]);
static void m_set_private(struct Service *, struct Client *, int, char *[]);

static int m_set_flag(struct Service *, struct Client *, char *, char *,
    unsigned char (*)(Group *), int (*)(Group *, unsigned char));
static int m_set_string(struct Service *, struct Client *, const char *,
    const char *, int, const char *(*)(Group *),
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
  NULL, "DROP", 0, 0, 1, 0, IDENTIFIED_FLAG, GS_HELP_DROP_SHORT, GS_HELP_DROP_LONG,
  m_drop
};

static struct ServiceMessage set_sub[] = {
  { NULL, "URL", 0, 0, 1, 0, IDENTIFIED_FLAG, GS_HELP_SET_URL_SHORT, 
    GS_HELP_SET_URL_LONG, m_set_url },
  { NULL, "EMAIL", 0, 0, 1, 0, IDENTIFIED_FLAG, GS_HELP_SET_EMAIL_SHORT, 
    GS_HELP_SET_EMAIL_LONG, m_set_email },
  { NULL, "PRIVATE", 0, 0, 1, 0, IDENTIFIED_FLAG, GS_HELP_SET_PRIVATE_SHORT, 
    GS_HELP_SET_PRIVATE_LONG, m_set_private },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage set_msgtab = {
  set_sub, "SET", 0, 1, 1, 0, IDENTIFIED_FLAG, GS_HELP_SET_SHORT, 
  GS_HELP_SET_LONG, NULL 
};

static struct ServiceMessage info_msgtab = {
  NULL, "INFO", 0, 0, 1, 0, USER_FLAG, GS_HELP_INFO_SHORT, GS_HELP_INFO_LONG,
  m_info
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
#if 0
  struct Client *target;
  Group *group = group_find(client->name);
  char *target_name = NULL;
  char *masterless_channel = NULL;

  assert(group != NULL);

  /* This might be being executed via sudo, find the real user of the group */
  if(group_get_id(client->groupname) != group_get_id(group))
  {
    target_name = group_group_from_id(group_get_nameid(client->groupname), FALSE);
    target = find_client(target_name);
  }
  else
  {
    target = client;
    target_name = client->name;
  }

  group_free(group);

  if(target == client)
  {
    /* A normal user dropping their own group */
    if(parc == 0)
    {
      /* No auth code, so give them one and do nothing else*/
      char buf[IRC_BUFSIZE+1] = {0};
      char *hmac;

      snprintf(buf, IRC_BUFSIZE, "DROP %ld %d %s", CurrentTime, 
          group_get_nameid(target->groupname), target->name);
      hmac = generate_hmac(buf);

      reply_user(service, service, client, GS_DROP_AUTH, service->name, 
          CurrentTime, hmac);

      MyFree(hmac);
      return;
    }
    else
    {
      /* Auth code given, check it and do the drop */
      char buf[IRC_BUFSIZE+1] = {0};
      char *hmac;
      char *auth;
      int timestamp;

      if((auth = strchr(parv[1], ':')) == NULL)
      {
        reply_user(service, service, client, GS_DROP_AUTH_FAIL, client->name);
        return;
      }

      *auth = '\0';
      auth++;

      snprintf(buf, IRC_BUFSIZE, "DROP %s %d %s", parv[1], 
          group_get_nameid(target->groupname), target->name);
      hmac = generate_hmac(buf);

      if(strncmp(hmac, auth, strlen(hmac)) != 0)
      {
        MyFree(hmac);
        reply_user(service, service, client, GS_DROP_AUTH_FAIL, client->name);
        return;
      }

      MyFree(hmac);
      timestamp = atoi(parv[1]);
      if((CurrentTime - timestamp) > 3600)
      {
        reply_user(service, service, client, GS_DROP_AUTH_FAIL, client->name);
        return;
      }
    }
  }

  /* Authentication passed(possibly because they're using sudo), go ahead and
   * drop
   */

  masterless_channel = check_masterless_channels(group_get_id(client->groupname));

  if(masterless_channel != NULL)
  {
    reply_user(service, service, client, GS_DROP_FAIL_MASTERLESS, target_name, masterless_channel);
    MyFree(masterless_channel);
    if(target != client)
      MyFree(target_name);
  }

  if(group_delete(client->groupname)) 
  {
    if(target != NULL)
    {
      ClearIdentified(target);
      if(target->groupname != NULL)
        group_free(target->groupname);
      target->groupname = NULL;
      target->access = USER_FLAG;
      send_umode(groupserv, target, "-R");
      reply_user(service, service, client, GS_group_DROPPED, target->name);
      ilog(L_NOTICE, "%s!%s@%s dropped group %s", client->name, 
        client->username, client->host, target->name);
    }
    else
    {
      reply_user(service, service, client, GS_group_DROPPED, target_name);
      ilog(L_NOTICE, "%s!%s@%s dropped group %s", client->name, 
        client->username, client->host, target_name);
    }
  }
  else
  {
    reply_user(service, service, client, GS_group_DROPFAIL, target_name);
  }

  if(target != client)
    MyFree(target_name);
#endif
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
  m_set_string(service, client, "URL", parv[1], parc,
    &group_get_url, &group_set_url);
}

static void
m_set_email(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  m_set_string(service, client, "EMAIL", parv[1], parc,
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

  if(client->nickname)
    access = groupaccess_find(group_get_id(group), 
        nickname_get_id(client->nickname));

  reply_user(service, service, client, GS_INFO_START, group_get_name(group));
  reply_time(service, client, GS_INFO_REGTIME_FULL, group_get_regtime(group));

  if(IsOper(client) || (access != NULL && access->level >= MEMBER_FLAG))
  {
    const char *ptr = group_get_desc(group);
    ptr = group_get_url(group);
    ptr = group_get_email(group);
    reply_user(service, service, client, GS_INFO,
        group_get_desc(group),
        group_get_url(group) == NULL ? "^BNot Set^B" : group_get_url(group),
        group_get_email(group) == NULL ? "^BNot Set^B" : group_get_email(group));
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
             const char *field, const char *value, int parc,
             const char *(*get_func)(Group *),
             int(*set_func)(Group *, const char*))
{
#if 0
  Group *group = client->groupname;
  int ret = FALSE;

  if(parc == 0)
  {
    const char *resp = get_func(group);
    reply_user(service, service, client, GS_SET_VALUE, field,
      resp == NULL ? "Not Set" : resp);
    return TRUE;
  }

  if(irccmp(value, "-") == 0)
    value = NULL;

  ret = set_func(group, value);

  reply_user(service, service, client, ret ? GS_SET_SUCCESS : GS_SET_FAILED, field,
      value == NULL ? "Not Set" : value);

  return ret;
#endif
  return TRUE;
}
