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

static struct CHACCESS_LALA ChAccessNames[13] = {
  { "BAN",       CHACCESS_BAN },
  { "AUTODEOP",  CHACCESS_AUTODEOP },
  { "VOICE",     CHACCESS_VOICE },
  { "OP",        CHACCESS_OP },
  { "INVITE",    CHACCESS_INVITE },
  { "UNBAN",     CHACCESS_UNBAN },
  { "AKICK",     CHACCESS_AKICK },
  { "CLEAR",     CHACCESS_CLEAR },
  { "SET",       CHACCESS_SET },
  { "ACCESS",    CHACCESS_ACCESS },
  { "AUTOVOICE", CHACCESS_AUTOVOICE },
  { "AUTOOP",    CHACCESS_AUTOOP },
  { NULL, 0 }
};


static struct Service *chanserv = NULL;

static dlink_node *cs_cmode_hook;
static dlink_node *cs_join_hook;
static dlink_node *cs_channel_destroy_hook;
static dlink_node *cs_on_nick_drop_hook;

static void *cs_on_cmode_change(va_list);
static void *cs_on_client_join(va_list);
static void *cs_on_channel_destroy(va_list);
static void *cs_on_nick_drop(va_list);

static void m_register(struct Service *, struct Client *, int, char *[]);
static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_drop(struct Service *, struct Client *, int, char *[]);
static void m_info(struct Service *, struct Client *, int, char *[]);
static void m_set(struct Service *, struct Client *, int, char *[]);

static void m_set_founder(struct Service *, struct Client *, int, char *[]);
static void m_set_successor(struct Service *, struct Client *, int, char *[]);
static void m_set_desc(struct Service *, struct Client *, int, char *[]);
static void m_set_url(struct Service *, struct Client *, int, char *[]);
static void m_set_email(struct Service *, struct Client *, int, char *[]);
static void m_set_entrymsg(struct Service *, struct Client *, int, char *[]);
static void m_set_topic(struct Service *, struct Client *, int, char *[]);
static void m_set_keeptopic(struct Service *, struct Client *, int, char *[]);
static void m_set_topiclock(struct Service *, struct Client *, int, char *[]);
static void m_set_private(struct Service *, struct Client *, int, char *[]);
static void m_set_restricted(struct Service *, struct Client *, int, char *[]);
static void m_set_secure(struct Service *, struct Client *, int, char *[]);
static void m_set_secureops(struct Service *, struct Client *, int, char *[]);
static void m_set_leaveops(struct Service *, struct Client *, int, char *[]);
static void m_set_verbose(struct Service *, struct Client *, int, char *[]);

static void m_access_del(struct Service *, struct Client *, int, char *[]);
static void m_access_add(struct Service *, struct Client *, int, char *[]);
static void m_access_list(struct Service *, struct Client *, int, char *[]);
static void m_access_view(struct Service *, struct Client *, int, char *[]);
static void m_access_count(struct Service *, struct Client *, int, char *[]);


/* temp */
static void m_not_avail(struct Service *, struct Client *, int, char *[]);

/* private */
static int m_set_flag(struct Service *, struct Client *, char *, char *, int, char *);
static struct RegChannel* cs_get_regchan_from_hash_or_db(struct Service *, struct Client *, struct Channel *, char *);

static struct ServiceMessage register_msgtab = {
  NULL, "REGISTER", 0, 2, CS_HELP_REG_SHORT, CS_HELP_REG_LONG,
  { m_unreg, m_register, m_register, m_register }
};

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, CS_HELP_SHORT, CS_HELP_LONG,
  { m_help, m_help, m_help, m_help }
};

/* 
 * contrary to old services:  /msg chanserv ACCESS #channel ADD foo bar baz
 * we do this:                /msg chanserv ACCESS ADD #channel foo bar baz
 * this is not nice, but fits our structure better
 */
static struct SubMessage set_sub[] = {
  { "FOUNDER",     0, 1, CS_SET_FOUNDER_SHORT, CS_SET_FOUNDER_LONG, 
    { m_set_founder, m_set_founder, m_set_founder, m_set_founder }
  },
  { "SUCCESSOR",   0, 1, CS_SET_SUCC_SHORT, CS_SET_SUCC_LONG, 
    { m_set_successor, m_set_successor, m_set_successor, m_set_successor }
  },
  { "PASSWORD",    0, 1, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail }
  },
  { "DESC",        0, 1, CS_SET_DESC_SHORT, CS_SET_DESC_LONG, 
    { m_set_desc, m_set_desc, m_set_desc, m_set_desc }
  },
  { "URL",         0, 1, CS_SET_URL_SHORT, CS_SET_URL_LONG, 
    { m_set_url, m_set_url, m_set_url, m_set_url }
  },
  { "EMAIL",       0, 1, CS_SET_EMAIL_SHORT, CS_SET_EMAIL_LONG, 
    { m_set_email, m_set_email, m_set_email, m_set_email }
  },
  { "ENTRYMSG",    0, 1, CS_SET_ENTRYMSG_SHORT, CS_SET_ENTRYMSG_LONG, 
    { m_set_entrymsg, m_set_entrymsg, m_set_entrymsg, m_set_entrymsg }
  },
  { "TOPIC",       0, 1, CS_SET_TOPIC_SHORT, CS_SET_TOPIC_LONG, 
    { m_set_topic, m_set_topic, m_set_topic, m_set_topic }
  },
  { "KEEPTOPIC",   0, 1, CS_SET_KEEPTOPIC_SHORT, CS_SET_KEEPTOPIC_LONG, 
    { m_set_keeptopic, m_set_keeptopic, m_set_keeptopic, m_set_keeptopic }
  },
  { "TOPICLOCK",   0, 1, CS_SET_TOPICLOCK_SHORT, CS_SET_TOPICLOCK_LONG, 
    { m_set_topiclock, m_set_topiclock, m_set_topiclock, m_set_topiclock }
  },
  { "MLOCK",       0, 1, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail }
  }, // +kl-mnt
  { "PRIVATE",     0, 1, CS_SET_PRIVATE_SHORT, CS_SET_PRIVATE_LONG, 
    { m_set_private, m_set_private, m_set_private, m_set_private }
  },
  { "RESTRICTED",  0, 1, CS_SET_RESTRICTED_SHORT, CS_SET_RESTRICTED_LONG, 
    { m_set_restricted, m_set_restricted, m_set_restricted, m_set_restricted }
  },
  { "SECURE",      0, 1, CS_SET_SECURE_SHORT, CS_SET_SECURE_LONG, 
    { m_set_secure, m_set_secure, m_set_secure, m_set_secure }
  },
  { "SECUREOPS",   0, 1, CS_SET_SECUREOPS_SHORT, CS_SET_SECUREOPS_LONG, 
    { m_set_secureops, m_set_secureops, m_set_secureops, m_set_secureops }
  },
  { "LEAVEOPS",    0, 1, CS_SET_LEAVEOPS_SHORT, CS_SET_LEAVEOPS_LONG, 
    { m_set_leaveops, m_set_leaveops, m_set_leaveops, m_set_leaveops }
  },
  { "VERBOSE",     0, 1, CS_SET_VERBOSE_SHORT, CS_SET_VERBOSE_LONG, 
    { m_set_verbose, m_set_verbose, m_set_verbose, m_set_verbose }
  },
  { "AUTOLIMIT",   0, 1, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail }
  }, // 5:2:2
  { "CLEARBANS",   0, 1, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail }
  }, // 120
  { NULL,          0, 0,  0,  0, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail }
  } 
};

static struct ServiceMessage set_msgtab = {
  set_sub, "SET", 0, 0, CS_SET_SHORT, CS_SET_LONG,
  { m_unreg, m_set, m_set, m_set }
};

static struct SubMessage access_sub[6] = {
  { "ADD",   0, 3, -1, -1, 
    { m_not_avail, m_access_add, m_access_add, m_access_add } },
  { "DEL",   0, 2, -1, -1, 
    { m_not_avail, m_access_del, m_access_del, m_access_del } },
  { "LIST",  0, 2, -1, -1, 
    { m_not_avail, m_access_list, m_access_list, m_access_list } },
  { "VIEW",  0, 1, -1, -1, 
    { m_not_avail, m_access_view, m_access_view, m_access_view } },
  { "COUNT", 0, 0, -1, -1, 
    { m_not_avail, m_access_count, m_access_count, m_access_count } },
  { NULL,    0, 0,  0,  0, { NULL, NULL, NULL, NULL } }
};

static struct ServiceMessage access_msgtab = {
  access_sub, "ACCESS", 0, 0, -1, -1, 
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct SubMessage levels_sub[6] = {
  { "SET",      0, 3, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail } },
  { "LIST",     0, 0, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail } },
  { "RESET",    0, 0, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail } },
  { "DIS",      0, 1, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail } },
  { "DISABLED", 0, 1, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail } },
  { NULL,       0, 0,  0,  0, { NULL, NULL, NULL, NULL } }
};

static struct ServiceMessage levels_msgtab = {
  levels_sub, "LEVELS", 0, 0, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct SubMessage akick_sub[7] = {
  { "ADD",     0, 2, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail } }, 
  { "DEL",     0, 1, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail } },
  { "LIST",    0, 1, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail } },
  { "VIEW",    0, 1, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail } },
  { "ENFORCE", 0, 0, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail } },
  { "COUNT",   0, 0, -1, -1, 
    { m_not_avail, m_not_avail, m_not_avail, m_not_avail } },
  { NULL,      0, 0,  0,  0, { NULL, NULL, NULL, NULL } }
};

static struct ServiceMessage akick_msgtab = {
  akick_sub, "AKICK", 0, 0, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct ServiceMessage drop_msgtab = {
  NULL, "DROP", 0, 1, -1, -1,
  { m_unreg, m_drop, m_drop, m_drop }
};

static struct ServiceMessage identify_msgtab = {
  NULL, "IDENTIFY", 0, 1, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct ServiceMessage info_msgtab = {
  NULL, "INFO", 0, 1, -1, -1,
  { m_unreg, m_info, m_info, m_info }
};

// ...

static struct ServiceMessage op_msgtab = {
  NULL, "OP", 0, 1, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct ServiceMessage deop_msgtab = {
  NULL, "DEOP", 0, 1, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};


static struct ServiceMessage unban_msgtab = {
  NULL, "UNBAN", 0, 1, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct ServiceMessage invite_msgtab = {
  NULL, "INVITE", 0, 1, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct ServiceMessage clear_msgtab = {
  NULL, "CLEAR", 0, 1, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct ServiceMessage aop_msgtab = {
  NULL, "AOP", 0, 1, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct ServiceMessage vop_msgtab = {
  NULL, "VOP", 0, 1, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct ServiceMessage sop_msgtab = {
  NULL, "SOP", 0, 1, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
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
  introduce_service(chanserv);
  load_language(chanserv, "chanserv.en");
/*  load_language(chanserv, "chanserv.rude");
  load_language(chanserv, "chanserv.de");
*/
  mod_add_servcmd(&chanserv->msg_tree, &register_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &help_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &set_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &drop_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &akick_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &identify_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &levels_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &access_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &info_msgtab);
  cs_cmode_hook = install_hook(on_cmode_change_cb, cs_on_cmode_change);
  cs_join_hook  = install_hook(on_join_cb, cs_on_client_join);
  cs_channel_destroy_hook = 
       install_hook(on_channel_destroy_cb, cs_on_channel_destroy);
  cs_on_nick_drop_hook = install_hook(on_nick_drop_cb, cs_on_nick_drop);
}

CLEANUP_MODULE
{
  uninstall_hook(on_cmode_change_cb, cs_on_cmode_change);
  uninstall_hook(on_join_cb, cs_on_client_join);
  uninstall_hook(on_channel_destroy_cb, cs_on_channel_destroy);
  uninstall_hook(on_nick_drop_cb, cs_on_nick_drop);
  exit_client(find_client(chanserv->name), &me, "Service unloaded");
  hash_del_service(chanserv);
  dlinkDelete(&chanserv->node, &services_list);
}


/*
 * CHANSERV REGISTER
 */
static void 
m_register(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  ilog(L_DEBUG, "T: %s wishes to reg %s", client->name, parv[1]);
  struct Channel    *chptr;
  struct RegChannel *regchptr;

  assert(parv[1]);

  // Bail out if channelname does not start with a hash
  if ( *parv[1] != '#' )
  {
    reply_user(service, client, CS_NAMESTART_HASH);
    ilog(L_DEBUG, "Channel REG failed for %s on %s (-ESPELING)", client->name, parv[1]);
    return;
  }

  // Bail out if services dont know the channel (it does not exist)
  // or if client is no member of the channel
  chptr = hash_find_channel(parv[1]);
  if ((chptr == NULL) || (! IsMember(client, chptr))) 
  {
    reply_user(service, client, CS_NOT_ONCHAN);
    ilog(L_DEBUG, "Channel REG failed for %s on %s (notonchan)", client->name, parv[1]);
    return;
  }
  
  // bail out if client is not opped on channel
  if (! IsChanop(client, chptr))
  {
    reply_user(service, client, CS_NOT_OPPED);
    ilog(L_DEBUG, "Channel REG failed for %s on %s (notop)", client->name, parv[1]);
    return;
  }

  // finally, bail out if channel is already registered
  regchptr = db_find_chan(parv[1]);
  if (regchptr != NULL)
  {
    reply_user(service, client, CS_ALREADY_REG, parv[1]);
    ilog(L_DEBUG, "Channel REG failed for %s on %s (exists)", client->name, parv[1]);
    free_regchan(regchptr);
    return;
  }

  if (db_register_chan(client, parv[1]) == 0)
  {
    chptr->regchan = db_find_chan(parv[1]);
    reply_user(service, client, CS_REG_SUCCESS, parv[1]);
    ilog(L_NOTICE, "%s!%s@%s registered channel %s", 
        client->name, client->username, client->host, parv[1]);
  }
  else
  {
    reply_user(service, client, CS_REG_FAIL, parv[1]);
    ilog(L_DEBUG, "Channel REG failed for %s on %s", client->name, parv[1]);
  }
  ilog(L_TRACE, "T: Leaving CS:m_register (%s:%s)", client->name, parv[1]);
}


/* 
 * CHANSERV DROP
 */
static void 
m_drop(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  ilog(L_TRACE, "T: %s wishes to drop %s", client->name, parv[1]);
  struct Channel *chptr;
  struct RegChannel *regchptr;

  assert(parv[1]);

  chptr = hash_find_channel(parv[1]);
  regchptr = cs_get_regchan_from_hash_or_db(service, client, chptr, parv[1]);
  
  if (regchptr->founder != client->nickname->id)
  {
    reply_user(service, client, CS_OWN_CHANNEL_ONLY, parv[1]);
    ilog(L_DEBUG, "Channel DROP failed for %s on %s (notown)", client->name, parv[1]);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
    return;
  }

  if (db_delete_chan(parv[1]) == 0)
  {
    reply_user(service, client, CS_DROPPED, parv[1]);
    ilog(L_NOTICE, "%s!%s@%s dropped channel %s", 
      client->name, client->username, client->host, parv[1]);

    chptr->regchan = NULL;
  } else
  {
    ilog(L_DEBUG, "Channel DROP failed for %s on %s", client->name, parv[1]);
    reply_user(service, client, CS_DROP_FAILED, parv[1]);
  }
  if (chptr == NULL)
  {
    free_regchan(regchptr);
  }
  ilog(L_TRACE, "T: Leaving CS:m_drop (%s:%s)", client->name, parv[1]);
  return;
}

/* XXX temp XXX */
static void
m_not_avail(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  reply_user(service, client, 0, "This function is currently not implemented."
    "   Bug the Devs! ;-)");
}

/*
 * CHANSERV INFO
 */
static void
m_info(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  ilog(L_TRACE, "Channel INFO from %s for %s", client->name, parv[1]);

  struct Channel *chptr;
  struct RegChannel *regchptr;

  chptr = hash_find_channel(parv[1]);
  regchptr = cs_get_regchan_from_hash_or_db(service, client, chptr, parv[1]);

  reply_user(service, client, CS_INFO_CHAN, parv[1], 
  db_get_nickname_from_id(regchptr->founder),
  db_get_nickname_from_id(regchptr->successor),
  regchptr->description, regchptr->url, regchptr->email,
  regchptr->topic, regchptr->entrymsg,
  IsChanKeeptopic(regchptr)  ? "KEEPTOPIC"  : "" ,
  IsChanTopiclock(regchptr)  ? "TOPICLOCK"  : "" ,
  IsChanPrivate(regchptr)    ? "PRIVATE"    : "" ,
  IsChanRestricted(regchptr) ? "RESTRICTED" : "" ,
  IsChanSecure(regchptr)     ? "SECURE"     : "" ,
  IsChanLeaveops(regchptr)   ? "LEAVEOPS"   : "" ,
  IsChanVerbose(regchptr)    ? "VERBOSE"    : "" );

  if (chptr == NULL)
    free_regchan(regchptr);

  ilog(L_TRACE, "T: Leaving CS:m_drop (%s:%s)", client->name, parv[1]);
}

/*
 * CHANSERV HELP
 */
static void
m_help(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  do_help(service, client, parv[1], parc, parv);
}


/*
 * CHANSERV SET
 */
static void
m_set(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  reply_user(service, client, CS_SET_LONG, "");
}


/*
 * CHANSERV SET FOUNDER
 */
static void
m_set_founder(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  ilog(L_TRACE, "Channel SET FOUNDER from %s for %s", client->name, parv[1]);

  struct Channel *chptr;
  struct RegChannel *regchptr;
  struct Nick *nick_p;
  char *foundernick;

  chptr = hash_find_channel(parv[1]);
  regchptr = cs_get_regchan_from_hash_or_db(service, client, chptr, parv[1]);

  if (regchptr->founder != client->nickname->id)
  {
    reply_user(service, client, CS_OWN_CHANNEL_ONLY, parv[1]);
    ilog(L_DEBUG, "Channel SET FOUNDER failed for %s on %s (notown)", client->name, parv[1]);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
    return;
  }

  if (parc < 2)
  {
    foundernick = db_get_nickname_from_id(regchptr->founder);
    reply_user(service, client, CS_SET_FOUNDER, parv[1], foundernick);
    ilog(L_TRACE, "T: Leaving CS:m_set_founder (%s:%s) (INFO ONLY)", client->name, parv[1]);
    MyFree(foundernick);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
    return;
  }

  /* we need to consult the db, since the nick may not be online -mc */
  if ((nick_p = db_find_nick(parv[2])) == NULL)
  {
    reply_user(service, client, CS_REGISTER_NICK, parv[2]);
    ilog(L_DEBUG, "Channel SET FOUNDER failed for %s on %s (newnotreg)", client->name, parv[1]);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
    return;
  }
  free_nick(nick_p);
  MyFree(nick_p);


  if (db_set_founder(parv[1], parv[2]) == 0)
  {
    reply_user(service, client, CS_SET_FOUNDER, parv[1], parv[2]);
    ilog(L_NOTICE, "%s (%s@%s) set founder of %s to %s", 
      client->name, client->username, client->host, parv[1], parv[2]);
    regchptr->founder = db_get_id_from_nick(parv[2]); 
  }
  else
  {
    reply_user(service, client, CS_SET_FOUNDER_FAILED, parv[1], parv[2]);
  }

  if (chptr == NULL)
  {
    free_regchan(regchptr);
  }
  ilog(L_TRACE, "T: Leaving CS:m_set_foudner (%s:%s)", client->name, parv[1]);
}


/*
 * CHANSERV SET SUCCESSOR
 */
static void
m_set_successor(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  ilog(L_TRACE, "Channel SET SUCCESSOR from %s for %s", client->name, parv[1]);

  struct Channel *chptr;
  struct RegChannel *regchptr;
  struct Nick *nick_p;
  char *successornick;

  chptr = hash_find_channel(parv[1]);
  regchptr = cs_get_regchan_from_hash_or_db(service, client, chptr, parv[1]);

  if (regchptr->founder != client->nickname->id)
  {
    reply_user(service, client, CS_OWN_CHANNEL_ONLY, parv[1]);
    ilog(L_DEBUG, "Channel SET SUCCESSOR failed for %s on %s (notown)", client->name, parv[1]);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
    return;
  }

  if (parc < 2)
  {
    successornick = db_get_nickname_from_id(regchptr->successor);
    reply_user(service, client, CS_SET_SUCCESSOR, parv[1], successornick);
    ilog(L_TRACE, "leaving CS:m_set_successor for %s on %s (INFO ONLY)", client->name, parv[1]);
    MyFree(successornick);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
    return;
  }

  if ((nick_p = db_find_nick(parv[2])) == NULL)
  {
    reply_user(service, client, CS_REGISTER_NICK, parv[2]);
    ilog(L_DEBUG, "Channel SET SUCCESSOR failed for %s on %s (newnotreg)", client->name, parv[1]);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
    return;
  } 

  free_nick(nick_p);
  MyFree(nick_p);

  if (db_set_successor(parv[1], parv[2]) == 0)
  {
    reply_user(service, client, CS_SET_SUCC, parv[1], parv[2]);
    ilog(L_NOTICE, "%s (%s@%s) set successor of %s to %s", 
      client->name, client->username, client->host, parv[1], parv[2]);
   regchptr->successor = db_get_id_from_nick(parv[2]); 
  }
  else
  {
    reply_user(service, client, CS_SET_SUCC_FAILED, parv[1], parv[2]);
  }
  if (chptr == NULL)
  {
    free_regchan(regchptr);
  }
  ilog(L_TRACE, "T: Leaving CS:m_set_successor (%s:%s)", client->name, parv[1]);
}

/**
 * Calculate a new level, given the old numeric level and another levelname
 * @param level old level (48)
 * @param ae level to be added/removed ("+AUTOOP")
 * @return new level (48 +CHACCESS_AUTOOP)
 * Given the old level as int and another new levelchange, in form of a word
 * we return the new level as int
 * for example: level=48 + newlevel: "+AUTOOP" = 48 + CHACCESS_AUTOOP
 */
static long int
set_access_level_by_name(long int level, char *ae)
{
  int dir;
  char *ptr;
  
  if (*ae == '+')
  {
    dir = 1;
    ptr = ae;
    ptr++;
  }
  else if (*ae == '-')
  {
    dir = 2;
    ptr = ae;
    ptr++;
  }
  else
  {
    dir = 1;
    ptr = ae;
  }
  
  int i;
  
  for (i=0; ChAccessNames[i].level != 0; i++)
  {
    if (strncmp(ChAccessNames[i].name, ae, strlen(ae) == 0))
    {
      if (dir == 1)
      {
        level |= ChAccessNames[i].level;
      } else if (dir == 2)
      {
        level &= ~ChAccessNames[i].level;
      }
      break;
    }
  }
  
  return level;
}

static void
m_access_add(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  ilog(L_TRACE, "CS ACCESS ADD from %s for %s", client->name, parv[1]);

  struct Channel *chptr;
  struct RegChannel *regchptr;
  struct ChannelAccessEntry *cae;
  int update;

  chptr = hash_find_channel(parv[1]);
  regchptr = cs_get_regchan_from_hash_or_db(service, client, chptr, parv[1]);

  if (regchptr->founder != client->nickname->id)
  {
    reply_user(service, client, CS_OWN_CHANNEL_ONLY, parv[1]);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
    return;
  }

  if ( (cae = db_chan_access_get(regchptr->id, db_get_id_from_nick(parv[2]))) == NULL)
  {
    cae = MyMalloc(sizeof(struct ChannelAccessEntry));
    cae->channel_id = regchptr->id;
    cae->id = 0;
    cae->level = 0;
    cae->nick_id = db_get_id_from_nick(parv[2]);
    update = 0;
  } else
    update = 1;

  if (cae->nick_id == 0)
  {
    reply_user(service, client, CS_FIXME);
    free_regchan(regchptr);
    MyFree(cae);
    return;
  }

  int i;
  for (i = 2; i < parc; i++)
  {
    cae->level = set_access_level_by_name(cae->level, parv[i]);
  }

  if (db_chan_access_add(cae) == 0)
  {
    reply_user(service, client, CS_ACCESS_ADD);
    ilog(L_DEBUG, "%s (%s@%s) added AE %s(%d) to %s", 
      client->name, client->username, client->host, parv[2], cae->level, parv[1]);
  }
  else
  {
    reply_user(service, client, CS_ACCESS_ADD_FAILED, parv[1]);
  }

  if (chptr == NULL)
  {
    free_regchan(regchptr);
  }
  MyFree(cae);
  ilog(L_TRACE, "T: Leaving CS:m_access_add");
}


static void
m_access_del(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  ilog(L_TRACE, "CS ACCESS DEL from %s for %s", client->name, parv[1]);

  struct Channel *chptr;
  struct RegChannel *regchptr;
  int nickid;

  chptr = hash_find_channel(parv[1]);
  regchptr = cs_get_regchan_from_hash_or_db(service, client, chptr, parv[1]);

  if (regchptr->founder != client->nickname->id)
  {
    reply_user(service, client, CS_OWN_CHANNEL_ONLY, parv[1]);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
    return;
  }

  nickid = db_get_id_from_nick(parv[2]);
  if (nickid == 0)
  {
    reply_user(service, client, CS_FIXME);
    free_regchan(regchptr);
    return;
  }


  if (db_chan_access_del(regchptr, nickid) == 0)
  {
    reply_user(service, client, CS_ACCESS_DEL);
    ilog(L_DEBUG, "%s (%s@%s) removed AE %s from %s", 
      client->name, client->username, client->host, parv[2], parv[1]);

  }
  else
  {
    reply_user(service, client, CS_ACCESS_DEL_FAILED, parv[1]);
  }

  if (chptr == NULL)
  {
    free_regchan(regchptr);
  }
  ilog(L_TRACE, "T: Leaving CS:m_access_del");
}

static void
m_access_view(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
}

static void
m_access_list(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  // FIXME: Permissions unchecked here -mc
  struct ChannelAccessEntry *cae;
  struct Channel *chptr;
  struct RegChannel *regchptr;
  void *handle;
  int i = 1;

  reply_user(service, client, CS_ACCESS_START);
  
  chptr = hash_find_channel(parv[1]);
  regchptr = cs_get_regchan_from_hash_or_db(service, client, chptr, parv[1]);

  handle = db_list_first("nickname", CHACCESS_LIST, regchptr->id, 
      (void**)&cae);

  if (handle == NULL)
  {
    reply_user(service, client, CS_ACCESS_EMPTY);
    return;
  }

  while(handle != NULL)
  {
    reply_user(service, client, CS_ACCESS_LIST, i++, cae->nick_id);
    MyFree(cae);
    handle = db_list_next(handle, CHACCESS_LIST, (void **)&cae);
  }
  db_list_done(handle);
  
  if (chptr == NULL)
  {
    free_regchan(regchptr);
  }
}

static void
m_access_count(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
}


/*
 * CHANSERV SET DESC
 */
static void
m_set_desc(struct Service *service, struct Client *client,
    int parc, char *parv[])
{

  ilog(L_TRACE, "Channel SET DESCRIPTION from %s for %s", client->name, parv[1]);

  struct Channel *chptr;
  struct RegChannel *regchptr;
  char desc[512];
  int i;

  memset(desc, 0, sizeof(desc));

  chptr = hash_find_channel(parv[1]);
  regchptr = cs_get_regchan_from_hash_or_db(service, client, chptr, parv[1]);


  if (regchptr->founder != client->nickname->id)
  {
    reply_user(service, client, CS_OWN_CHANNEL_ONLY, parv[1]);
    ilog(L_DEBUG, "Channel SET DESCRIPTION failed for %s on %s (notown)", client->name, parv[1]);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
    return;
  }

  if (parc < 2)
  {
    reply_user(service, client, CS_SET_DESCRIPTION, parv[1], 
        regchptr->description);
    ilog(L_DEBUG, "Channel SET DESCRIPTION for %s on %s (INFOONLY)", client->name, parv[1]);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
    return;
  }

  for (i = 2; parv[i] != '\0'; i++)
  {
    strncat(desc, parv[i], sizeof(desc) - strlen(desc) - 1);
    strncat(desc, " ", 1);
  }

  if (db_set_string("channel",db_get_id_from_chan(parv[1]), "description", desc) == 0)
  {
    reply_user(service, client, CS_SET_DESC, parv[1], desc);
    ilog(L_NOTICE, "%s (%s@%s) changed description of %s to %s", 
      client->name, client->username, client->host, parv[1], desc);

    replace_string(regchptr->description, desc);
  }
  else
  {
    reply_user(service, client, CS_SET_DESC_FAILED, parv[1]);
  }
  if (chptr == NULL)
  {
    free_regchan(regchptr);
  }
  ilog(L_TRACE, "T: Leaving CS:m_set_desc (%s:%s)", client->name, parv[1]);
}

/*
 * CHANSERV SET URL
 */
static void
m_set_url(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  ilog(L_TRACE, "Channel SET URL from %s for %s", client->name, parv[1]);

  struct Channel *chptr;
  struct RegChannel *regchptr;

  chptr = hash_find_channel(parv[1]);
  regchptr = cs_get_regchan_from_hash_or_db(service, client, chptr, parv[1]);


  if (regchptr->founder != client->nickname->id)
  {
    reply_user(service, client, CS_OWN_CHANNEL_ONLY, parv[1]);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
    return;
  }

  if (parc < 2)
  {
    reply_user(service, client, CS_SET_URL, parv[1], regchptr->url);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
    return;
  }

  if (db_set_string("channel",db_get_id_from_chan(parv[1]), "url", parv[2]) == 0)
  {
    reply_user(service, client, CS_SET_URL, parv[1], parv[2]);
    ilog(L_NOTICE, "%s (%s@%s) changed url of %s to %s", 
      client->name, client->username, client->host, parv[1], parv[2]);

    replace_string(regchptr->url, parv[2]);
  }
  else
  {
    reply_user(service, client, CS_SET_URL_FAILED, parv[1]);
  }
  if (chptr == NULL)
  {
    free_regchan(regchptr);
  }
  ilog(L_TRACE, "T: Leaving CS:m_set_url(%s:%s)", client->name, parv[1]);
}

/*
 * CHANSERV SET EMAIL
 */
static void
m_set_email(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  ilog(L_TRACE, "Channel SET EMAIL from %s for %s", client->name, parv[1]);

  struct Channel *chptr;
  struct RegChannel *regchptr;

  chptr = hash_find_channel(parv[1]);
  regchptr = cs_get_regchan_from_hash_or_db(service, client, chptr, parv[1]);


  if (chptr->regchan->id != client->nickname->id)
  {
    reply_user(service, client, CS_OWN_CHANNEL_ONLY, parv[1]);
    if (chptr == NULL);
    {
      free_regchan(regchptr);
    }
    return;
  }

  if (parc < 2)
  {
    reply_user(service, client, CS_SET_EMAIL, parv[1], chptr->regchan->email);
    if (chptr == NULL);
    {
      free_regchan(regchptr);
    }
    return;
  }

  if (db_set_string("channel",db_get_id_from_chan(parv[1]), "email", parv[2]) == 0)
  {
    reply_user(service, client, CS_SET_EMAIL, parv[1], parv[2]);
    ilog(L_NOTICE, "%s (%s@%s) changed email of %s to %s", 
      client->name, client->username, client->host, parv[1], parv[2]);

    replace_string(regchptr->email, parv[2]);
  }
  else
  {
    reply_user(service, client, CS_SET_EMAIL_FAILED, parv[1]);
  }
  if (chptr == NULL);
  {
    free_regchan(regchptr);
  }
  ilog(L_TRACE, "T: Leaving CS:m_set_email(%s:%s)", client->name, parv[1]);
}

/*
 * CHANSERV SET ENTRYMSG
 */
static void
m_set_entrymsg(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  ilog(L_TRACE, "Channel SET ENTRYMSG from %s for %s", client->name, parv[1]);

  struct Channel *chptr;
  struct RegChannel *regchptr;
  int i; char msg[512];

  chptr = hash_find_channel(parv[1]);
  regchptr = cs_get_regchan_from_hash_or_db(service, client, chptr, parv[1]);

  if (regchptr->founder != client->nickname->id)
  {
    reply_user(service, client, CS_OWN_CHANNEL_ONLY, parv[1]);
    if (chptr == NULL)
    {
       free_regchan(regchptr);
    }
    return;
  }

  if (parc < 2)
  {
    reply_user(service, client, CS_SET_ENTRYMSG, parv[1], regchptr->entrymsg);
    if (chptr == NULL)
    {
       free_regchan(regchptr);
    }
    return;
  }

  for (i = 2; parv[i] != '\0'; i++)
  {
    strncat(msg, parv[i], sizeof(msg) - strlen(msg) - 1);
    strncat(msg, " ", 1);
  }

  if (db_set_string("channel",db_get_id_from_chan(parv[1]), "entrymsg", msg) == 0)
  {
    reply_user(service, client, CS_SET_MSG, parv[1], msg);
    ilog(L_NOTICE, "%s (%s@%s) changed entrymsg of %s to %s", 
      client->name, client->username, client->host, parv[1], msg);
    
    replace_string(regchptr->entrymsg, msg);
  }
  else
  {
    reply_user(service, client, CS_SET_MSG_FAILED, parv[1]);
  }

  if (chptr == NULL)
  {
    free_regchan(regchptr);
  }
  ilog(L_TRACE, "T: Leaving CS:m_set_entrymsg(%s:%s)", client->name, parv[1]);
}


/*
 * CHANSERV SET TOPIC
 */
static void
m_set_topic(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  ilog(L_TRACE, "Channel SET TOPIC from %s for %s", client->name, parv[1]);

  struct Channel *chptr;
  struct RegChannel *regchptr;
  int i; char topic[TOPICLEN+1];

  chptr = hash_find_channel(parv[1]);
  regchptr = cs_get_regchan_from_hash_or_db(service, client, chptr, parv[1]);

  if (regchptr->founder != client->nickname->id)
  {
    reply_user(service, client, CS_OWN_CHANNEL_ONLY, parv[1]);
    if (chptr == NULL)
      free_regchan(regchptr);
    return;
  }

  if (parc < 2)
  {
    reply_user(service, client, CS_SET_TOPIC, parv[1], regchptr->topic);
    return;
  }

  for (i = 2; parv[i] != '\0'; i++)
  {
    strncat(topic, parv[i], sizeof(topic) - strlen(topic) - 1);
    strncat(topic, " ", 1);
  }

  if (db_set_string("channel",db_get_id_from_chan(parv[1]), "topic", topic) == 0)
  {
    reply_user(service, client, CS_SET_TOPIC, parv[1], topic);
    ilog(L_NOTICE, "%s (%s@%s) changed TOPIC of %s to %s", 
      client->name, client->username, client->host, parv[1], topic);

    replace_string(regchptr->topic, topic);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }

    // XXX: send topic
  }
  else
  {
    reply_user(service, client, CS_SET_TOPIC_FAILED, parv[1]);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
  }
  ilog(L_TRACE, "T: Leaving CS:m_set_topic(%s:%s)", client->name, parv[1]);
}

/*
 * CHANSERV SET KEEPTOPIC
 */
static void
m_set_keeptopic(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], CHSET_KEEPTOPIC, "KEEPTOPIC");
}

/*
 * CHANSERV SET TOPICLOCK
 */
static void
m_set_topiclock(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], CHSET_TOPICLOCK, "TOPICLOCK");
}

/*
 * CHANSERV SET PRIVATE
 */
static void
m_set_private(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], CHSET_PRIVATE, "PRIVATE");
}

/*
 * CHANSERV SET RESTRICTED
 */
static void
m_set_restricted(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], CHSET_RESTRICTED, "RESTRICTED");
}

/*
 * CHANSERV SET SECURE
 */
static void
m_set_secure(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], CHSET_SECURE, "SECURE");
}

/*
 * CHANSERV SET SECUREOPS
 */
static void
m_set_secureops(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], CHSET_SECUREOPS, "SECUREOPS");
}

/*
 *
 * CHANSERV SET LEAVEOPS
 */
static void
m_set_leaveops(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], CHSET_LEAVEOPS, "LEAVEOPS");
}

/*
 *
 * CHANSERV SET VERBOSE
 */
static void
m_set_verbose(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], CHSET_VERBOSE, "VERBOSE");
}


/*
 * CHANSERV set flag (private)
 */
static int 
m_set_flag(struct Service *service, struct Client *client,
           char *channel, char *toggle, int flag, char *flagname)
{
  ilog(L_TRACE, "Channel SET FLAG from %s for %s", client->name, channel);

  struct Channel *chptr;
  struct RegChannel *regchptr;
  int newflag;

  chptr = hash_find_channel(channel);
  regchptr = cs_get_regchan_from_hash_or_db(service, client, chptr, channel);

  if (regchptr->founder != client->nickname->id)
  {
    reply_user(service, client, CS_OWN_CHANNEL_ONLY, channel);
    if (regchptr)
    {
      free_regchan(regchptr);
    }
    return -1;
  }

  newflag = regchptr->flags;

  if (toggle == NULL)
  {
    reply_user(service, client, CS_SET_FLAG,
      flagname, (newflag & flag) ? "ON" : "OFF", channel);
    if (regchptr)
    {
      free_regchan(regchptr);
    }

    return -1;
  }

  if ( strncasecmp(toggle, "ON", strlen(toggle)) == 0 )
  {
    newflag |= flag;
  }
  else if ( strncasecmp(toggle, "OFF", strlen(toggle)) == 0 )
  {
    newflag &= ~flag;
  }

  if (db_set_number("channel", db_get_id_from_chan(channel), "flags", newflag) == 0 )
  {
    reply_user(service, client, CS_SET_SUCCESS, channel, flagname, toggle);

    regchptr->flags = newflag;
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
  }
  else
  {
    reply_user(service, client, CS_SET_FAILED, flagname, channel);
    if (chptr == NULL)
    {
      free_regchan(regchptr);
    }
  }

  ilog(L_TRACE, "T: Leaving CS:m_set_flag(%s:%s)", client->name, channel);
  return 0;
}


/**
 * @brief Retrieve a struct RegChannel Record from Memory or DB
 * @param service 
 * @param client 
 * @param chptr struct Channel * or NULL
 * @param name name of Channel
 * @return struct RegChannel *
 * At any cost give us the struct RegChannel* associated with name
 * Rationale:  We need it, regardless wether its in Memory or only in DB
 *             (Channel exists in Services DB, but is not used now on the network)
 * So, while we're at it, we can aswell attach the record, if thats possible.
 */
static struct RegChannel*
cs_get_regchan_from_hash_or_db(struct Service *service, struct Client *client, struct Channel *chptr, char *name)
{
  struct RegChannel *regchptr;

  if ( (chptr == NULL) || (chptr->regchan == NULL))
  {
    regchptr = db_find_chan(name);
    if (regchptr == NULL)
    {
      reply_user(service, client, CS_NOT_REG, name);
      return NULL;
    }
    else
    {
      chptr->regchan = regchptr;
    }
  }
  else
  {
    regchptr = chptr->regchan;
  }
  return regchptr;
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
  struct Client  *client_p = va_arg(args, struct Client*);
  struct Client  *source_p = va_arg(args, struct Client*);
  struct Channel *chptr    = va_arg(args, struct Channel*);
  int             parc     = va_arg(args, int);
  char           **parv    = va_arg(args, char **);

  // ... actually do stuff    

  // last function to call in this func
  return pass_callback(cs_cmode_hook, client_p, source_p, chptr, parc, parv);
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

  /* Find Channel in hash */
  if ((chptr = hash_find_channel(name)) != NULL)
  {
    /* regchan attached, good */
    if ( chptr->regchan != NULL)
    {
      /* fetch entrymsg from hash if it exists there */
      if (chptr->regchan->entrymsg != NULL)
        reply_user(chanserv, source_p, 0, chptr->regchan->entrymsg);
    }
    /* regchan not attached, get it from DB */
    else if ((regchptr = db_find_chan(name)) != NULL)
    {
      /* it does exist there, so attach it now */
      if (regchptr->entrymsg != NULL)
        reply_user(chanserv, source_p, 0, regchptr->entrymsg);
      chptr->regchan = regchptr;
    }
  } else
  {
    ilog(L_ERROR, "badbad. Client %s joined non-existing Channel %s\n", 
        source_p->name, chptr->chname);
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
static void*
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
 * - we need to make sure theres no Channel left with the nick as founder
 * - what to do when the nick is successor? (FIXME)
 */
static void*
cs_on_nick_drop(va_list args)
{
  char *nick = va_arg(args, char *);

  db_chan_success_founder(nick);

  return pass_callback(cs_on_nick_drop_hook, nick);
}
