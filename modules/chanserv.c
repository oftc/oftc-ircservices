/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  chanserv.c: A C implementation of Chanell Services
 *
 *  Copyright (C) 2006 The OFTC Coding department
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

static struct Service *chanserv = NULL;

static dlink_node *cs_cmode_hook;
static dlink_node *cs_join_hook;
static dlink_node *cs_channel_destroy_hook;
static dlink_node *cs_on_nick_drop_hook;
static dlink_node *cs_on_topic_change_hook;

static dlink_list channel_limit_list = { NULL, NULL, 0 };
static dlink_list channel_expireban_list = { NULL, NULL, 0 };

static void process_limit_list(void *);
static void process_expireban_list(void *);

static void *cs_on_cmode_change(va_list);
static void *cs_on_client_join(va_list);
static void *cs_on_channel_destroy(va_list);
static void *cs_on_nick_drop(va_list);
static void *cs_on_topic_change(va_list);

static void m_register(struct Service *, struct Client *, int, char *[]);
static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_drop(struct Service *, struct Client *, int, char *[]);
static void m_info(struct Service *, struct Client *, int, char *[]);
static void m_sudo(struct Service *, struct Client *, int, char *[]);

static void m_set_desc(struct Service *, struct Client *, int, char *[]);
static void m_set_url(struct Service *, struct Client *, int, char *[]);
static void m_set_email(struct Service *, struct Client *, int, char *[]);
static void m_set_entrymsg(struct Service *, struct Client *, int, char *[]);
static void m_set_topic(struct Service *, struct Client *, int, char *[]);
static void m_set_topiclock(struct Service *, struct Client *, int, char *[]);
static void m_set_private(struct Service *, struct Client *, int, char *[]);
static void m_set_restricted(struct Service *, struct Client *, int, char *[]);
static void m_set_verbose(struct Service *, struct Client *, int, char *[]);
static void m_set_autolimit(struct Service *, struct Client *, int, char *[]);
static void m_set_expirebans(struct Service *, struct Client *, int, char *[]);
static void m_set_mlock(struct Service *, struct Client *, int, char *[]);

static void m_akick_add(struct Service *, struct Client *, int, char *[]);
static void m_akick_list(struct Service *, struct Client *, int, char *[]);
static void m_akick_del(struct Service *, struct Client *, int, char *[]);
static void m_akick_enforce(struct Service *, struct Client *, int, char *[]);

static void m_clear_bans(struct Service *, struct Client *, int, char *[]);
static void m_clear_ops(struct Service *, struct Client *, int, char *[]);
static void m_clear_voices(struct Service *, struct Client *, int, char *[]);
static void m_clear_users(struct Service *, struct Client *, int, char *[]);

static void m_op(struct Service *, struct Client *, int, char *[]);
static void m_deop(struct Service *, struct Client *, int, char *[]);
static void m_invite(struct Service *, struct Client *, int, char *[]);
static void m_unban(struct Service *, struct Client *, int, char *[]);

static void m_access_add(struct Service *, struct Client *, int, char *[]);
static void m_access_del(struct Service *, struct Client *, int, char *[]);
static void m_access_list(struct Service *, struct Client *, int, char *[]);

/* private */
static int m_set_flag(struct Service *, struct Client *, char *, char *, int, char *);
static int m_set_string(struct Service *, struct Client *, const char *, 
    const char *, unsigned int, const char *, char **, int);

static struct ServiceMessage register_msgtab = {
  NULL, "REGISTER", 0, 2, 2, SFLG_UNREGOK|SFLG_NOMAXPARAM, CHIDENTIFIED_FLAG, 
  CS_HELP_REG_SHORT, CS_HELP_REG_LONG, m_register
};

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, 2, SFLG_UNREGOK, CHUSER_FLAG, CS_HELP_SHORT, 
  CS_HELP_LONG, m_help
};

static struct ServiceMessage set_sub[] = {
  { NULL, "DESC", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG|SFLG_NOMAXPARAM, MASTER_FLAG, 
    CS_HELP_SET_DESC_SHORT, CS_HELP_SET_DESC_LONG, m_set_desc },
  { NULL, "URL", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
    CS_HELP_SET_URL_SHORT, CS_HELP_SET_URL_LONG, m_set_url },
  { NULL, "EMAIL", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
    CS_HELP_SET_EMAIL_SHORT, CS_HELP_SET_EMAIL_LONG, m_set_email },
  { NULL, "ENTRYMSG", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG|SFLG_NOMAXPARAM, CHANOP_FLAG, 
    CS_HELP_SET_ENTRYMSG_SHORT, CS_HELP_SET_ENTRYMSG_LONG, m_set_entrymsg },
  { NULL, "TOPIC", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG|SFLG_NOMAXPARAM, CHANOP_FLAG, 
    CS_HELP_SET_TOPIC_SHORT, CS_HELP_SET_TOPIC_LONG, m_set_topic },
  { NULL, "TOPICLOCK", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
    CS_HELP_SET_TOPICLOCK_SHORT, CS_HELP_SET_TOPICLOCK_LONG, m_set_topiclock },
  { NULL, "MLOCK", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
    CS_HELP_SET_MLOCK_SHORT, CS_HELP_SET_MLOCK_LONG, m_set_mlock }, 
  { NULL, "PRIVATE", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
    CS_HELP_SET_PRIVATE_SHORT, CS_HELP_SET_PRIVATE_LONG, m_set_private },
  { NULL, "RESTRICTED", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
    CS_HELP_SET_RESTRICTED_SHORT, CS_HELP_SET_RESTRICTED_LONG, m_set_restricted },
  { NULL, "VERBOSE", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
    CS_HELP_SET_VERBOSE_SHORT, CS_HELP_SET_VERBOSE_LONG, m_set_verbose },
  { NULL, "AUTOLIMIT", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
    CS_HELP_SET_AUTOLIMIT_SHORT, CS_HELP_SET_AUTOLIMIT_LONG, m_set_autolimit },
  { NULL, "EXPIREBANS", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
    CS_HELP_SET_EXPIREBANS_SHORT, CS_HELP_SET_EXPIREBANS_LONG, m_set_expirebans },
  { NULL, NULL, 0, 0, 0, SFLG_KEEPARG|SFLG_CHANARG, 0, 0, SFLG_KEEPARG|SFLG_CHANARG, NULL } 
};

static struct ServiceMessage set_msgtab = {
  set_sub, "SET", 0, 2, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
  CS_HELP_SET_SHORT, CS_HELP_SET_LONG, NULL
};

static struct ServiceMessage access_sub[6] = {
  { NULL, "ADD", 0, 3, 3, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
    CS_HELP_ACCESS_ADD_SHORT, CS_HELP_ACCESS_ADD_LONG, m_access_add },
  { NULL, "DEL", 0, 2, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
    CS_HELP_ACCESS_DEL_SHORT, CS_HELP_ACCESS_DEL_LONG, m_access_del },
  { NULL, "LIST", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, CHUSER_FLAG, 
    CS_HELP_ACCESS_LIST_SHORT, CS_HELP_ACCESS_LIST_LONG, m_access_list },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage access_msgtab = {
  access_sub, "ACCESS", 0, 2, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
  CS_HELP_ACCESS_SHORT, CS_HELP_ACCESS_LONG, NULL 
};

static struct ServiceMessage akick_sub[] = {
  { NULL, "ADD", 0, 2, 3, SFLG_NOMAXPARAM|SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG, 
    CS_HELP_AKICK_ADD_SHORT, CS_HELP_AKICK_ADD_LONG, m_akick_add }, 
  { NULL, "DEL", 0, 2, 2, SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG, 
    CS_HELP_AKICK_DEL_SHORT, CS_HELP_AKICK_DEL_LONG, m_akick_del },
  { NULL, "LIST", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, MEMBER_FLAG,
    CS_HELP_AKICK_LIST_SHORT, CS_HELP_AKICK_LIST_LONG, m_akick_list },
  { NULL, "ENFORCE", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, MEMBER_FLAG, 
    CS_HELP_AKICK_ENFORCE_SHORT, CS_HELP_AKICK_ENFORCE_LONG, m_akick_enforce },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage akick_msgtab = {
  akick_sub, "AKICK", 0, 2, 2, SFLG_KEEPARG|SFLG_CHANARG, MEMBER_FLAG, 
  CS_HELP_AKICK_SHORT, CS_HELP_AKICK_LONG, NULL
};

static struct ServiceMessage drop_msgtab = {
  NULL, "DROP", 0, 1, 2, SFLG_CHANARG, MASTER_FLAG, 
  CS_HELP_DROP_SHORT, CS_HELP_DROP_LONG, m_drop
};

static struct ServiceMessage info_msgtab = {
  NULL, "INFO", 0, 1, 1, SFLG_CHANARG, CHUSER_FLAG,
  CS_HELP_INFO_SHORT, CS_HELP_INFO_LONG, m_info
};

static struct ServiceMessage op_msgtab = {
  NULL, "OP", 0, 1, 2, SFLG_CHANARG, CHANOP_FLAG, 
  CS_HELP_OP_SHORT, CS_HELP_OP_LONG, m_op
};

static struct ServiceMessage deop_msgtab = {
  NULL, "DEOP", 0, 1, 2, SFLG_CHANARG, CHANOP_FLAG, 
  CS_HELP_DEOP_SHORT, CS_HELP_DEOP_LONG, m_deop
};

static struct ServiceMessage unban_msgtab = {
  NULL, "UNBAN", 0, 1, 1, SFLG_CHANARG, MEMBER_FLAG, 
  CS_HELP_UNBAN_SHORT, CS_HELP_UNBAN_LONG, m_unban
};

static struct ServiceMessage invite_msgtab = {
  NULL, "INVITE", 0, 1, 2, SFLG_CHANARG, MEMBER_FLAG,
  CS_HELP_INVITE_SHORT, CS_HELP_INVITE_LONG, m_invite
};

static struct ServiceMessage clear_sub[] = {
  { NULL, "BANS", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG, 
    CS_HELP_CLEAR_BANS_SHORT, CS_HELP_CLEAR_BANS_LONG, m_clear_bans },
  { NULL, "OPS", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG, 
    CS_HELP_CLEAR_OPS_SHORT, CS_HELP_CLEAR_OPS_LONG, m_clear_ops },
  { NULL, "VOICES", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG, 
    CS_HELP_CLEAR_VOICES_SHORT, CS_HELP_CLEAR_VOICES_LONG, m_clear_voices },
  { NULL, "USERS", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
    CS_HELP_CLEAR_USERS_SHORT, CS_HELP_CLEAR_USERS_LONG, m_clear_users },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage clear_msgtab = {
  clear_sub, "CLEAR", 0, 2, 2, SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG, 
  CS_HELP_CLEAR_SHORT, CS_HELP_CLEAR_LONG, NULL
};

static struct ServiceMessage sudo_msgtab = {
  NULL, "SUDO", 0, 2, 2, SFLG_NOMAXPARAM, ADMIN_FLAG,
  CS_HELP_SUDO_SHORT, CS_HELP_SUDO_LONG, m_sudo
};

/*
LIST
SYNC
SENDPASS
*/

INIT_MODULE(chanserv, "$Revision$")
{
  chanserv = make_service("ChanServ");
  clear_serv_tree_parse(&chanserv->msg_tree);
  dlinkAdd(chanserv, &chanserv->node, &services_list);
  hash_add_service(chanserv);
  introduce_client(chanserv->name);
  load_language(chanserv->languages, "chanserv.en");
/*  load_language(chanserv, "chanserv.rude");
  load_language(chanserv, "chanserv.de");
*/
  mod_add_servcmd(&chanserv->msg_tree, &register_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &help_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &set_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &drop_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &akick_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &access_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &info_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &op_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &deop_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &invite_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &clear_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &unban_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &invite_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &sudo_msgtab);
  cs_cmode_hook = install_hook(on_cmode_change_cb, cs_on_cmode_change);
  cs_join_hook  = install_hook(on_join_cb, cs_on_client_join);
  cs_channel_destroy_hook = 
       install_hook(on_channel_destroy_cb, cs_on_channel_destroy);
  cs_on_nick_drop_hook = install_hook(on_nick_drop_cb, cs_on_nick_drop);
  cs_on_topic_change_hook = install_hook(on_topic_change_cb, cs_on_topic_change);

  eventAdd("process channel autolimits", process_limit_list, NULL, 10);
  eventAdd("process channel expirebans", process_expireban_list, NULL, 10);
}

CLEANUP_MODULE
{
  uninstall_hook(on_cmode_change_cb, cs_on_cmode_change);
  uninstall_hook(on_join_cb, cs_on_client_join);
  uninstall_hook(on_channel_destroy_cb, cs_on_channel_destroy);
  uninstall_hook(on_nick_drop_cb, cs_on_nick_drop);
  uninstall_hook(on_topic_change_cb, cs_on_topic_change);

  serv_clear_messages(chanserv);

  unload_languages(chanserv->languages);

  exit_client(find_client(chanserv->name), &me, "Service unloaded");
  hash_del_service(chanserv);
  dlinkDelete(&chanserv->node, &services_list);
  eventDelete(process_limit_list, NULL);
  eventDelete(process_expireban_list, NULL);
  ilog(L_DEBUG, "Unloaded chanserv");
}

static void
process_limit_list(void *param)
{
  dlink_node *ptr;

  DLINK_FOREACH(ptr, channel_limit_list.head)
  {
    struct Channel *chptr = ptr->data;

    if(chptr->limit_time > CurrentTime)
    {
      int limit;

      limit = dlink_list_length(&chptr->members) + 3;
      if(chptr->mode.limit != limit)
        set_limit(chanserv, chptr, limit);

      chptr->limit_time = CurrentTime + 90;
    }
  }
}

static void
process_expireban_list(void *param)
{
  dlink_node *ptr;
  dlink_node *bptr, *bnptr;

  DLINK_FOREACH(ptr, channel_expireban_list.head)
  {
    struct Channel *chptr = ptr->data;

    DLINK_FOREACH_SAFE(bptr, bnptr, chptr->banlist.head)
    {
      struct Ban *banptr = bptr->data;
      char ban[IRC_BUFSIZE+1];

      if((CurrentTime - banptr->when) > (1*60*60))
      {
        snprintf(ban, IRC_BUFSIZE, "%s!%s@%s", banptr->name, banptr->username,
            banptr->host);
        unban_mask(chanserv, chptr, ban);
      }
    }
  }
}

static void 
m_register(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Channel    *chptr;
  struct RegChannel *regchptr;

  assert(parv[1]);

  /* Bail out if channelname does not start with a hash */
  if (*parv[1] != '#')
  {
    reply_user(service, service, client, CS_NAMESTART_HASH);
    ilog(L_DEBUG, "Channel REG failed for %s on %s (-ESPELING)", client->name, 
        parv[1]);
    return;
  }

  /* Bail out if services dont know the channel (it does not exist)
     or if client is no member of the channel */
  chptr = hash_find_channel(parv[1]);
  if ((chptr == NULL) || (!IsMember(client, chptr))) 
  {
    reply_user(service, service, client, CS_NOT_ONCHAN);
    ilog(L_DEBUG, "Channel REG failed for %s on %s (notonchan)", client->name, 
        parv[1]);
    return;
  }
  
  /* bail out if client is not opped on channel */
  if (!IsChanop(client, chptr))
  {
    reply_user(service, service, client, CS_NOT_OPPED);
    ilog(L_DEBUG, "Channel REG failed for %s on %s (notop)", client->name, 
        parv[1]);
    return;
  }

  /* finally, bail out if channel is already registered */
  if (chptr->regchan != NULL)
  {
    reply_user(service, service, client, CS_ALREADY_REG, parv[1]);
    ilog(L_DEBUG, "Channel REG failed for %s on %s (exists)", client->name, 
        parv[1]);
    return;
  }

  regchptr = MyMalloc(sizeof(struct RegChannel));
  strlcpy(regchptr->channel, parv[1], sizeof(regchptr->channel));
  DupString(regchptr->description, parv[2]);

  if (db_register_chan(regchptr, client->nickname->id))
  {
    chptr->regchan = regchptr;
    reply_user(service, service, client, CS_REG_SUCCESS, parv[1]);
    ilog(L_NOTICE, "%s!%s@%s registered channel %s", 
        client->name, client->username, client->host, parv[1]);
  }
  else
  {
    reply_user(service, service, client, CS_REG_FAIL, parv[1]);
    ilog(L_DEBUG, "Channel REG failed for %s on %s", client->name, parv[1]);
  }
}

static void 
m_drop(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;

  assert(parv[1]);

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  if (db_delete_chan(parv[1]))
  {
    reply_user(service, service, client, CS_DROPPED, parv[1]);
    ilog(L_NOTICE, "%s!%s@%s dropped channel %s", 
      client->name, client->username, client->host, parv[1]);

    free_regchan(regchptr);
    if(chptr != NULL)
      chptr->regchan = NULL;
    regchptr = NULL;
  } 
  else
  {
    ilog(L_DEBUG, "Channel DROP failed for %s on %s", client->name, parv[1]);
    reply_user(service, service, client, CS_DROP_FAILED, parv[1]);
  }

  if (chptr == NULL && regchptr != NULL)
    free_regchan(regchptr);
}

static void
m_info(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  
  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;
  
  reply_user(service, service, client, CS_INFO_CHAN_START, regchptr->channel);
  reply_time(service, client, CS_INFO_REGTIME_FULL, regchptr->regtime);
  reply_user(service, service, client, CS_INFO_CHAN, regchptr->description, 
      regchptr->url == NULL ? "Not Set" : regchptr->url, 
      regchptr->email == NULL ? "Not Set" : regchptr->email, 
      regchptr->topic == NULL ? "Not Set" : regchptr->topic, 
      regchptr->entrymsg == NULL ? "Not Set" : regchptr->entrymsg);

  reply_user(service, service, client, CS_INFO_OPTION, "TOPICLOCK",
      regchptr->topic_lock ? "ON" : "OFF");
  
  reply_user(service, service, client, CS_INFO_OPTION, "PRIVATE",
      regchptr->priv ? "ON" : "OFF");

  reply_user(service, service, client, CS_INFO_OPTION, "RESTRICTED",
      regchptr->restricted ? "ON" : "OFF");

  reply_user(service, service, client, CS_INFO_OPTION, "VERBOSE",
      regchptr->verbose ? "ON" : "OFF");
 
  reply_user(service, service, client, CS_INFO_OPTION, "AUTOLIMIT",
      regchptr->autolimit ? "ON" : "OFF");

  reply_user(service, service, client, CS_INFO_OPTION, "EXPIREBANS",
      regchptr->expirebans ? "ON" : "OFF");

  if (chptr == NULL)
    free_regchan(regchptr);
}

static void
m_help(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  do_help(service, client, parv[1], parc, parv);
}

/* ACCESS ADD nick type */
static void
m_access_add(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  struct ChanAccess *access;
  unsigned int account, level;
  char *level_added = "MEMBER";

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  if((account = db_get_id_from_name(parv[2], GET_NICKID_FROM_NICK)) <= 0)
  {
    reply_user(service, service, client, CS_REGISTER_NICK, parv[2]);
    if(chptr == NULL)
      free_regchan(regchptr);
    return;
  }

  if(irccmp(parv[3], "MASTER") == 0)
  {
    level = MASTER_FLAG;
    level_added = "MASTER";
  }
  else if(irccmp(parv[3], "CHANOP") == 0)
  {
    level = CHANOP_FLAG;
    level_added = "CHANOP";
  }
  else if(irccmp(parv[3], "MEMBER") == 0)
    level = MEMBER_FLAG;
  else
  {
    reply_user(service, service, client, CS_ACCESS_BADLEVEL, parv[3]);
    if(chptr == NULL)
      free_regchan(regchptr);
    return;
  }

  access = MyMalloc(sizeof(struct ChanAccess));
  access->channel = regchptr->id;
  access->account = account;
  access->level   = level;
 
  if(db_list_add(CHACCESS_LIST, access))
  {
    reply_user(service, service, client, CS_ACCESS_ADDOK, parv[2], parv[1],
        level_added);
    ilog(L_DEBUG, "%s (%s@%s) added AE %s(%d) to %s", client->name, 
        client->username, client->host, parv[2], access->level, parv[1]);
  }
  else
    reply_user(service, service, client, CS_ACCESS_ADDFAIL, parv[2], parv[1],
        parv[3]);

  if (chptr == NULL)
    free_regchan(regchptr);

  free_chanaccess(access);
}


static void
m_access_del(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  struct ChanAccess *access;
  unsigned int nickid;

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  if((nickid = db_get_id_from_name(parv[2], GET_NICKID_FROM_NICK)) <= 0)
  {
    reply_user(service, service, client, CS_ACCESS_NOTLISTED, parv[2], parv[1]);
    if(chptr == NULL)
      free_regchan(regchptr);
    return;
  }

  access = db_find_chanaccess(regchptr->id, nickid);
  if(access == NULL)
  {
    reply_user(service, service, client, CS_ACCESS_NOTLISTED, parv[2], parv[1]);
      if(chptr == NULL)
        free_regchan(regchptr);
    return;
  }

  if(access->level == MASTER_FLAG && db_get_num_masters(regchptr->id) <= 1)
  {
    reply_user(service, service, client, CS_ACCESS_NOMASTERS, parv[2],
        regchptr->channel);
    if(chptr == NULL)
      free_regchan(regchptr);
    free_chanaccess(access);
    return;
  }
  free_chanaccess(access);

  if(db_list_del_index(DELETE_CHAN_ACCESS, nickid, regchptr->id))
  {
    reply_user(service, service, client, CS_ACCESS_DELOK, parv[2], parv[1]);
    ilog(L_DEBUG, "%s (%s@%s) removed AE %s from %s", 
      client->name, client->username, client->host, parv[2], parv[1]);
  }
  else
    reply_user(service, service, client, CS_ACCESS_DELFAIL, parv[2], parv[1]);

  if (chptr == NULL)
    free_regchan(regchptr);
}

static void
m_access_list(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct ChanAccess *access;
  struct Channel *chptr;
  struct RegChannel *regchptr;
  void *handle, *first;
  char *nick;
  int i = 1;

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  first = handle = db_list_first(CHACCESS_LIST, regchptr->id, (void**)&access);
  if (handle == NULL)
  {
    reply_user(service, service, client, CS_ACCESS_LISTEND, regchptr->channel);
    return;
  }

  while(handle != NULL)
  {
    char *level;

    switch(access->level)
    {
      /* XXX Some sort of lookup table maybe, but we only have these 3 atm */
      case MEMBER_FLAG:
        level = "MEMBER";
        break;
      case CHANOP_FLAG:
        level = "CHANOP";
        break;
      case MASTER_FLAG:
        level = "MASTER";
        break;
      default:
        level = "UNKNOWN";
        break;
    }

    nick = db_get_nickname_from_id(access->account);
    reply_user(service, service, client, CS_ACCESS_LIST, i++, nick, level);

    free_chanaccess(access);
    MyFree(nick);
    handle = db_list_next(handle, CHACCESS_LIST, (void **)&access);
  }

  free_chanaccess(access);

  db_list_done(first);
  reply_user(service, service, client, CS_ACCESS_LISTEND, regchptr->channel);

  if (chptr == NULL)
    free_regchan(regchptr);
}

static void
m_set_desc(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  char value[IRC_BUFSIZE+1];

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  join_params(value, parc-1, &parv[2]);

  m_set_string(service, client, parv[1], "DESC", SET_CHAN_DESC, value, 
      &regchptr->description, parc);

  if(chptr == NULL)
    free_regchan(regchptr);
}

static void
m_set_url(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  char value[IRC_BUFSIZE+1];

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  join_params(value, parc-1, &parv[2]);

  m_set_string(service, client, parv[1], "URL", SET_CHAN_URL, value, 
      &regchptr->url, parc);

  if(chptr == NULL)
    free_regchan(regchptr);
}

static void
m_set_email(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  char value[IRC_BUFSIZE+1];

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  join_params(value, parc-1, &parv[2]);

  m_set_string(service, client, parv[1], "EMAIL", SET_CHAN_EMAIL, value, 
      &regchptr->email, parc);

  if(chptr == NULL)
    free_regchan(regchptr);
}

static void
m_set_entrymsg(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  char value[IRC_BUFSIZE+1];

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  join_params(value, parc-1, &parv[2]);

  m_set_string(service, client, parv[1], "ENTRYMSG", SET_CHAN_ENTRYMSG, value, 
      &regchptr->entrymsg, parc);

  if(chptr == NULL)
    free_regchan(regchptr);
}

static void
m_set_topic(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  char value[IRC_BUFSIZE+1];
  char *topic = NULL;
  int changetopic = FALSE;

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  DupString(topic, regchptr->topic);

  join_params(value, parc-1, &parv[2]);
  strlcpy(value, value, TOPICLEN+1);

  m_set_string(service, client, parv[1], "TOPIC", SET_CHAN_TOPIC, value, 
      &regchptr->topic, parc);

  if(topic != NULL && strncmp(topic, regchptr->topic, 
        LIBIO_MAX(strlen(topic), strlen(regchptr->topic))) != 0)
    changetopic = TRUE;

  if(changetopic || ((topic == NULL && (regchptr->topic != NULL)) && (chptr != NULL)))
    send_topic(service, chptr, client, regchptr->topic);

  MyFree(topic);

  if(chptr == NULL)
    free_regchan(regchptr);
}

static void
m_set_mlock(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  char value[IRC_BUFSIZE+1];

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  join_params(value, parc-1, &parv[2]);

  if(parc == 1)
  {
    m_set_string(service, client, parv[1], "MLOCK", SET_CHAN_MLOCK, value, 
        &regchptr->mlock, parc);
    if(chptr == NULL)
      free_regchan(regchptr);
    return;
  }

  if(set_mode_lock(service, chptr, client, value, &regchptr->mlock))
  {
    if(regchptr->mlock == NULL)
      m_set_string(service, client, parv[1], "MLOCK", SET_CHAN_MLOCK, value,
          &regchptr->mlock, parc);
    else
      reply_user(service, service, client, CS_SET_SUCCESS, "MLOCK", 
          regchptr->mlock, parv[1]);
  }
  else
    reply_user(service, service, client, CS_SET_FAILED, "MLOCK", 
        value, parv[1]);

  if(chptr == NULL)
    free_regchan(regchptr);
}
static void
m_set_topiclock(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], SET_CHAN_TOPICLOCK, 
      "TOPICLOCK");
}

static void
m_set_private(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], SET_CHAN_PRIVATE, "PRIVATE");
}

static void
m_set_restricted(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], SET_CHAN_RESTRICTED, 
      "RESTRICTED");
}

static void
m_set_verbose(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], SET_CHAN_VERBOSE, "VERBOSE");
}

static void
m_set_autolimit(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr = hash_find_channel(parv[1]);

  m_set_flag(service, client, parv[1], parv[2], SET_CHAN_AUTOLIMIT, "AUTOLIMIT");
  if(chptr != NULL && chptr->regchan != NULL && chptr->regchan->autolimit)
  {
    chptr->limit_time = CurrentTime + 90;
    dlinkAdd(chptr, make_dlink_node(), &channel_limit_list);
  }
  else if(chptr != NULL && chptr->regchan != NULL)
  {
    dlink_node *ptr;

    if((ptr = dlinkFindDelete(&channel_limit_list, chptr)) != NULL)
      free_dlink_node(ptr);
  }
}
  
static void
m_set_expirebans(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr = hash_find_channel(parv[1]);

  m_set_flag(service, client, parv[1], parv[2], SET_CHAN_EXPIREBANS, 
      "EXPIREBANS");
  if(chptr != NULL && chptr->regchan != NULL && chptr->regchan->expirebans)
    dlinkAdd(chptr, make_dlink_node(), &channel_expireban_list);
  else if(chptr != NULL && chptr->regchan != NULL)
  {
    dlink_node *ptr;

    if((ptr = dlinkFindDelete(&channel_expireban_list, chptr)) != NULL)
      free_dlink_node(ptr);
  }
}

/* AKICK ADD (nick|mask) reason */
static void
m_akick_add(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct ServiceBan *akick;
  struct Nick *nick;
  struct Channel *chptr;
  struct RegChannel *regchptr;
  char reason[IRC_BUFSIZE+1];

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  akick = MyMalloc(sizeof(struct ServiceBan));
  akick->type = AKICK_BAN;
  if(strchr(parv[2], '@') == NULL)
  {
    /* Nickname based akick */
    if((nick = db_find_nick(parv[2])) == NULL)
    {
      reply_user(service, service, client, CS_AKICK_NONICK, parv[2]);
      MyFree(akick);
      if(chptr == NULL)
        free_regchan(regchptr);
      return;
    }
    akick->mask = NULL;
    akick->target = nick->id;
    free_nick(nick);
  }
  else
  {
    /* mask based akick */
    akick->target = 0;
    DupString(akick->mask, parv[2]);
  }

  akick->setter = client->nickname->id;
  DupString(akick->channel, parv[1]);
  akick->time_set = CurrentTime;
  akick->duration = 0;

  if(parv[3] != NULL)
  {
    join_params(reason, parc-2, &parv[3]);
    DupString(akick->reason, reason);
  }
  else
    DupString(akick->reason, "You are not permitted on this channel");

  if(db_list_add(AKICK_LIST, akick))
    reply_user(service, service, client, CS_AKICK_ADDED, parv[2]);
  else
    reply_user(service, service, client, CS_AKICK_ADDFAIL, parv[2]);

  if(chptr == NULL)
    free_regchan(regchptr);
  else
  {
    int numkicks = 0;

    numkicks = enforce_akick(service, chptr, akick);
    reply_user(service, service, client, CS_AKICK_ENFORCE, regchptr->channel,
      numkicks);
  }

  free_serviceban(akick);
}

static void
m_akick_list(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct ServiceBan *akick;
  void *handle, *first;
  int i = 1;
  struct Channel *chptr;
  struct RegChannel *regchptr;
  char setbuf[TIME_BUFFER + 1];

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  first = handle = db_list_first(AKICK_LIST, regchptr->id, (void**)&akick);
  while(handle != NULL)
  {
    char *who, *whoset;

    if(akick->target == 0)
      who = akick->mask;
    else
      who = db_get_nickname_from_id(akick->target);

    whoset = db_get_nickname_from_id(akick->setter);

    strtime(client, akick->time_set, setbuf);

    reply_user(service, service, client, CS_AKICK_LIST, i++, who, akick->reason,
        whoset, setbuf);
    if(akick->target != 0)
      MyFree(who);
    free_serviceban(akick);
    MyFree(whoset);
    handle = db_list_next(handle, AKICK_LIST, (void**)&akick);
  }
  if(first)
    db_list_done(first);

  free_serviceban(akick);
  reply_user(service, service, client, CS_AKICK_LISTEND, parv[1]);

  if(chptr == NULL)
    free_regchan(regchptr);
}

static void
m_akick_del(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  int index, ret;

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  index = atoi(parv[2]);
  if(index > 0)
    ret = db_list_del_index(DELETE_AKICK_IDX, regchptr->id, index);
  else if(strchr(parv[2], '@') != NULL)
    ret = db_list_del(DELETE_AKICK_MASK, regchptr->id, parv[2]);
  else
    ret = db_list_del(DELETE_AKICK_ACCOUNT, regchptr->id, parv[2]);

  reply_user(service, service, client, CS_AKICK_DEL, ret);
}

static void
m_akick_enforce(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  int numkicks = 0;
  dlink_node *ptr;

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? db_find_chan(parv[1]) : chptr->regchan;

  DLINK_FOREACH(ptr, chptr->members.head)
  {
    struct Membership *ms = ptr->data;
    struct Client *client = ms->client_p;

    numkicks += enforce_matching_serviceban(service, chptr, client);
  }

  reply_user(service, service, client, CS_AKICK_ENFORCE, numkicks, 
      regchptr->channel);

  if(chptr == NULL)
    free_regchan(regchptr);
}

static void
m_clear_bans(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  dlink_node *ptr, *nptr;
  int numbans = 0;
 
  chptr = hash_find_channel(parv[1]);

  if(chptr == NULL)
  {
    reply_user(service, service, client, CS_CHAN_NOT_USED, parv[1]);
    return;
  }
  regchptr = chptr->regchan;

  DLINK_FOREACH_SAFE(ptr, nptr, chptr->banlist.head)
  {
    const struct Ban *banptr = ptr->data;
    char ban[IRC_BUFSIZE+1];

    snprintf(ban, IRC_BUFSIZE, "%s!%s@%s", banptr->name, banptr->username,
        banptr->host);
    unban_mask(service, chptr, ban);
    numbans++;
  }

  reply_user(service, service, client, CS_CLEAR_BANS, numbans, 
      regchptr->channel);
}

static void
m_clear_ops(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  dlink_node *ptr;
  int opcount = 0;
 
  chptr = hash_find_channel(parv[1]);

  if(chptr == NULL)
  {
    reply_user(service, service, client, CS_CHAN_NOT_USED, parv[1]);
    return;
  }

  regchptr = chptr->regchan;
  DLINK_FOREACH(ptr, chptr->members.head)
  {
    struct Membership *ms = ptr->data;
    struct Client *target = ms->client_p;

    if(has_member_flags(ms, CHFL_CHANOP))
    {
      deop_user(service, chptr, target);
      opcount++;
    }
  }
  reply_user(service, service, client, CS_CLEAR_OPS, opcount, 
      regchptr->channel);
}

static void
m_clear_voices(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  dlink_node *ptr;
  int voicecount = 0;
 
  chptr = hash_find_channel(parv[1]);

  if(chptr == NULL)
  {
    reply_user(service, service, client, CS_CHAN_NOT_USED, parv[1]);
    return;
  }

  regchptr = chptr->regchan;
  DLINK_FOREACH(ptr, chptr->members.head)
  {
    struct Membership *ms = ptr->data;
    struct Client *target = ms->client_p;

    if(has_member_flags(ms, CHFL_VOICE))
    {
      devoice_user(service, chptr, target);
      voicecount++;
    }
  }
  reply_user(service, service, client, CS_CLEAR_VOICES, voicecount, 
      regchptr->channel);
}

static void
m_clear_users(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  dlink_node *ptr, *nptr;
  char buf[IRC_BUFSIZE+1];
  int usercount = 0;
 
  chptr = hash_find_channel(parv[1]);

  if(chptr == NULL)
  {
    reply_user(service, service, client, CS_CHAN_NOT_USED, parv[1]);
    return;
  }
  regchptr = chptr->regchan;

  snprintf(buf, IRC_BUFSIZE, "CLEAR USERS command used by %s", client->name);

  DLINK_FOREACH_SAFE(ptr, nptr, chptr->members.head)
  {
    struct Membership *ms = ptr->data;
    struct Client *target = ms->client_p;

    kick_user(service, chptr, target->name, buf);
    usercount++;
  }

  reply_user(service, service, client, CS_CLEAR_USERS, usercount, 
      regchptr->channel);
}

static void
m_op(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  struct Client *target;
  struct Membership *ms;

  chptr = hash_find_channel(parv[1]);

  if(chptr == NULL)
  {
    reply_user(service, service, client, CS_CHAN_NOT_USED, parv[1]);
    return;
  }

  regchptr = chptr->regchan;

  if(parv[2] == NULL)
    target = client;
  else
    target = find_client(parv[2]);

  if(target == NULL || (ms = find_channel_link(target, chptr)) == NULL)
  {
    reply_user(service, service, client, CS_NOT_ON_CHAN, parv[2], parv[1]);
    return;
  }

  if(!has_member_flags(ms, CHFL_CHANOP))
  {
    op_user(service, chptr, target);
    reply_user(service, service, client, CS_OP, target->name, parv[1]);
  }
}

static void
m_deop(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  struct Client *target;
  struct Membership *ms;

  chptr = hash_find_channel(parv[1]);

  if(chptr == NULL)
  {
    reply_user(service, service, client, CS_CHAN_NOT_USED, parv[1]);
    return;
  }

  regchptr = chptr->regchan;
  if(parv[2] == NULL)
    target = client;
  else
    target = find_client(parv[2]);

  if(target == NULL || (ms = find_channel_link(target, chptr)) == NULL)
  {
    reply_user(service, service, client, CS_NOT_ON_CHAN, parv[2], parv[1]);
    return;
  }

  if(has_member_flags(ms, CHFL_CHANOP))
  {
    deop_user(service, chptr, target);
    reply_user(service, service, client, CS_DEOP, target->name, parv[1]);
  }
}

static void
m_invite(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  struct Client *target;

  chptr = hash_find_channel(parv[1]);

  if(chptr == NULL)
  {
    reply_user(service, service, client, CS_CHAN_NOT_USED, parv[1]);
    return;
  }
  
  regchptr = chptr->regchan;

  if(parv[2] == NULL)
    target = client;
  else
    target = find_client(parv[2]);

  if(target == NULL)
  {
    reply_user(service, service, client, CS_NICK_NOT_ONLINE, parv[2]);
    return;
  }

  if(find_channel_link(target, chptr) != NULL)
  {
    reply_user(service, service, client, CS_ALREADY_ON_CHAN, target->name, 
        parv[1]);
    return;
  }

  invite_user(service, chptr, target);
  reply_user(service, service, client, CS_INVITED, target->name, parv[1]);
}

static void
m_unban(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  struct Ban *banp;
  int numbans = 0;

  chptr = hash_find_channel(parv[1]);

  if(chptr == NULL)
  {
    reply_user(service, service, client, CS_CHAN_NOT_USED, parv[1]);
    return;
  }
  regchptr = chptr->regchan;

  banp = find_bmask(client, &chptr->banlist);
  while(banp != NULL)
  {
    char ban[IRC_BUFSIZE+1];

    snprintf(ban, IRC_BUFSIZE, "%s!%s@%s", banp->name, banp->username,
        banp->host);
    unban_mask(service, chptr, ban);
    numbans++;

    banp = find_bmask(client, &chptr->banlist);
  }

  reply_user(service, service, client, CS_CLEAR_BANS, numbans,
      regchptr->channel);
}

static int
m_set_string(struct Service *service, struct Client *client, 
    const char *channel, const char *field, unsigned int query, 
    const char *value, char **param, int parc)
{
  struct Channel *chptr;
  struct RegChannel *regchptr;

  chptr = hash_find_channel(channel);
  regchptr = chptr == NULL ? db_find_chan(channel) : chptr->regchan;

  if(parc < 2)
  {
    char *response = *param;

    if(response == NULL)
      response = "unset";
    reply_user(service, service, client, CS_SET_VALUE, field, response, 
        regchptr->channel);
    if(chptr == NULL)
      free_regchan(regchptr);
    return TRUE;
  }

  if(ircncmp(value, "unset", strlen(value)) == 0)
    value = NULL;

  if(db_set_string(query, regchptr->id, value))
  {
    reply_user(service, service, client, CS_SET_SUCCESS, field, 
        value == NULL ? "unset" : value, regchptr->channel);
    ilog(L_DEBUG, "%s (%s@%s) changed %s of %s to %s", 
      client->name, client->username, client->host, field, regchptr->channel, 
      value);

    if(value != NULL)
      *param = replace_string(*param, value);
    else
    {
      MyFree(*param);
      *param = NULL;
    }
    if(chptr == NULL)
      free_regchan(regchptr);

    return TRUE;
  }
  else
    reply_user(service, service, client, CS_SET_FAILED, field,
        value == NULL ? "unset" : value, regchptr->channel);
  
  if(chptr == NULL)
    free_regchan(regchptr);

  return FALSE;
}

static int
m_set_flag(struct Service *service, struct Client *client,
           char *channel, char *toggle, int type, char *flagname)
{
  struct Channel *chptr;
  struct RegChannel *regchptr;
  int on;

  chptr = hash_find_channel(channel);
  regchptr = chptr == NULL ? db_find_chan(channel) : chptr->regchan;

  if (toggle == NULL)
  {
    switch(type)
    {
      case SET_CHAN_PRIVATE:
        on = regchptr->priv;
        break;
      case SET_CHAN_RESTRICTED:
        on = regchptr->restricted;
        break;
      case SET_CHAN_TOPICLOCK:
        on = regchptr->topic_lock;
        break;
      case SET_CHAN_VERBOSE:
        on = regchptr->verbose;
        break;
      case SET_CHAN_AUTOLIMIT:
        on = regchptr->autolimit;
        break;
      default:
        on = FALSE;
        break;
    }
    reply_user(service, service, client, CS_SET_VALUE, flagname, 
        on ? "ON" : "OFF", channel);
    
    if (chptr == NULL)
      free_regchan(regchptr);
    return -1;
  }

  if (strncasecmp(toggle, "ON", 2) == 0)
    on = TRUE;
  else if (strncasecmp(toggle, "OFF", 3) == 0)
    on = FALSE;
  else
  {
    reply_user(service, service, client, CS_SET_VALUE, flagname,
        on ? "ON" : "OFF", channel);
    return -1;
  }

  if (db_set_bool(type, regchptr->id, on))
  {
    reply_user(service, service, client, CS_SET_SUCCESS, 
        flagname, on ? "ON" : "OFF", channel);

    switch(type)
    {
      case SET_CHAN_PRIVATE:
        regchptr->priv= on;
        break;
      case SET_CHAN_RESTRICTED:
        regchptr->restricted = on;
        break;
      case SET_CHAN_TOPICLOCK:
        regchptr->topic_lock = on;
        break;
      case SET_CHAN_VERBOSE:
        regchptr->verbose = on;
        break;
      case SET_CHAN_AUTOLIMIT:
        regchptr->autolimit = on;
        break;
    }
    if (chptr == NULL)
      free_regchan(regchptr);
  }
  else
  {
    reply_user(service, service, client, CS_SET_FAILED, flagname, channel);
    if (chptr == NULL)
      free_regchan(regchptr);
  }

  return 0;
}

static void
m_sudo(struct Service *service, struct Client *client, int parc, char *parv[])
{
  char buf[IRC_BUFSIZE] = { '\0' };
  char **newparv;

  newparv = MyMalloc(4 * sizeof(char*));

  newparv[0] = parv[0];
  newparv[1] = service->name;

  join_params(buf, parc, &parv[1]);

  DupString(newparv[2], buf);

  client->access = SUDO_FLAG;

  ilog(L_INFO, "%s executed %s SUDO: %s", client->name, service->name, 
      newparv[2]);

  process_privmsg(1, me.uplink, client, 3, newparv);
  MyFree(newparv[2]);
  MyFree(newparv);

  client->access = MASTER_FLAG;
}

/**
 * @brief CS Callback when a ModeChange is received for a Channel
 * @param args 
 * @return pass_callback()
 * We dont do anything yet :-)
 */
static void *
cs_on_cmode_change(va_list args) 
{
  struct Client  *source  = va_arg(args, struct Client*);
  struct Channel *chptr   = va_arg(args, struct Channel*);
  int  dir  = va_arg(args, int);
  char mode = (char)va_arg(args, int);
  char *param = va_arg(args, char *);

  if(chptr->regchan != NULL && chptr->regchan->mlock != NULL)
  {
    set_mode_lock(chanserv, chptr, NULL, chptr->regchan->mlock, NULL);
  }
  
  /* last function to call in this func */
  return pass_callback(cs_cmode_hook, source, chptr, dir, mode, param);
}

/**
 * @brief CS Callback when a Client joins a Channel
 * @param args 
 * @return pass_callback(self, struct Client *, char *)
 * When a Client joins a Channel:
 *  - attach struct RegChannel * to struct Channel*
 */
static void *
cs_on_client_join(va_list args)
{
  struct Client *source_p = va_arg(args, struct Client *);
  char          *name     = va_arg(args, char *);
  
  struct RegChannel *regchptr;
  struct Channel *chptr;
  struct ChanAccess *access;
  unsigned int level;

  /* Find Channel in hash */
  if((chptr = hash_find_channel(name)) == NULL)
  {
    ilog(L_ERROR, "badbad. Client %s joined non-existing Channel %s\n", 
        source_p->name, chptr->chname);
    return pass_callback(cs_join_hook, source_p, name);
  }

  if(enforce_matching_serviceban(chanserv, chptr, source_p))
    return pass_callback(cs_join_hook, source_p, name);

  if((regchptr = chptr->regchan) == NULL)
    return pass_callback(cs_join_hook, source_p, name);

  if(source_p->nickname == NULL)
    level = CHUSER_FLAG;
  else
  {
    access = db_find_chanaccess(regchptr->id, source_p->nickname->id);
    if(access == NULL)
      level = CHIDENTIFIED_FLAG;
    else
      level = access->level;
  }

  if(regchptr->restricted && level < MEMBER_FLAG)
  {
    char ban[IRC_BUFSIZE+1];

    snprintf(ban, IRC_BUFSIZE, "*!%s@%s", source_p->username, source_p->host);
    ban_mask(chanserv, chptr, ban);
    kick_user(chanserv, chptr, source_p->name, 
        "Access to this channel is restricted");
    return pass_callback(cs_join_hook, source_p, name);
  }

  if(IsChanop(source_p, chptr) && level < CHANOP_FLAG)
  {
    deop_user(chanserv, chptr, source_p);
    reply_user(chanserv, chanserv, source_p, CS_DEOP_REGISTERED, 
        regchptr->channel);
  }

  if(regchptr->entrymsg != NULL && regchptr->entrymsg[0] != '\0' &&
      !IsConnecting(me.uplink))
    reply_user(chanserv, chanserv, source_p, CS_ENTRYMSG, regchptr->channel,
        regchptr->entrymsg);

  if(regchptr->autolimit)
  {
    if(dlinkFind(&channel_limit_list, chptr) == NULL)
    {
      chptr->limit_time = CurrentTime + 90;
      dlinkAdd(chptr, make_dlink_node(), &channel_limit_list);
    }
  }

  if(regchptr->expirebans)
  {
    if(dlinkFind(&channel_expireban_list, chptr) == NULL)
    {
      dlinkAdd(chptr, make_dlink_node(), &channel_expireban_list);
    }
  }

  return pass_callback(cs_join_hook, source_p, name);
}


/**
 * @brief CS Callback when a channel is destroyed.
 * @param args
 * @return pass_callback(self, struct Channel *)
 * When a Channel is destroyed, 
 *  - we need to detach struct RegChannel from struct Channel->regchan 
 */
static void *
cs_on_channel_destroy(va_list args)
{
  struct Channel *chan = va_arg(args, struct Channel *);

  if (chan->regchan != NULL)
  {
    free_regchan(chan->regchan);
    chan->regchan = NULL;
  }

  return pass_callback(cs_channel_destroy_hook, chan);
}

/**
 * @brief CS Callback when a nick is dropped
 * @param args 
 * @return pass_callback(self, char *)
 * When a Nick is dropped
 * - we need to make sure theres no Channel left with the nick as eounder
 */
static void *
cs_on_nick_drop(va_list args)
{
  char *nick = va_arg(args, char *);

  return pass_callback(cs_on_nick_drop_hook, nick);
}

static void *
cs_on_topic_change(va_list args)
{
  struct Channel *chan = va_arg(args, struct Channel *);
  char *setter = va_arg(args, char *);
  struct RegChannel *regchptr;

  if(chan->regchan == NULL)
    return pass_callback(cs_on_topic_change_hook, chan, setter);

  regchptr = chan->regchan;

  if(regchptr->topic_lock)
  {
    if(regchptr->topic != NULL)
    {
      if(chan->topic == NULL || ircncmp(chan->topic, regchptr->topic,
            LIBIO_MAX(strlen(chan->topic), strlen(regchptr->topic))) != 0)
      {
        send_topic(chanserv, chan, find_client(chanserv->name), regchptr->topic); 
      }
    }
  }

  return pass_callback(cs_on_topic_change_hook, chan, setter);
}
