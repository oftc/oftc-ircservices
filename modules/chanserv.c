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

static void *cs_on_cmode_change(va_list);
static void *cs_on_client_join(va_list);
static void *cs_on_channel_destroy(va_list);

static void m_register(struct Service *, struct Client *, int, char *[]);
static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_drop(struct Service *, struct Client *, int, char *[]);
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


/* temp */
static void m_not_avail(struct Service *, struct Client *, int, char *[]);

/* private */
int m_set_flag(struct Service *, struct Client *, char *, char *, int, char *);
struct Channel *cs_find_chan(struct Service *, struct Client *, char *);

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
  { "FOUNDER",     0, 1, CS_SET_FOUNDER_SHORT, CS_SET_FOUNDER_LONG, m_set_founder },
  { "SUCCESSOR",   0, 1, CS_SET_SUCC_SHORT, CS_SET_SUCC_LONG, m_set_successor },
  { "PASSWORD",    0, 1, -1, -1, m_not_avail },
  { "DESC",        0, 1, CS_SET_DESC_SHORT, CS_SET_DESC_LONG, m_set_desc },
  { "URL",         0, 1, CS_SET_URL_SHORT, CS_SET_URL_LONG, m_set_url },
  { "EMAIL",       0, 1, CS_SET_EMAIL_SHORT, CS_SET_EMAIL_LONG, m_set_email },
  { "ENTRYMSG",    0, 1, CS_SET_ENTRYMSG_SHORT, CS_SET_ENTRYMSG_LONG, m_set_entrymsg },
  { "TOPIC",       0, 1, CS_SET_TOPIC_SHORT, CS_SET_TOPIC_LONG, m_set_topic },
  { "KEEPTOPIC",   0, 1, CS_SET_KEEPTOPIC_SHORT, CS_SET_KEEPTOPIC_LONG, m_set_keeptopic },
  { "TOPICLOCK",   0, 1, CS_SET_TOPICLOCK_SHORT, CS_SET_TOPICLOCK_LONG, m_set_topiclock },
  { "MLOCK",       0, 1, -1, -1, m_not_avail }, // +kl-mnt
  { "PRIVATE",     0, 1, CS_SET_PRIVATE_SHORT, CS_SET_PRIVATE_LONG, m_set_private },
  { "RESTRICTED",  0, 1, CS_SET_RESTRICTED_SHORT, CS_SET_RESTRICTED_LONG, m_set_restricted },
  { "SECURE",      0, 1, CS_SET_SECURE_SHORT, CS_SET_SECURE_LONG, m_set_secure },
  { "SECUREOPS",   0, 1, CS_SET_SECUREOPS_SHORT, CS_SET_SECUREOPS_LONG, m_set_secureops },
  { "LEAVEOPS",    0, 1, CS_SET_LEAVEOPS_SHORT, CS_SET_LEAVEOPS_LONG, m_set_leaveops },
  { "VERBOSE",     0, 1, CS_SET_VERBOSE_SHORT, CS_SET_VERBOSE_LONG, m_set_verbose },
  { "AUTOLIMIT",   0, 1, -1, -1, m_not_avail }, // 5:2:2
  { "CLEARBANS",   0, 1, -1, -1, m_not_avail }, // 120
  { "NULL",        0, 0,  0,  0, m_not_avail } 
};

static struct ServiceMessage set_msgtab = {
  set_sub, "SET", 0, 0, CS_SET_SHORT, CS_SET_LONG,
  { m_unreg, m_set, m_set, m_set }
};

static struct SubMessage access_sub[6] = {
  { "ADD",   0, 3, -1, -1, m_not_avail },
  { "DEL",   0, 2, -1, -1, m_not_avail },
  { "LIST",  0, 2, -1, -1, m_not_avail },
  { "VIEW",  0, 1, -1, -1, m_not_avail },
  { "COUNT", 0, 0, -1, -1, m_not_avail },
  { "NULL",  0, 0,  0,  0, NULL }
};

static struct ServiceMessage access_msgtab = {
  access_sub, "ACCESS", 0, 0, -1, -1, 
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct SubMessage levels_sub[6] = {
  { "SET",      0, 3, -1, -1, m_not_avail },
  { "LIST",     0, 0, -1, -1, m_not_avail },
  { "RESET",    0, 0, -1, -1, m_not_avail },
  { "DIS",      0, 1, -1, -1, m_not_avail },
  { "DISABLED", 0, 1, -1, -1, m_not_avail },
  { "NULL",     0, 0,  0,  0, NULL }
};

static struct ServiceMessage levels_msgtab = {
  levels_sub, "LEVELS", 0, 0, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct SubMessage akick_sub[7] = {
  { "ADD",     0, 2, -1, -1, m_not_avail }, 
  { "DEL",     0, 1, -1, -1, m_not_avail },
  { "LIST",    0, 1, -1, -1, m_not_avail },
  { "VIEW",    0, 1, -1, -1, m_not_avail },
  { "ENFORCE", 0, 0, -1, -1, m_not_avail },
  { "COUNT",   0, 0, -1, -1, m_not_avail },
  { "NULL",    0, 0,  0,  0, NULL }
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


/*
INFO
LIST
INVITE
OP
DEOP
UNBAN
CLEAR
VOP
AOP
SOP
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
  cs_cmode_hook = install_hook(on_cmode_change_cb, cs_on_cmode_change);
  cs_join_hook  = install_hook(on_join_cb, cs_on_client_join);
  cs_channel_destroy_hook = 
       install_hook(on_channel_destroy_cb, cs_on_channel_destroy);
}

CLEANUP_MODULE
{
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
  struct Channel    *chptr;
  struct RegChannel *regchptr;

  assert(parv[1]);

  // Bail out if channelname does not start with a hash
  if ( *parv[1] != '#' )
  {
    reply_user(service, client, _L(chanserv, client, CS_NAMESTART_HASH));
    return;
  }

  // Bail out if services dont know the channel (it does not exist)
  // or if client is no member of the channel
  chptr = hash_find_channel(parv[1]);
  if ((chptr == NULL) || (! IsMember(client, chptr))) 
  {
    reply_user(service, client, _L(chanserv, client, CS_NOT_ONCHAN));
    return;
  }
  
  // bail out if client is not opped on channel
  if (! IsChanop(client, chptr))
  {
    reply_user(service, client, _L(chanserv, client, CS_NOT_OPPED));
    return;
  }

  // finally, bail out if channel is already registered
  regchptr = db_find_chan(parv[1]);
  if (regchptr != NULL)
  {
    reply_user(service, client, _L(chanserv, client, CS_ALREADY_REG), parv[1]);
    free_regchan(regchptr);
    MyFree(regchptr);
    return;
  }

  if (db_register_chan(client, parv[1]) == 0)
  {
    chptr->regchan = db_find_chan(parv[1]);
    reply_user(service, client, _L(chanserv, client, CS_REG_SUCCESS), parv[1]);
    global_notice(NULL, "%s (%s@%s) registered channel %s", 
      client->name, client->username, client->host, parv[1]);
  }
  else
  {
    reply_user(service, client, _L(chanserv, client, CS_REG_FAIL), parv[1]);
  }
}


/* 
 * CHANSERV DROP
 */
static void 
m_drop(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Channel *chptr;

  assert(parv[1]);

  if ((chptr = cs_find_chan(service, client, parv[1])) == NULL)
    return;
  
  if (chptr->regchan->founder != client->nickname->id)
  {
    reply_user(service, client, _L(chanserv, client, CS_OWN_CHANNEL_ONLY), parv[1]);
    return;
  }

  if (db_delete_chan(parv[1]) == 0)
  {
    reply_user(service, client, _L(chanserv, client, CS_DROPPED), parv[1]);
    global_notice(NULL, "%s (%s@%s) dropped channel %s", 
      client->name, client->username, client->host, parv[1]);

    free_regchan(chptr->regchan);
    MyFree(chptr->regchan);
    chptr->regchan = NULL;
  } else
  {
    reply_user(service, client, _L(chanserv, client, CS_DROP_FAILED), parv[1]);
  }
  return;
}

/* XXX temp XXX */
static void
m_not_avail(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  reply_user(service, client, "This function is currently not implemented."
    "   Bug the Devs! ;-)");
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
  reply_user(service, client, _L(chanserv, client, CS_SET_LONG));
}


/*
 * CHANSERV SET FOUNDER
 */
static void
m_set_founder(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Channel *chptr;
  struct Nick *nick_p;
  char *foundernick;

  if ((chptr = cs_find_chan(service, client, parv[1])) == NULL)
    return;

  if (chptr->regchan->founder != client->nickname->id)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_OWN_CHANNEL_ONLY), parv[1]);
    return;
  }

  if (parc < 2)
  {
    foundernick = db_get_nickname_from_id(chptr->regchan->founder);
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_FOUNDER), parv[1], foundernick);
    MyFree(foundernick);
    return;
  }

  /* we need to consult the db, since the nick may not be online -mc */
  if ((nick_p = db_find_nick(parv[2])) == NULL)
  {
    reply_user(service, client, _L(chanserv, client, CS_REGISTER_NICK), parv[2]);
    return;
  }
  free_nick(nick_p);
  MyFree(nick_p);


  if (db_set_founder(parv[1], parv[2]) == 0)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_FOUNDER), parv[1], parv[2]);
    global_notice(NULL, "%s (%s@%s) set founder of %s to %s", 
      client->name, client->username, client->host, parv[1], parv[2]);
    chptr->regchan->founder = db_get_id_from_nick(parv[2]); // correct? -mc
  }
  else
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_FOUNDER_FAILED), parv[1], parv[2]);
  }
}


/*
 * CHANSERV SET SUCCESSOR
 */
static void
m_set_successor(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Channel *chptr;
  struct Nick *nick_p;
  char *successornick;

  if ((chptr = cs_find_chan(service, client, parv[1])) == NULL)
    return;

  if (chptr->regchan->founder != client->nickname->id)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_OWN_CHANNEL_ONLY), parv[1]);
    return;
  }

  if (parc < 2)
  {
    successornick = db_get_nickname_from_id(chptr->regchan->successor);
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_SUCCESSOR), parv[1], successornick);
    MyFree(successornick);
    return;
  }

  if ((nick_p = db_find_nick(parv[2])) == NULL)
  {
    reply_user(service, client, _L(chanserv, client, CS_REGISTER_NICK), parv[2]);
    return;
  } 

  free_nick(nick_p);
  MyFree(nick_p);

  if (db_set_successor(parv[1], parv[2]) == 0)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_SUCC), parv[1], parv[2]);
    global_notice(NULL, "%s (%s@%s) set successor of %s to %s", 
      client->name, client->username, client->host, parv[1], parv[2]);
   chptr->regchan->successor = db_get_id_from_nick(parv[2]); // XXX correct? -mc
  }
  else
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_SUCC_FAILED), parv[1], parv[2]);
  }
}


/*
 * CHANSERV SET DESC
 */
static void
m_set_desc(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  char desc[512];
  int i;

  memset(desc, 0, sizeof(desc));

  if ((chptr = cs_find_chan(service, client, parv[1])) == NULL)
    return;

  if (chptr->regchan->founder != client->nickname->id)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_OWN_CHANNEL_ONLY), parv[1]);
    return;
  }

  if (parc < 2)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_DESCRIPTION), 
        parv[1], chptr->regchan->description);
    return;
  }

  for (i = 2; parv[i] != '\0'; i++)
  {
    strncat(desc, parv[i], sizeof(desc) - strlen(desc) - 1);
    strncat(desc, " ", 1);
  }

  if (db_chan_set_string(db_get_id_from_chan(parv[1]), "description", desc) == 0)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_DESC), parv[1], desc);
    global_notice(NULL, "%s (%s@%s) changed description of %s to %s", 
      client->name, client->username, client->host, parv[1], desc);

    if (chptr->regchan->description != NULL)
      MyFree(chptr->regchan->description);
    DupString(chptr->regchan->description, desc);
  }
  else
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_DESC_FAILED), parv[1]);
  }
}

/*
 * CHANSERV SET URL
 */
static void
m_set_url(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;

  if ((chptr = cs_find_chan(service, client, parv[1])) == NULL)
    return;

  if (chptr->regchan->founder != client->nickname->id)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_OWN_CHANNEL_ONLY), parv[1]);
    return;
  }

  if (parc < 2)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_URL), 
        parv[1], chptr->regchan->url);
    return;
  }

  if (db_chan_set_string(db_get_id_from_chan(parv[1]), "url", parv[2]) == 0)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_URL), parv[1], parv[2]);
    global_notice(NULL, "%s (%s@%s) changed url of %s to %s", 
      client->name, client->username, client->host, parv[1], parv[2]);

    if (chptr->regchan->url != NULL)
      MyFree(chptr->regchan->url);
    DupString(chptr->regchan->url, parv[2]);
  }
  else
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_URL_FAILED), parv[1]);
  }
}

/*
 * CHANSERV SET EMAIL
 */
static void
m_set_email(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;

  if (( chptr = cs_find_chan(service, client, parv[1])) == NULL)
    return;

  if (chptr->regchan->id != client->nickname->id)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_OWN_CHANNEL_ONLY), parv[1]);
    return;
  }

  if (parc < 2)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_EMAIL), 
        parv[1], chptr->regchan->email);
    return;
  }

  if (db_chan_set_string(db_get_id_from_chan(parv[1]), "email", parv[2]) == 0)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_EMAIL), parv[1], parv[2]);
    global_notice(NULL, "%s (%s@%s) changed email of %s to %s", 
      client->name, client->username, client->host, parv[1], parv[2]);

    if (chptr->regchan->email != NULL)
      MyFree(chptr->regchan->email);
    DupString(chptr->regchan->email, parv[2]);
  }
  else
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_EMAIL_FAILED), parv[1]);
  }
}

/*
 * CHANSERV SET ENTRYMSG
 */
static void
m_set_entrymsg(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  int i; char msg[512];

  if ((chptr = cs_find_chan(service, client, parv[1])) == NULL)
    return;

  if (chptr->regchan->founder != client->nickname->id)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_OWN_CHANNEL_ONLY), parv[1]);
    return;
  }

  if (parc < 2)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_ENTRYMSG),
        parv[1], chptr->regchan->entrymsg);
    return;
  }

  for (i = 2; parv[i] != '\0'; i++)
  {
    strncat(msg, parv[i], sizeof(msg) - strlen(msg) - 1);
    strncat(msg, " ", 1);
  }

  if (db_chan_set_string(db_get_id_from_chan(parv[1]), "entrymsg", msg) == 0)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_MSG), parv[1], msg);
    global_notice(NULL, "%s (%s@%s) changed entrymsg of %s to %s", 
      client->name, client->username, client->host, parv[1], msg);
    
    if (chptr->regchan->entrymsg != NULL)
      MyFree(chptr->regchan->entrymsg);
    DupString(chptr->regchan->entrymsg, msg);
  }
  else
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_MSG_FAILED), parv[1]);
  }
}

/*
 * CHANSERV SET TOPIC
 */
static void
m_set_topic(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  int i; char topic[TOPICLEN+1];

  if ((chptr = cs_find_chan(service, client, parv[1])) == NULL)
    return;

  if (chptr->regchan->founder != client->nickname->id)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_OWN_CHANNEL_ONLY), parv[1]);
    return;
  }

  if (parc < 2)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_TOPIC),
        parv[1], chptr->regchan->topic);
    return;
  }

  for (i = 2; parv[i] != '\0'; i++)
  {
    strncat(topic, parv[i], sizeof(topic) - strlen(topic) - 1);
    strncat(topic, " ", 1);
  }

  if (db_chan_set_string(db_get_id_from_chan(parv[1]), "topic", topic) == 0)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_TOPIC), parv[1], topic);
    global_notice(NULL, "%s (%s@%s) changed TOPIC of %s to %s", 
      client->name, client->username, client->host, parv[1], topic);

    if (chptr->regchan->topic != NULL)
      MyFree(chptr->regchan->topic);
    DupString(chptr->regchan->topic, topic);

    // XXX: send topic
  }
  else
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_TOPIC_FAILED), parv[1]);
  }
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
int 
m_set_flag(struct Service *service, struct Client *client,
           char *channel, char *toggle, int flag, char *flagname)
{
  struct Channel *chptr;
  int newflag;

  if ((chptr = cs_find_chan(service, client, channel)) == NULL)
    return -1;

  if (chptr->regchan->founder != client->nickname->id)
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_OWN_CHANNEL_ONLY), channel);
    return -1;
  }

  newflag = chptr->regchan->flags;

  if (toggle == NULL)
  {
    reply_user(service, client, _L(chanserv, client, CS_SET_FLAG),
      flagname, (newflag & flag) ? "ON" : "OFF", channel);

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

  if (db_chan_set_number(db_get_id_from_chan(channel), "flags", newflag) == 0 )
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_SUCCESS), channel, flagname, toggle);

    chptr->regchan->flags = newflag;
  }
  else
  {
    reply_user(service, client, 
        _L(chanserv, client, CS_SET_FAILED), flagname, channel);
  }

  return 0;
}



/*
 * EVENT: Channel Mode Change
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


/* 
 * EVENT: Client Joins Channel
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
        reply_user(chanserv, source_p, chptr->regchan->entrymsg);
    }
    /* regchan not attached, get it from DB */
    else if ((regchptr = db_find_chan(name)) != NULL)
    {
      /* it does exist there, so attach it now */
      if (regchptr->entrymsg != NULL)
        reply_user(chanserv, source_p, regchptr->entrymsg);
      chptr->regchan = regchptr;
    }
  } else
  {
    printf("badbad. Client %s joined non-existing Channel %s\n", 
        source_p->name, chptr->chname);
  }

  return pass_callback(cs_join_hook, source_p, name);
}

static void*
cs_on_channel_destroy(va_list args)
{
  struct Channel *chan = va_arg(args, struct Channel *);

  /* Free regchan from chptr before freeing chptr */
  free_regchan(chan->regchan);
  MyFree(chan->regchan);
  chan->regchan = NULL;

  return pass_callback(cs_channel_destroy_hook, chan);
}


/*
 * find a Channel/Regchannel from hash_find_chan(name)->regchan
 * or complete from db_find_chan(name)
 * return: struct Channel * for completeness
 */
struct Channel *
cs_find_chan(struct Service *service, struct Client *client, char *name)
{
  struct Channel *channel;
  struct RegChannel *regchptr;

  if ((channel = hash_find_channel(name)) == NULL)
  {
    reply_user(service, client, _L(chanserv, client, CS_NOT_EXIST), name);
    return NULL;
  }

  if ( channel->regchan == NULL)
  {
    regchptr = db_find_chan(name);
    if (regchptr == NULL)
    {
      reply_user(service, client, _L(chanserv, client, CS_NOT_REG), name);
      return NULL;
    }
    else
    {
      channel->regchan = regchptr;
    }
  }

  return channel;
}
