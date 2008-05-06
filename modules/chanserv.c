/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  chanserv.c: A C implementation of Channel Services
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
#include "client.h"
#include "chanserv.h"
#include "nickname.h"
#include "dbchannel.h"
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
#include "akick.h"
#include "chanaccess.h"
#include "servicemask.h"

static struct Service *chanserv = NULL;
static struct Client *chanserv_client = NULL;

static dlink_node *cs_cmode_hook;
static dlink_node *cs_join_hook;
static dlink_node *cs_channel_destroy_hook;
static dlink_node *cs_channel_create_hook;
static dlink_node *cs_on_topic_change_hook;
static dlink_node *cs_on_burst_done_hook;

static dlink_list channel_limit_list = { NULL, NULL, 0 };
static dlink_list channel_expireban_list = { NULL, NULL, 0 };

static void process_limit_list(void *);
static void process_expireban_list(void *);

static void *cs_on_cmode_change(va_list);
static void *cs_on_client_join(va_list);
static void *cs_on_channel_destroy(va_list);
static void *cs_on_channel_create(va_list);
static void *cs_on_topic_change(va_list);
static void *cs_on_burst_done(va_list);

static void m_register(struct Service *, struct Client *, int, char *[]);
static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_drop(struct Service *, struct Client *, int, char *[]);
static void m_info(struct Service *, struct Client *, int, char *[]);
static void m_sudo(struct Service *, struct Client *, int, char *[]);
static void m_list(struct Service *, struct Client *, int, char *[]);
static void m_forbid(struct Service *, struct Client *, int, char *[]);
static void m_unforbid(struct Service *, struct Client *, int, char *[]);

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
static void m_set_floodserv(struct Service *, struct Client *, int, char *[]);
static void m_set_mlock(struct Service *, struct Client *, int, char *[]);
static void m_set_autoop(struct Service *, struct Client *, int, char *[]);
static void m_set_autovoice(struct Service *, struct Client *, int, char *[]);
static void m_set_leaveops(struct Service *, struct Client *, int, char *[]);

static void m_akick_add(struct Service *, struct Client *, int, char *[]);
static void m_akick_list(struct Service *, struct Client *, int, char *[]);
static void m_akick_del(struct Service *, struct Client *, int, char *[]);
static void m_akick_enforce(struct Service *, struct Client *, int, char *[]);

static void m_clear_bans(struct Service *, struct Client *, int, char *[]);
static void m_clear_quiets(struct Service *, struct Client *, int, char *[]);
static void m_clear_ops(struct Service *, struct Client *, int, char *[]);
static void m_clear_voices(struct Service *, struct Client *, int, char *[]);
static void m_clear_modes(struct Service *, struct Client *, int, char *[]);
static void m_clear_users(struct Service *, struct Client *, int, char *[]);

static void m_op(struct Service *, struct Client *, int, char *[]);
static void m_deop(struct Service *, struct Client *, int, char *[]);
static void m_voice(struct Service *, struct Client *, int, char *[]);
static void m_devoice(struct Service *, struct Client *, int, char *[]);
static void m_invite(struct Service *, struct Client *, int, char *[]);
static void m_unban(struct Service *, struct Client *, int, char *[]);
static void m_unquiet(struct Service *, struct Client *, int, char *[]);
static void m_list(struct Service *, struct Client *, int, char *[]);

static void m_access_add(struct Service *, struct Client *, int, char *[]);
static void m_access_del(struct Service *, struct Client *, int, char *[]);
static void m_access_list(struct Service *, struct Client *, int, char *[]);

static void m_invites_add(struct Service *, struct Client *, int, char *[]);
static void m_invites_del(struct Service *, struct Client *, int, char *[]);
static void m_invites_list(struct Service *, struct Client *, int, char *[]);

static void m_excepts_add(struct Service *, struct Client *, int, char *[]);
static void m_excepts_del(struct Service *, struct Client *, int, char *[]);
static void m_excepts_list(struct Service *, struct Client *, int, char *[]);

static void m_quiets_add(struct Service *, struct Client *, int, char *[]);
static void m_quiets_del(struct Service *, struct Client *, int, char *[]);
static void m_quiets_list(struct Service *, struct Client *, int, char *[]);

/* private */
static int m_set_flag(struct Service *, struct Client *, char *, char *, char *,
    char (*)(DBChannel *), int (*)(DBChannel *, char));
static int m_set_string(struct Service *, struct Client *, const char *,
    const char *, const char *, int, const char *(*)(DBChannel *),
    int(*)(DBChannel *, const char*));
static void m_delete_autolimit(struct Channel *chptr);
static int m_mask_add(struct Client *, int, char *[], const char *,
  int (*)(const char *, unsigned int, unsigned int, unsigned int, unsigned int, const char *),
  void (*)(struct Service *, struct Channel *, const char *));
static void m_mask_list(struct Client *, const char *, const char *,
  int (*)(unsigned int, dlink_list *));
static int m_mask_del(struct Client *, const char *, const char *, const char *,
  int (*) (unsigned int, const char *),
  void (*) (struct Service *, struct Channel *, const char *));

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
  { NULL, "MLOCK", 0, 1, 4, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
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
  { NULL, "FLOODSERV", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG,
    CS_HELP_SET_FLOODSERV_SHORT, CS_HELP_SET_FLOODSERV_LONG, m_set_floodserv },
  { NULL, "AUTOOP", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG,
    CS_HELP_SET_AUTOOP_SHORT, CS_HELP_SET_AUTOOP_LONG, m_set_autoop },
  { NULL, "AUTOVOICE", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG,
    CS_HELP_SET_AUTOVOICE_SHORT, CS_HELP_SET_AUTOVOICE_LONG, m_set_autovoice },
  { NULL, "LEAVEOPS", 0, 1, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG,
    CS_HELP_SET_LEAVEOPS_SHORT, CS_HELP_SET_LEAVEOPS_LONG, m_set_leaveops },
  { NULL, NULL, 0, 0, 0, SFLG_KEEPARG|SFLG_CHANARG, 0, 0, 
    SFLG_KEEPARG|SFLG_CHANARG, NULL } 
};

static struct ServiceMessage set_msgtab = {
  set_sub, "SET", 0, 2, 2, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
  CS_HELP_SET_SHORT, CS_HELP_SET_LONG, NULL
};

static struct ServiceMessage access_sub[6] = {
  { NULL, "ADD", 0, 3, 3, SFLG_KEEPARG|SFLG_CHANARG, MASTER_FLAG, 
    CS_HELP_ACCESS_ADD_SHORT, CS_HELP_ACCESS_ADD_LONG, m_access_add },
  { NULL, "DEL", 0, 2, 2, SFLG_KEEPARG|SFLG_CHANARG, MEMBER_FLAG, 
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

static struct ServiceMessage invites_sub[] = {
  { NULL, "ADD", 0, 2, 3, SFLG_NOMAXPARAM|SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG,
    CS_HELP_INVITES_ADD_SHORT, CS_HELP_INVITES_ADD_LONG, m_invites_add },
  { NULL, "DEL", 0, 2, 3, SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG,
    CS_HELP_INVITES_DEL_SHORT, CS_HELP_INVITES_DEL_LONG, m_invites_del },
  { NULL, "LIST", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, MEMBER_FLAG,
    CS_HELP_INVITES_LIST_SHORT, CS_HELP_INVITES_LIST_LONG, m_invites_list },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage invites_msgtab = {
  invites_sub, "INVITES", 0, 2, 2, SFLG_KEEPARG|SFLG_CHANARG, MEMBER_FLAG,
  CS_HELP_INVITES_SHORT, CS_HELP_INVITES_LONG, NULL
};

static struct ServiceMessage excepts_sub[] = {
  { NULL, "ADD", 0, 2, 3, SFLG_NOMAXPARAM|SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG,
    CS_HELP_EXCEPTS_ADD_SHORT, CS_HELP_EXCEPTS_ADD_LONG, m_excepts_add },
  { NULL, "DEL", 0, 2, 3, SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG,
    CS_HELP_EXCEPTS_DEL_SHORT, CS_HELP_EXCEPTS_DEL_LONG, m_excepts_del },
  { NULL, "LIST", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, MEMBER_FLAG,
    CS_HELP_EXCEPTS_LIST_SHORT, CS_HELP_EXCEPTS_LIST_LONG, m_excepts_list },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage excepts_msgtab = {
  excepts_sub, "EXCEPTS", 0, 2, 2, SFLG_KEEPARG|SFLG_CHANARG, MEMBER_FLAG,
  CS_HELP_EXCEPTS_SHORT, CS_HELP_EXCEPTS_LONG, NULL
};

static struct ServiceMessage quiets_sub[] = {
  { NULL, "ADD", 0, 2, 3, SFLG_NOMAXPARAM|SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG,
    CS_HELP_QUIETS_ADD_SHORT, CS_HELP_QUIETS_ADD_LONG, m_quiets_add },
  { NULL, "DEL", 0, 2, 3, SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG,
    CS_HELP_QUIETS_DEL_SHORT, CS_HELP_QUIETS_DEL_LONG, m_quiets_del },
  { NULL, "LIST", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, MEMBER_FLAG,
    CS_HELP_QUIETS_LIST_SHORT, CS_HELP_QUIETS_LIST_LONG, m_quiets_list },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage quiets_msgtab = {
  quiets_sub, "QUIETS", 0, 2, 2, SFLG_KEEPARG|SFLG_CHANARG, MEMBER_FLAG,
  CS_HELP_QUIETS_SHORT, CS_HELP_QUIETS_LONG, NULL
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

static struct ServiceMessage voice_msgtab = {
  NULL, "VOICE", 0, 1, 2, SFLG_CHANARG, MEMBER_FLAG, 
  CS_HELP_VOICE_SHORT, CS_HELP_VOICE_LONG, m_voice
};

static struct ServiceMessage devoice_msgtab = {
  NULL, "DEVOICE", 0, 1, 2, SFLG_CHANARG, MEMBER_FLAG, 
  CS_HELP_DEVOICE_SHORT, CS_HELP_DEVOICE_LONG, m_devoice
};

static struct ServiceMessage unban_msgtab = {
  NULL, "UNBAN", 0, 1, 1, SFLG_CHANARG, MEMBER_FLAG, 
  CS_HELP_UNBAN_SHORT, CS_HELP_UNBAN_LONG, m_unban
};

static struct ServiceMessage unquiet_msgtab = {
  NULL, "UNQUIET", 0, 1, 1, SFLG_CHANARG, MEMBER_FLAG,
  CS_HELP_UNQUIET_SHORT, CS_HELP_UNQUIET_LONG, m_unquiet
};

static struct ServiceMessage invite_msgtab = {
  NULL, "INVITE", 0, 1, 2, SFLG_CHANARG, MEMBER_FLAG,
  CS_HELP_INVITE_SHORT, CS_HELP_INVITE_LONG, m_invite
};

static struct ServiceMessage clear_sub[] = {
  { NULL, "BANS", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG, 
    CS_HELP_CLEAR_BANS_SHORT, CS_HELP_CLEAR_BANS_LONG, m_clear_bans },
  { NULL, "QUIETS", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG,
    CS_HELP_CLEAR_QUIETS_SHORT, CS_HELP_CLEAR_QUIETS_LONG, m_clear_quiets },
  { NULL, "OPS", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG, 
    CS_HELP_CLEAR_OPS_SHORT, CS_HELP_CLEAR_OPS_LONG, m_clear_ops },
  { NULL, "VOICES", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG, 
    CS_HELP_CLEAR_VOICES_SHORT, CS_HELP_CLEAR_VOICES_LONG, m_clear_voices },
  { NULL, "MODES", 0, 1, 1, SFLG_KEEPARG|SFLG_CHANARG, CHANOP_FLAG,
    CS_HELP_CLEAR_MODES_SHORT, CS_HELP_CLEAR_MODES_LONG, m_clear_modes },
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

static struct ServiceMessage list_msgtab = {
  NULL, "LIST", 0, 1, 2, 0, USER_FLAG,
  CS_HELP_LIST_SHORT, CS_HELP_LIST_LONG, m_list
};

static struct ServiceMessage forbid_msgtab = {
  NULL, "FORBID", 0, 1, 2, 0, ADMIN_FLAG,
  CS_HELP_FORBID_SHORT, CS_HELP_FORBID_LONG, m_forbid
};

static struct ServiceMessage unforbid_msgtab = {
  NULL, "UNFORBID", 0, 1, 1, 0, ADMIN_FLAG,
  CS_HELP_UNFORBID_SHORT, CS_HELP_UNFORBID_LONG, m_unforbid
};

INIT_MODULE(chanserv, "$Revision$")
{
  chanserv = make_service("ChanServ");
  clear_serv_tree_parse(&chanserv->msg_tree);
  dlinkAdd(chanserv, &chanserv->node, &services_list);
  hash_add_service(chanserv);
  chanserv_client = introduce_client(chanserv->name, chanserv->name, TRUE);
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
  mod_add_servcmd(&chanserv->msg_tree, &voice_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &devoice_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &invite_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &clear_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &unban_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &unquiet_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &invite_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &sudo_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &list_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &forbid_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &unforbid_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &invites_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &excepts_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &quiets_msgtab);

  cs_cmode_hook = install_hook(on_cmode_change_cb, cs_on_cmode_change);
  cs_join_hook  = install_hook(on_join_cb, cs_on_client_join);
  cs_channel_destroy_hook = install_hook(on_channel_destroy_cb, 
      cs_on_channel_destroy);

  cs_channel_create_hook = install_hook(on_channel_created_cb, 
      cs_on_channel_create);
  cs_on_topic_change_hook = install_hook(on_topic_change_cb, cs_on_topic_change);
  cs_on_burst_done_hook = install_hook(on_burst_done_cb, cs_on_burst_done);

  eventAdd("process channel autolimits", process_limit_list, NULL, 10);
  eventAdd("process channel expirebans", process_expireban_list, NULL, 10);
  return chanserv;
}

CLEANUP_MODULE
{
  uninstall_hook(on_cmode_change_cb, cs_on_cmode_change);
  uninstall_hook(on_join_cb, cs_on_client_join);
  uninstall_hook(on_channel_destroy_cb, cs_on_channel_destroy);
  uninstall_hook(on_channel_created_cb, cs_on_channel_create);
  uninstall_hook(on_topic_change_cb, cs_on_topic_change);
  uninstall_hook(on_burst_done_cb, cs_on_burst_done);

  serv_clear_messages(chanserv);

  unload_languages(chanserv->languages);

  exit_client(chanserv_client, &me, "Service unloaded");
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

    if(chptr->limit_time < CurrentTime)
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
      time_t delta = CurrentTime - banptr->when;
      
      if(delta > dbchannel_get_expirebans_lifetime(chptr->regchan))
      {
        snprintf(ban, IRC_BUFSIZE, "%s!%s@%s", banptr->name, banptr->username,
            banptr->host);
        ilog(L_DEBUG, "ChanServ ExpireBan: BAN %s %d %s", chptr->chname,
          (int)delta, ban);
        unban_mask(chanserv, chptr, ban);
      }
    }

    DLINK_FOREACH_SAFE(bptr, bnptr, chptr->quietlist.head)
    {
      struct Ban *banptr = bptr->data;
      char ban[IRC_BUFSIZE+1];
      time_t delta = CurrentTime - banptr->when;

      if(delta > dbchannel_get_expirebans_lifetime(chptr->regchan))
      {
        snprintf(ban, IRC_BUFSIZE, "%s!%s@%s", banptr->name, banptr->username,
            banptr->host);
        ilog(L_DEBUG, "ChanServ ExpireBan: QUIET %s %d %s", chptr->chname,
          (int)delta, ban);
        unquiet_mask(chanserv, chptr, ban);
      }
    }
  }
}

static void 
m_register(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Channel    *chptr;
  DBChannel *regchptr;
  char desc[IRC_BUFSIZE+1];

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

  /* bail out if channel is already registered */
  if (chptr->regchan != NULL)
  {
    reply_user(service, service, client, CS_ALREADY_REG, parv[1]);
    ilog(L_DEBUG, "Channel REG failed for %s on %s (exists)", client->name, 
        parv[1]);
    return;
  }

  /* finally, bail if this is a forbidden channel */
  if(dbchannel_is_forbid(parv[1]))
  {
    reply_user(service, service, client, CS_NOREG_FORBID, parv[1]);
    ilog(L_DEBUG, "Channel REG failed for %s on %s (forbidden)", client->name, 
        parv[1]);
    return;
  }

  regchptr = dbchannel_new();
  dbchannel_set_channel(regchptr, parv[1]);
  join_params(desc, parc-1, &parv[2]);
  dbchannel_set_description(regchptr, desc);

  if(dbchannel_register(regchptr, client->nickname))
  {
    chptr->regchan = regchptr;
    reply_user(service, service, client, CS_REG_SUCCESS, parv[1]);
    ilog(L_NOTICE, "%s!%s@%s registered channel %s", 
        client->name, client->username, client->host, parv[1]);
    execute_callback(on_chan_reg_cb, client, chptr);
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
  DBChannel *regchptr;

  assert(parv[1]);

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? dbchannel_find(parv[1]) : chptr->regchan;

  if(dbchannel_delete(regchptr))
  {
    reply_user(service, service, client, CS_DROPPED, parv[1]);
    ilog(L_NOTICE, "%s!%s@%s dropped channel %s", 
      client->name, client->username, client->host, parv[1]);

    dbchannel_free(regchptr);
    if(chptr != NULL)
      chptr->regchan = NULL;
    regchptr = NULL;
  } 
  else
  {
    ilog(L_DEBUG, "Channel DROP failed for %s on %s", client->name, parv[1]);
    reply_user(service, service, client, CS_DROP_FAILED, parv[1]);
  }

  if(chptr == NULL && regchptr != NULL)
    dbchannel_free(regchptr);
}

static void
m_info(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  DBChannel *regchptr;
  char buf[IRC_BUFSIZE+1] = {0};
  char *nick;
  struct ChanAccess *access = NULL;
  dlink_node *ptr;
  dlink_list list = { 0 };

  if(dbchannel_is_forbid(parv[1]))
  {
    reply_user(service, service, client, CS_NOREG_FORBID, parv[1]);
    return;
  }

  regchptr = dbchannel_find(parv[1]);

  if(client->nickname)
    access = chanaccess_find(dbchannel_get_id(regchptr), nickname_get_id(client->nickname));

  reply_user(service, service, client, CS_INFO_CHAN_START, dbchannel_get_channel(regchptr));
  reply_time(service, client, CS_INFO_REGTIME_FULL, dbchannel_get_regtime(regchptr));
  if(IsOper(client) || (access != NULL && access->level >= MEMBER_FLAG))
  {
    reply_user(service, service, client, CS_INFO_CHAN_MEMBER,
        dbchannel_get_description(regchptr),
        dbchannel_get_url(regchptr) == NULL ? "Not Set" : dbchannel_get_url(regchptr),
        dbchannel_get_email(regchptr) == NULL ? "Not Set" : dbchannel_get_email(regchptr),
        dbchannel_get_topic(regchptr) == NULL ? "Not Set" : dbchannel_get_topic(regchptr),
        dbchannel_get_entrymsg(regchptr) == NULL ? "Not Set" : dbchannel_get_entrymsg(regchptr),
        dbchannel_get_mlock(regchptr) == NULL ? "Not Set" : dbchannel_get_mlock(regchptr));
  }
  else
  {
    reply_user(service, service, client, CS_INFO_CHAN_NMEMBER,
        dbchannel_get_description(regchptr),
        dbchannel_get_url(regchptr) == NULL ? "Not Set" : dbchannel_get_url(regchptr),
        dbchannel_get_email(regchptr) == NULL ? "Not Set" : dbchannel_get_email(regchptr));
  }

  if(dbchannel_masters_list(dbchannel_get_id(regchptr), &list))
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
    dbchannel_masters_list_free(&list);
  }

  reply_user(service, service, client, CS_INFO_MASTERS, buf);

  reply_user(service, service, client, CS_INFO_OPTION, "TOPICLOCK",
      dbchannel_get_topic_lock(regchptr) ? "ON" : "OFF");

  reply_user(service, service, client, CS_INFO_OPTION, "PRIVATE",
      dbchannel_get_priv(regchptr) ? "ON" : "OFF");

  reply_user(service, service, client, CS_INFO_OPTION, "RESTRICTED",
      dbchannel_get_restricted(regchptr) ? "ON" : "OFF");

  reply_user(service, service, client, CS_INFO_OPTION, "VERBOSE",
      dbchannel_get_restricted(regchptr) ? "ON" : "OFF");

  reply_user(service, service, client, CS_INFO_OPTION, "AUTOLIMIT",
      dbchannel_get_autolimit(regchptr) ? "ON" : "OFF");

  reply_user(service, service, client, CS_INFO_OPTION, "EXPIREBANS",
      dbchannel_get_expirebans(regchptr) ? "ON" : "OFF");

  reply_user(service, service, client, CS_INFO_OPTION, "AUTOOP",
      dbchannel_get_autoop(regchptr) ? "ON" : "OFF");

  reply_user(service, service, client, CS_INFO_OPTION, "AUTOVOICE",
      dbchannel_get_autovoice(regchptr) ? "ON" : "OFF");

  reply_user(service, service, client, CS_INFO_OPTION, "LEAVEOPS",
      dbchannel_get_leaveops(regchptr) ? "ON" : "OFF");

  reply_user(service, service, client, CS_INFO_OPTION, "FLOODSERV",
      dbchannel_get_floodserv(regchptr) ? "ON" : "OFF");

  if(access != NULL)
    MyFree(access);

  dbchannel_free(regchptr);
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
  DBChannel *regchptr;
  struct ChanAccess *access, *oldaccess;
  unsigned int account, level;
  char *level_added = "MEMBER";

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? dbchannel_find(parv[1]) : chptr->regchan;

  if((account = nickname_id_from_nick(parv[2], TRUE)) <= 0)
  {
    reply_user(service, service, client, CS_REGISTER_NICK, parv[2]);
    if(chptr == NULL)
      dbchannel_free(regchptr);
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
      dbchannel_free(regchptr);
    return;
  }

  access = MyMalloc(sizeof(struct ChanAccess));
  access->channel = dbchannel_get_id(regchptr);
  access->account = account;
  access->level   = level;

  if((oldaccess = chanaccess_find(access->channel, access->account)) != NULL)
  {
    int mcount = -1;
    dbchannel_masters_count(dbchannel_get_id(regchptr), &mcount);
    if(oldaccess->level == MASTER_FLAG && mcount <= 1)
    {
      reply_user(service, service, client, CS_ACCESS_NOMASTERS, parv[2],
          dbchannel_get_channel(regchptr));
      if(chptr == NULL)
        dbchannel_free(regchptr);
      MyFree(oldaccess);
      MyFree(access);
      return;
    }
    chanaccess_remove(access);
    MyFree(oldaccess);
  }

  if(chanaccess_add(access))
  {
    reply_user(service, service, client, CS_ACCESS_ADDOK, parv[2], parv[1],
        level_added);
    ilog(L_DEBUG, "%s (%s@%s) added AE %s(%d) to %s", client->name, 
        client->username, client->host, parv[2], access->level, parv[1]);
    send_chops_notice(service, chptr, "[%s ChanOps] %s Added %s to %s "
        "access list as %s", chptr->chname, client->name, parv[2], 
        chptr->chname, level_added); 
  }
  else
    reply_user(service, service, client, CS_ACCESS_ADDFAIL, parv[2], parv[1],
        parv[3]);

  if (chptr == NULL)
    dbchannel_free(regchptr);

  MyFree(access);
}


static void
m_access_del(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
  struct ChanAccess *access, *myaccess;
  unsigned int nickid;
  int mcount = -1;

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? dbchannel_find(parv[1]) : chptr->regchan;

  if((nickid = nickname_id_from_nick(parv[2], TRUE)) <= 0)
  {
    reply_user(service, service, client, CS_ACCESS_NOTLISTED, parv[2], parv[1]);
    if(chptr == NULL)
      dbchannel_free(regchptr);
    return;
  }

  access = chanaccess_find(dbchannel_get_id(regchptr), nickid);
  if(access == NULL)
  {
    reply_user(service, service, client, CS_ACCESS_NOTLISTED, parv[2], parv[1]);
    if(chptr == NULL)
      dbchannel_free(regchptr);
    return;
  }

  if(nickid != nickname_get_id(client->nickname) && client->access != SUDO_FLAG)
  {
    if(client->access != SUDO_FLAG)
    {
      myaccess = chanaccess_find(dbchannel_get_id(regchptr), nickname_get_id(client->nickname));
      if(myaccess->level != MASTER_FLAG)
      {
        reply_user(service, NULL, client, SERV_NO_ACCESS_CHAN, "DEL",
            dbchannel_get_channel(regchptr));
        if(chptr == NULL)
          dbchannel_free(regchptr);
        MyFree(access);
        return;
      }
    }
  }

  dbchannel_masters_count(dbchannel_get_id(regchptr), &mcount);
  if(access->level == MASTER_FLAG && mcount <= 1)
  {
    reply_user(service, service, client, CS_ACCESS_NOMASTERS, parv[2],
        dbchannel_get_channel(regchptr));
    if(chptr == NULL)
      dbchannel_free(regchptr);
    MyFree(access);
    return;
  }

  if(chanaccess_remove(access))
  {
    reply_user(service, service, client, CS_ACCESS_DELOK, parv[2], parv[1]);
    ilog(L_DEBUG, "%s (%s@%s) removed AE %s from %s", 
      client->name, client->username, client->host, parv[2], parv[1]);
    send_chops_notice(service, chptr, "[%s ChanOps] %s Removed %s from %s "
        "access list", chptr->chname, client->name, parv[2], chptr->chname); 
  }
  else
    reply_user(service, service, client, CS_ACCESS_DELFAIL, parv[2], parv[1]);

  MyFree(access);

  if (chptr == NULL)
    dbchannel_free(regchptr);
}

static void
m_access_list(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct ChanAccess *access = NULL;
  struct Channel *chptr;
  DBChannel *regchptr;
  char *nick;
  int i = 1;
  dlink_node *ptr;
  dlink_list list = { 0 };

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? dbchannel_find(parv[1]) : chptr->regchan;

  if(chanaccess_list(dbchannel_get_id(regchptr), &list) == 0)
  {
    reply_user(service, service, client, CS_ACCESS_LISTEND, dbchannel_get_channel(regchptr));
    MyFree(access);
    if(chptr == NULL)
      dbchannel_free(regchptr);
    return;
  }

  DLINK_FOREACH(ptr, list.head)
  {
    char *level;
    access = (struct ChanAccess *)ptr->data;

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

    nick = nickname_nick_from_id(access->account, TRUE);
    reply_user(service, service, client, CS_ACCESS_LIST, i++, nick, level);

    MyFree(nick);
  }

  reply_user(service, service, client, CS_ACCESS_LISTEND, dbchannel_get_channel(regchptr));

  chanaccess_list_free(&list);

  if (chptr == NULL)
    dbchannel_free(regchptr);
}

static void
m_set_desc(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  char value[IRC_BUFSIZE+1];

  join_params(value, parc-1, &parv[2]);

  m_set_string(service, client, parv[1], "DESC", value, parc,
    &dbchannel_get_description, &dbchannel_set_description);
}

static void
m_set_url(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  char value[IRC_BUFSIZE+1];

  join_params(value, parc-1, &parv[2]);

  m_set_string(service, client, parv[1], "URL", value, parc,
    &dbchannel_get_url, &dbchannel_set_url);
}

static void
m_set_email(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  char value[IRC_BUFSIZE+1];

  join_params(value, parc-1, &parv[2]);

  m_set_string(service, client, parv[1], "EMAIL", value, parc,
      &dbchannel_get_email, &dbchannel_set_email);
}

static void
m_set_entrymsg(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  char value[IRC_BUFSIZE+1];

  join_params(value, parc-1, &parv[2]);

  m_set_string(service, client, parv[1], "ENTRYMSG", value, parc,
      &dbchannel_get_entrymsg, &dbchannel_set_entrymsg);
}

static void
m_set_topic(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
  char value[IRC_BUFSIZE+1];
  char *topic = NULL;
  int changetopic = FALSE;

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? dbchannel_find(parv[1]) : chptr->regchan;

  DupString(topic, dbchannel_get_topic(regchptr));

  join_params(value, parc-1, &parv[2]);
  strlcpy(value, value, TOPICLEN+1);

  m_set_string(service, client, parv[1], "TOPIC", value, parc,
      &dbchannel_get_topic, &dbchannel_set_topic);

  if(topic != NULL && (dbchannel_get_topic(regchptr) == NULL || 
        strncmp(topic, dbchannel_get_topic(regchptr), 
          LIBIO_MAX(strlen(topic), strlen(dbchannel_get_topic(regchptr))))) != 0)
    changetopic = TRUE;

  if((changetopic || ((topic == NULL && (dbchannel_get_topic(regchptr) != NULL)))) && (chptr != NULL))
    send_topic(service, chptr, client, dbchannel_get_topic(regchptr));

  MyFree(topic);

  if(chptr == NULL)
    dbchannel_free(regchptr);
}

static void
m_set_mlock(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
  char value[IRC_BUFSIZE+1];
  char mode[MODEBUFLEN+1];
  int nolimit;
  unsigned int result;

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? dbchannel_find(parv[1]) : chptr->regchan;

  join_params(value, parc-1, &parv[2]);

  if(value != NULL && ircncmp(value, "-", strlen(value)) == 0)
  {
    if(dbchannel_set_topic(regchptr, NULL))
      reply_user(service, service, client, CS_SET_FAILED, "MLOCK",
        "Not Set", dbchannel_get_channel(regchptr));
    else
      reply_user(service, service, client, CS_SET_FAILED, "MLOCK",
        "Not Set", dbchannel_get_channel(regchptr));

    if(chptr == NULL)
      dbchannel_free(regchptr);

    return;
  }

  result = enforce_mode_lock(service, chptr, value, mode, &nolimit);

  if(result > 0)
  {
    /* Invalid MLOCK */
    /* TODO FIXME XXX reply with illegal mode configuration */
    reply_user(service, service, client, CS_SET_FAILED, "MLOCK",
        value, dbchannel_get_channel(regchptr));
    return;
  }

  if(nolimit && dbchannel_get_autolimit(regchptr))
  {
    dbchannel_set_autolimit(regchptr, FALSE);
    reply_user(service, service, client, CS_MLOCK_CONFLICT_LIMIT);
  }

  if(dbchannel_set_mlock(regchptr, mode))
  {
    reply_user(service, service, client, CS_SET_SUCCESS, "MLOCK",
        dbchannel_get_mlock(regchptr) == NULL ? "Not set" : dbchannel_get_mlock(regchptr),
        dbchannel_get_channel(regchptr));
  }
  else
  {
    reply_user(service, service, client, CS_SET_FAILED, "MLOCK",
        mode, dbchannel_get_channel(regchptr));
  }

  if(chptr == NULL)
    dbchannel_free(regchptr);
}
static void
m_set_topiclock(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], "TOPICLOCK",
    &dbchannel_get_topic_lock, &dbchannel_set_topic_lock);
}

static void
m_set_private(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], "PRIVATE",
    &dbchannel_get_priv, &dbchannel_set_priv);
}

static void
m_set_restricted(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], "RESTRICTED",
    &dbchannel_get_restricted, &dbchannel_set_restricted);
}

static void
m_set_verbose(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], "VERBOSE",
    &dbchannel_get_verbose, &dbchannel_set_verbose);
}

static void
m_set_autoop(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], "AUTOOP",
    &dbchannel_get_autoop, &dbchannel_set_autoop);
}

static void
m_set_autovoice(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], "AUTOVOICE",
    &dbchannel_get_autovoice, &dbchannel_set_autovoice);
}

static void
m_set_leaveops(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], parv[2], "LEAVEOPS",
    &dbchannel_get_leaveops, &dbchannel_set_leaveops);
}

static void
m_set_autolimit(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr = hash_find_channel(parv[1]);

  /*TODO XXX FIXME Setting AUTOLIMIT and having -l/+l MLOCK will fight each other*/
  m_set_flag(service, client, parv[1], parv[2], "AUTOLIMIT",
    &dbchannel_get_autolimit, &dbchannel_set_autolimit);
  if(chptr != NULL && chptr->regchan != NULL && dbchannel_get_autolimit(chptr->regchan))
  {
    chptr->limit_time = CurrentTime + 90;
    dlinkAdd(chptr, make_dlink_node(), &channel_limit_list);
  }
  else if(chptr != NULL && chptr->regchan != NULL)
  {
    m_delete_autolimit(chptr);
  }
}

static void
m_delete_autolimit(struct Channel *chptr)
{
  dlink_node *ptr;

  if((ptr = dlinkFindDelete(&channel_limit_list, chptr)) != NULL)
    free_dlink_node(ptr);
}
  
static void
m_set_expirebans(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr = hash_find_channel(parv[1]);
  DBChannel *regchptr;
  int interval;
  char *flag;

  regchptr = (chptr == NULL) ? dbchannel_find(parv[1]) : chptr->regchan;
  interval = dbchannel_get_expirebans_lifetime(regchptr); 

  if(parv[2] != NULL)
  {
    interval = atoi(parv[2]);
    if(interval > 0)
      flag = "ON";
    else if(interval == 0)
    {
      flag = "OFF";
      interval = dbchannel_get_expirebans_lifetime(regchptr);
    }
    else
    {
      flag = parv[2];
      interval = dbchannel_get_expirebans_lifetime(regchptr);
    }
  }
  else
    flag = NULL;

  m_set_flag(service, client, parv[1], flag, "EXPIREBANS",
    &dbchannel_get_expirebans, &dbchannel_set_expirebans);
  if(chptr != NULL && chptr->regchan != NULL && dbchannel_get_expirebans(chptr->regchan))
    dlinkAdd(chptr, make_dlink_node(), &channel_expireban_list);
  else if(chptr != NULL && chptr->regchan != NULL)
  {
    dlink_node *ptr;

    if((ptr = dlinkFindDelete(&channel_expireban_list, chptr)) != NULL)
      free_dlink_node(ptr);
  }

  if(parv[2] != NULL)
    dbchannel_set_expirebans_lifetime(regchptr, interval);

  reply_user(service, service, client, CS_EXPIREBANS_LIFETIME, interval);

  dbchannel_set_expirebans_lifetime(regchptr, interval);

  if(chptr == NULL)
    dbchannel_free(regchptr);
}

static void
m_set_floodserv(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr = hash_find_channel(parv[1]);
  struct Client *floodserv;
  char fsname[NICKLEN+1] = "FloodServ";

  m_set_flag(service, client, parv[1], parv[2], "FLOODSERV",
    &dbchannel_get_floodserv, &dbchannel_set_floodserv);

  if(chptr != NULL && chptr->regchan != NULL && dbchannel_get_floodserv(chptr->regchan))
  {
    if(ServicesState.namesuffix)
      strlcat(fsname, ServicesState.namesuffix, sizeof(fsname));

    floodserv = find_client(fsname);

    if(floodserv != NULL)
      join_channel(floodserv, chptr);
    else
    {
      reply_user(service, service, client, CS_FS_NOT_LOADED, chptr->chname);
      ilog(L_INFO, "%s SET %s ON for %s, FloodServ isn't loaded",
        client->name, fsname, chptr->chname);
    }
  }
  else if(chptr != NULL && chptr->regchan != NULL)
  {

    if(ServicesState.namesuffix)
      strlcat(fsname, ServicesState.namesuffix, sizeof(fsname));

    floodserv = find_client(fsname);

    if(floodserv != NULL && IsMember(floodserv, chptr))
      part_channel(floodserv, chptr->chname, "Unset");
  }
}

/* AKICK ADD (nick|mask) reason */
static void
m_akick_add(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  Nickname *nick;
  struct Channel *chptr;
  DBChannel *regchptr;
  char *reason;
  int ret;

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? dbchannel_find(parv[1]) : chptr->regchan;

  if(parv[3] != NULL)
  {
    reason = MyMalloc(IRC_BUFSIZE+1);
    join_params(reason, parc-2, &parv[3]);
  }
  else
    DupString(reason, "You are not permitted on this channel");

  if(strchr(parv[2], '@') == NULL)
  {
    /* Nickname based akick */
    if((nick = nickname_find(parv[2])) == NULL)
    {
      reply_user(service, service, client, CS_AKICK_NONICK, parv[2]);
      if(chptr == NULL)
        dbchannel_free(regchptr);
      if(reason != NULL)
        MyFree(reason);
      return;
    }

    ret = servicemask_add_akick_target(nickname_get_id(nick),
        nickname_get_id(client->nickname), dbchannel_get_id(regchptr),
        CurrentTime, 0, reason);

    nickname_free(nick);
  }
  else
  {
    /* mask based akick */
    ret = servicemask_add_akick(parv[2], nickname_get_id(client->nickname),
        dbchannel_get_id(regchptr), CurrentTime, 0, reason);
  }

  if(ret)
    reply_user(service, service, client, CS_AKICK_ADDED, parv[2]);
  else
    reply_user(service, service, client, CS_AKICK_ADDFAIL, parv[2]);

  if(chptr == NULL)
    dbchannel_free(regchptr);
  else if(ret) /* Only enforce if a mask was added */
  {
    /* a successful addition will trigger an enforcement of all akicks */
    char **parv = MyMalloc(2 * sizeof(char *));
    DupString(parv[0], "ENFORCE");
    DupString(parv[1], chptr->chname);
    m_akick_enforce(service, client, 2, parv);
    MyFree(parv[0]);
    MyFree(parv[1]);
    MyFree(parv);
  }

  if(reason != NULL)
    MyFree(reason);
}

static void
m_akick_list(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_mask_list(client, parv[1], "AKICK", &servicemask_list_akick);
}

static void
m_akick_del(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
  int ret;

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? dbchannel_find(parv[1]) : chptr->regchan;

  /*index = atoi(parv[2]);
  if(index > 0)
    ret = akick_remove_index(dbchannel_get_id(regchptr), index);*/
  if(strchr(parv[2], '@') != NULL)
    ret = servicemask_remove_akick(dbchannel_get_id(regchptr), parv[2]);
  else
    ret = servicemask_remove_akick_target(dbchannel_get_id(regchptr), parv[2]);

  if(chptr == NULL)
    dbchannel_free(regchptr);

  reply_user(service, service, client, CS_AKICK_DEL, ret, "AKICK");
}

static void
m_akick_enforce(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
  int numkicks = 0;
  dlink_node *ptr;
  dlink_node *next_ptr;

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? dbchannel_find(parv[1]) : chptr->regchan;

  DLINK_FOREACH_SAFE(ptr, next_ptr, chptr->members.head)
  {
    struct Membership *ms = ptr->data;
    struct Client *client = ms->client_p;

    numkicks += akick_check_client(service, chptr, client);
  }

  reply_user(service, service, client, CS_AKICK_ENFORCE, numkicks, 
      dbchannel_get_channel(regchptr));

  if(chptr == NULL)
    dbchannel_free(regchptr);
}

static void
m_clear_bans(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
  dlink_node *ptr, *nptr;
  int numbans = 0;
  dlink_list list = { 0 };
 
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
    char *ban = MyMalloc(IRC_BUFSIZE+1);

    snprintf(ban, IRC_BUFSIZE, "%s!%s@%s", banptr->name, banptr->username,
        banptr->host);

    dlinkAdd(ban, make_dlink_node(), &list);
    numbans++;
  }

  unban_mask_many(service, chptr, &list);

  db_string_list_free(&list);

  reply_user(service, service, client, CS_CLEAR_BANS, numbans, 
      dbchannel_get_channel(regchptr));
}

static void
m_clear_quiets(struct Service *service, struct Client *client, int parc,
    char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
  dlink_node *ptr, *nptr;
  dlink_list list = { 0 };
  int numbans = 0;

  chptr = hash_find_channel(parv[1]);

  if(chptr == NULL)
  {
    reply_user(service, service, client, CS_CHAN_NOT_USED, parv[1]);
    return;
  }
  regchptr = chptr->regchan;

  DLINK_FOREACH_SAFE(ptr, nptr, chptr->quietlist.head)
  {
    const struct Ban *banptr = ptr->data;
    char *ban = MyMalloc(IRC_BUFSIZE+1);

    snprintf(ban, IRC_BUFSIZE, "%s!%s@%s", banptr->name, banptr->username,
        banptr->host);
    dlinkAdd(ban, make_dlink_node(), &list);
    numbans++;
  }

  unquiet_mask_many(service, chptr, &list);

  db_string_list_free(&list);

  reply_user(service, service, client, CS_CLEAR_QUIETS, numbans,
      dbchannel_get_channel(regchptr));
}

static void
m_clear_ops(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
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
      dbchannel_get_channel(regchptr));
}

static void
m_clear_voices(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
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
      dbchannel_get_channel(regchptr));
}

static void
m_clear_modes(struct Service *service, struct Client *client, int parc,
    char *parv[])
{
  struct Channel *chptr;
  char mbuf[MODEBUFLEN+1];
  char buf[MODEBUFLEN+1] = "-";
  int tmp;

  chptr = hash_find_channel(parv[1]);

  if(chptr == NULL)
  {
    reply_user(service, service, client, CS_CHAN_NOT_USED, parv[1]);
    return;
  }

  get_modestring(chptr->mode.mode, mbuf, MODEBUFLEN);

  ilog(L_DEBUG, "ChanServ CLEAR MODES: %s has %s", chptr->chname, mbuf);

  strlcat(buf, mbuf, MODEBUFLEN);

  if(chptr->mode.key[0] != '\0')
  {
    strlcat(buf, "k", MODEBUFLEN);
    chptr->mode.key[0] = '\0';
  }

  if(chptr->mode.limit > 0)
  {
    strlcat(buf, "l", MODEBUFLEN);
    chptr->mode.limit = 0;
  }

  ilog(L_DEBUG, "ChanServ CLEAR MODES: %s removing %s", chptr->chname, buf);

  send_cmode(service, chptr, buf, "");
  chptr->mode.mode &= ~chptr->mode.mode;

  /* Only enforce if MLOCK is on */
  if(dbchannel_get_mlock(chptr->regchan) != NULL)
  {
    ilog(L_DEBUG, "ChanServ CLEAR MODES: %s setting MLOCK %s", chptr->chname,
      dbchannel_get_mlock(chptr->regchan));
    enforce_mode_lock(chanserv, chptr, NULL, NULL, &tmp);
  }

  reply_user(service, service, client, CS_CLEAR_MODES, chptr->chname);
}

static void
m_clear_users(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
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
      dbchannel_get_channel(regchptr));
}

static void
m_op(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
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
    send_chops_notice(service, chptr, "[%s ChanOps] %s OP %s", 
        chptr->chname, client->name, target->name, chptr->chname);
  }
  else
    reply_user(service, service, client, CS_ALREADY_OP, target->name, parv[1]);
}

static void
m_deop(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
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
    send_chops_notice(service, chptr, "[%s ChanOps] %s DEOP %s", 
        chptr->chname, client->name, target->name, chptr->chname);
  }
  else
    reply_user(service, service, client, CS_NOT_OP, target->name, parv[1]);
}

static void
m_voice(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Channel    *chptr;
  DBChannel *regchptr;
  struct Client     *target;
  struct Membership *ms;
  struct ChanAccess *access;

  chptr = hash_find_channel(parv[1]);

  if(chptr == NULL)
  {
    reply_user(service, service, client, CS_CHAN_NOT_USED, parv[1]);
    return;
  }

  regchptr = chptr->regchan;
  access = chanaccess_find(dbchannel_get_id(regchptr), nickname_get_id(client->nickname));

  if(parv[2] == NULL)
    target = client;
  else if(access->level < CHANOP_FLAG)
  {
    reply_user(service, service, client, CS_NO_VOICE_OTHERS, parv[1]);
    MyFree(access);
    return;
  }
  else
    target = find_client(parv[2]);

  MyFree(access);

  if(target == NULL || (ms = find_channel_link(target, chptr)) == NULL)
  {
    reply_user(service, service, client, CS_NOT_ON_CHAN, parv[2], parv[1]);
    return;
  }

  if(!has_member_flags(ms, CHFL_VOICE))
  {
    voice_user(service, chptr, target);
    reply_user(service, service, client, CS_VOICE, target->name, parv[1]);
    send_chops_notice(service, chptr, "[%s ChanOps] %s VOICE %s", 
        chptr->chname, client->name, target->name, chptr->chname); 
  }
  else
    reply_user(service, service, client, CS_ALREADY_VOICE, target->name, parv[1]);
}

static void
m_devoice(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Channel    *chptr;
  DBChannel *regchptr;
  struct Client     *target;
  struct Membership *ms;
  struct ChanAccess *access;

  chptr = hash_find_channel(parv[1]);

  if(chptr == NULL)
  {
    reply_user(service, service, client, CS_CHAN_NOT_USED, parv[1]);
    return;
  }

  regchptr = chptr->regchan;
  access = chanaccess_find(dbchannel_get_id(regchptr), nickname_get_id(client->nickname));

  if(parv[2] == NULL)
    target = client;
  else if(access->level < CHANOP_FLAG)
  {
    reply_user(service, service, client, CS_NO_DEVOICE_OTHERS, parv[1]);
    MyFree(access);
    return;
  }
  else
    target = find_client(parv[2]);

  MyFree(access);

  if(target == NULL || (ms = find_channel_link(target, chptr)) == NULL)
  {
    reply_user(service, service, client, CS_NOT_ON_CHAN, parv[2], parv[1]);
    return;
  }

  if(has_member_flags(ms, CHFL_VOICE))
  {
    devoice_user(service, chptr, target);
    reply_user(service, service, client, CS_DEVOICE, target->name, parv[1]);
    send_chops_notice(service, chptr, "[%s ChanOps] %s DEVOICE %s", 
        chptr->chname, client->name, target->name, chptr->chname); 
  }
  else
    reply_user(service, service, client, CS_NOT_VOICE, target->name, parv[1]);
}

static void
m_invite(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
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
  send_chops_notice(service, chptr, "[%s ChanOps] %s INVITE %s", 
      chptr->chname, client->name, target->name, chptr->chname); 
}

static void
m_unban(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
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
    send_chops_notice(service, chptr, "[%s ChanOps] %s UNBAN %s!%s@%s", 
        chptr->chname, client->name, banp->name, banp->username, banp->host);
    unban_mask(service, chptr, ban);
 
    numbans++;

    banp = find_bmask(client, &chptr->banlist);
  }

  reply_user(service, service, client, CS_CLEAR_BANS, numbans,
      dbchannel_get_channel(regchptr));
}

static void
m_unquiet(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Channel *chptr;
  DBChannel *regchptr;
  struct Ban *banp;
  int numbans = 0;

  chptr = hash_find_channel(parv[1]);

  if(chptr == NULL)
  {
    reply_user(service, service, client, CS_CHAN_NOT_USED, parv[1]);
    return;
  }
  regchptr = chptr->regchan;

  banp = find_bmask(client, &chptr->quietlist);
  while(banp != NULL)
  {
    char ban[IRC_BUFSIZE+1];

    snprintf(ban, IRC_BUFSIZE, "%s!%s@%s", banp->name, banp->username,
        banp->host);
    unquiet_mask(service, chptr, ban);
    numbans++;

    banp = find_bmask(client, &chptr->quietlist);
  }

  reply_user(service, service, client, CS_CLEAR_QUIETS, numbans,
      dbchannel_get_channel(regchptr));
}

static int
m_set_string(struct Service *service, struct Client *client,
    const char *channel, const char *field, const char *value,
    int parc, const char *(*get_func)(DBChannel *),
    int (*set_func)(DBChannel *, const char *))
{
  struct Channel *chptr;
  DBChannel *regchptr;

  chptr = hash_find_channel(channel);
  regchptr = chptr == NULL ? dbchannel_find(channel) : chptr->regchan;

  if(parc < 2)
  {
    const char *response = get_func(regchptr);

    if(client != NULL)
    {
      reply_user(service, service, client, CS_SET_VALUE, field,
          response == NULL ? "Not Set" : response,
          dbchannel_get_channel(regchptr));
    }
    if(chptr == NULL)
      dbchannel_free(regchptr);
    return TRUE;
  }

  if(value != NULL && ircncmp(value, "-", strlen(value)) == 0)
    value = NULL;

  if(set_func(regchptr, value))
  {
    if(client != NULL)
    {
      reply_user(service, service, client, CS_SET_SUCCESS, field,
          value == NULL ? "Not set" : value, dbchannel_get_channel(regchptr));
      ilog(L_DEBUG, "%s (%s@%s) changed %s of %s to %s",
          client->name, client->username, client->host, field, dbchannel_get_channel(regchptr),
          value);
      send_chops_notice(service, chptr, "[%s ChanOps] %s SET %s to %s",
          chptr->chname, client->name, field,
          value == NULL ? "Not set" : value);
    }

    if(chptr == NULL)
      dbchannel_free(regchptr);

    return TRUE;
  }
  else if(client != NULL)
    reply_user(service, service, client, CS_SET_FAILED, field,
        value == NULL ? "Not set" : value, dbchannel_get_channel(regchptr));

  if(chptr == NULL)
    dbchannel_free(regchptr);

  return FALSE;
}

static int
m_set_flag(struct Service *service, struct Client *client,
           char *channel, char *toggle, char *flagname,
           char (*get_func)(DBChannel *), int (*set_func)(DBChannel *, char))
{
  struct Channel *chptr;
  DBChannel *regchptr;
  int on = FALSE;

  chptr = hash_find_channel(channel);
  regchptr = chptr == NULL ? dbchannel_find(channel) : chptr->regchan;

  if (toggle == NULL)
  {
    on = get_func(regchptr);
    reply_user(service, service, client, CS_SET_VALUE, flagname,
        on ? "ON" : "OFF", channel);

    if (chptr == NULL)
      dbchannel_free(regchptr);
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
    if(chptr == NULL)
      dbchannel_free(regchptr);
    return -1;
  }

  if(set_func(regchptr, on))
    reply_user(service, service, client, CS_SET_SUCCESS,
        flagname, on ? "ON" : "OFF", channel);
  else
    reply_user(service, service, client, CS_SET_FAILED, flagname, channel);

  if (chptr == NULL)
    dbchannel_free(regchptr);

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

  client->access = ADMIN_FLAG;
}

static void
m_list(struct Service *service, struct Client *client, int parc, char *parv[])
{
  char *chan;
  int count = 0;
  int qcount = 0;
  dlink_list list = { 0 };
  dlink_node *ptr;

  if(parc == 2 && client->access >= OPER_FLAG)
  {
    if(irccmp(parv[2], "FORBID") == 0)
      qcount = dbchannel_list_forbid(&list);
    else
    {
      reply_user(service, service, client, CS_LIST_INVALID_OPTION, parv[2]);
      return;
    }
  }

  if(qcount == 0 && client->access >= OPER_FLAG)
    qcount = dbchannel_list_all(&list);
  else if(qcount == 0)
    qcount = dbchannel_list_regular(&list);

  if(qcount == 0)
  {
    /* XXX TODO FIXME
     * cache which method was called
     */
    db_string_list_free(&list);
    reply_user(service, service, client, CS_LIST_NO_MATCHES, parv[1]);
    return;
  }

  DLINK_FOREACH(ptr, list.head)
  {
    chan = (char *)ptr->data;
    if(match(parv[1], chan))
    {
      count++;
      reply_user(service, service, client, CS_LIST_ENTRY, chan);
    }
    if(count == 50)
      break;
  }
  /* XXX TODO FIXME
   * cache which method was called
   */
  db_string_list_free(&list);

  reply_user(service, service, client, CS_LIST_END, count);
}

static void
m_forbid(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Channel *target;
  char *resv = parv[1];
  char duration_char;
  time_t duration = -1;
  dlink_node *ptr, *nptr;

  if(*parv[1] == '+')
  {
    char *ptr = parv[1];

    resv = parv[2];
    ptr++;

    while(*ptr != '\0')
    {
      if(!IsDigit(*ptr))
      {
        duration_char = *ptr;
        *ptr = '\0';
        duration = atoi(parv[1]);
        break;
      }
      ptr++;
    }
  }

  if(*resv != '#')
  {
    reply_user(service, service, client, CS_NAMESTART_HASH, resv);
    return;
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
        break;
      default:
        reply_user(service, service, client, CS_FORBID_BAD_DURATIONCHAR,
            duration_char);
        return;
    }
  }
  else
    duration = ServicesInfo.def_forbid_dur;

  if(duration == -1)
    duration = 0;

  if(!dbchannel_forbid(resv))
  {
    reply_user(service, service, client, CS_FORBID_FAIL, parv[1]);
    return;
  }

  send_resv(service, resv, "Forbidden channel", duration);

  if((target = hash_find_channel(resv)) != NULL)
  {
    DLINK_FOREACH_SAFE(ptr, nptr, target->members.head)
    {
      struct Membership *ms = ptr->data;
      struct Client *user = ms->client_p;

      kick_user(service, target, user->name, 
          "This channel is forbidden and may not be used");
    }
  }
  reply_user(service, service, client, CS_FORBID_OK, parv[1]);
}

static void
m_unforbid(struct Service *service, struct Client *client, int parc, char *parv[])
{
  if(!dbchannel_is_forbid(parv[1]))
  {
    reply_user(service, service, client, CS_CHAN_NOT_FORBID, parv[1]);
    return;
  }

  if(!dbchannel_delete_forbid(parv[1]))
  {
    reply_user(service, service, client, CS_UNFORBID_FAIL, parv[1]);
    return;
  }

  send_unresv(service, parv[1]);

  reply_user(service, service, client, CS_UNFORBID_OK, parv[1]);
}

static int
m_mask_add(struct Client *client, int parc, char *parv[], const char *modename,
  int (*add_func)(const char *, unsigned int, unsigned int, unsigned int, unsigned int, const char *),
  void (*mask_func)(struct Service *, struct Channel *, const char *))
{
  DBChannel *regchptr;
  struct Channel *chptr;
  int ret;
  char reason[IRC_BUFSIZE+1] = { '\0' };

  chptr = hash_find_channel(parv[1]);
  regchptr = chptr == NULL ? dbchannel_find(parv[1]) : chptr->regchan;

  if(parv[2] != NULL)
    join_params(reason, parc-2, &parv[3]);
  else
    strlcat(reason, "No Reason Given", IRC_BUFSIZE);

  ret = add_func(parv[2], nickname_get_id(client->nickname), dbchannel_get_id(regchptr),
                 CurrentTime, 0, reason);

  if(ret)
  {
    if(chptr != NULL)
      mask_func(chanserv, chptr, parv[2]);

    reply_user(chanserv, chanserv, client, CS_SERVICEMASK_ADD_SUCCESS, parv[1], reason, modename);
  }
  else
    reply_user(chanserv, chanserv, client, CS_SERVICEMASK_ADD_FAILED, parv[1], modename);

  if(chptr == NULL)
    dbchannel_free(regchptr);

  return ret;
}

static void
m_mask_list(struct Client *client, const char* channel, const char* modename,
  int (*list_func)(unsigned int, dlink_list *))
{
  struct ServiceMask *akick = NULL;
  int i = 1;
  struct Channel *chptr;
  DBChannel *regchptr;
  char setbuf[TIME_BUFFER + 1];
  dlink_node *ptr;
  dlink_list list = { 0 };

  chptr = hash_find_channel(channel);
  regchptr = chptr == NULL ? dbchannel_find(channel) : chptr->regchan;

  list_func(dbchannel_get_id(regchptr), &list);
  DLINK_FOREACH(ptr, list.head)
  {
    char *who, *whoset;
    akick = (struct ServiceMask *)ptr->data;

    if(akick->target == 0)
      who = akick->mask;
    else
      who = nickname_nick_from_id(akick->target, TRUE);

    whoset = nickname_nick_from_id(akick->setter, TRUE);

    strtime(client, akick->time_set, setbuf);

    reply_user(chanserv, chanserv, client, CS_AKICK_LIST, i++, who, akick->reason,
        whoset, setbuf);
    if(akick->target != 0)
      MyFree(who);
    MyFree(whoset);
  }
  servicemask_list_free(&list);

  reply_user(chanserv, chanserv, client, CS_AKICK_LISTEND, modename, channel);

  if(chptr == NULL)
    dbchannel_free(regchptr);
}

static int
m_mask_del(struct Client *client, const char *channel, const char *mask, const char *modename,
  int (*del_func) (unsigned int, const char *),
  void (*unmask_func) (struct Service *, struct Channel *, const char *))
{
  struct Channel *chptr;
  DBChannel *regchptr;
  int ret;

  chptr = hash_find_channel(channel);
  regchptr = chptr == NULL ? dbchannel_find(channel) : chptr->regchan;

  ret = del_func(dbchannel_get_id(regchptr), mask);

  if(chptr == NULL)
    dbchannel_free(regchptr);
  else if(ret > 0)
    unmask_func(chanserv, chptr, mask);

  reply_user(chanserv, chanserv, client, CS_AKICK_DEL, ret, modename);

  return ret;
}

static void
m_invites_add(struct Service *service, struct Client *client, int parc, char * parv[])
{
  m_mask_add(client, parc, parv, "INVITES", &servicemask_add_invex, &invex_mask);
}

static void
m_invites_del(struct Service *service, struct Client *client, int parc, char * parv[])
{
  m_mask_del(client, parv[1], parv[2], "INVITES", &servicemask_remove_invex, &uninvex_mask);
}

static void
m_invites_list(struct Service *service, struct Client *client, int parc, char * parv[])
{
  m_mask_list(client, parv[1], "INVITES", &servicemask_list_invex);
}

static void
m_excepts_add(struct Service *service, struct Client *client, int parc, char * parv[])
{
  m_mask_add(client, parc, parv, "EXCEPTS", &servicemask_add_excpt, &except_mask);
}

static void
m_excepts_del(struct Service *service, struct Client *client, int parc, char * parv[])
{
  m_mask_del(client, parv[1], parv[2], "EXCEPTS", &servicemask_remove_excpt, &unexcept_mask);
}

static void
m_excepts_list(struct Service *service, struct Client *client, int parc, char * parv[])
{
  m_mask_list(client, parv[1], "EXCEPTS", &servicemask_list_excpt);
}

static void
m_quiets_add(struct Service *service, struct Client *client, int parc, char * parv[])
{
  m_mask_add(client, parc, parv, "QUIETS", &servicemask_add_quiet, &quiet_mask);
}

static void
m_quiets_del(struct Service *service, struct Client *client, int parc, char * parv[])
{
  m_mask_del(client, parv[1], parv[2], "QUIETS", &servicemask_remove_quiet, &unquiet_mask);
}

static void
m_quiets_list(struct Service *service, struct Client *client, int parc, char * parv[])
{
  m_mask_list(client, parv[1], "QUIETS", &servicemask_list_quiet);
}


/**
 * @brief CS Callback when a ModeChange is received for a Channel
 * @param args 
 * @return pass_callback()
 */
static void *
cs_on_cmode_change(va_list args) 
{
  struct Client  *source  = va_arg(args, struct Client*);
  struct Channel *chptr   = va_arg(args, struct Channel*);
  int  dir  = va_arg(args, int);
  char mode = (char)va_arg(args, int);
  char *param = va_arg(args, char *);
  int tmp;

  if(chptr->regchan != NULL && dbchannel_get_mlock(chptr->regchan) != NULL)
    enforce_mode_lock(chanserv, chptr, NULL, NULL, &tmp);
  
  /* last function to call in this func */
  return pass_callback(cs_cmode_hook, source, chptr, dir, mode, param);
}

/**
 * @brief CS Callback when a Client joins a Channel
 * @param args 
 * @return pass_callback(self, struct Client *, char *)
 * When a Client joins a Channel:
 *  - attach DBChannel * to struct Channel*
 */
static void *
cs_on_client_join(va_list args)
{
  struct Client *source_p = va_arg(args, struct Client *);
  char          *name     = va_arg(args, char *);

  char tmp_name[CHANNELLEN+1];
  DBChannel *regchptr;
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

  if(dbchannel_is_forbid(name))
  {
    strlcpy(tmp_name, name, CHANNELLEN);
    kick_user(chanserv, chptr, source_p->name, 
        "This channel is forbidden and may not be used");
    send_resv(chanserv, tmp_name, "Forbidden channel", ServicesInfo.def_forbid_dur);
    return NULL;
  }

  if(akick_check_client(chanserv, chptr, source_p))
    return pass_callback(cs_join_hook, source_p, name);

  if((regchptr = chptr->regchan) == NULL)
    return pass_callback(cs_join_hook, source_p, name);

  if(source_p->nickname == NULL)
    level = CHUSER_FLAG;
  else
  {
    access = chanaccess_find(dbchannel_get_id(regchptr), nickname_get_id(source_p->nickname));
    if(access == NULL)
      level = CHIDENTIFIED_FLAG;
    else
      level = access->level;

    MyFree(access);
  }

  if(dbchannel_get_restricted(regchptr) && level < MEMBER_FLAG &&
    !MyConnect(source_p) && !IsGod(source_p))
  {
    char ban[IRC_BUFSIZE+1];

    snprintf(ban, IRC_BUFSIZE, "*!%s@%s", source_p->username, source_p->host);
    ban_mask(chanserv, chptr, ban);
    kick_user(chanserv, chptr, source_p->name, 
        "Access to this channel is restricted");
    return pass_callback(cs_join_hook, source_p, name);
  }

  if(!dbchannel_get_leaveops(regchptr) && (IsChanop(source_p, chptr) && level < CHANOP_FLAG))
  {
    deop_user(chanserv, chptr, source_p);
    reply_user(chanserv, chanserv, source_p, CS_DEOP_REGISTERED, 
        dbchannel_get_channel(regchptr));
  }

  if((!IsChanop(source_p, chptr)) && level >= CHANOP_FLAG && dbchannel_get_autoop(regchptr))
  {
    op_user(chanserv, chptr, source_p);
  }
  else if((!IsVoice(source_p, chptr)) && level >= MEMBER_FLAG && 
      dbchannel_get_autovoice(regchptr))
  {
    voice_user(chanserv, chptr, source_p);
  }
 
  if(dbchannel_get_entrymsg(regchptr) != NULL && dbchannel_get_entrymsg(regchptr)[0] != '\0' &&
      !IsConnecting(me.uplink))
    reply_user(chanserv, chanserv, source_p, CS_ENTRYMSG, dbchannel_get_channel(regchptr),
        dbchannel_get_entrymsg(regchptr));

  if(dbchannel_get_autolimit(regchptr))
  {
    if(dlinkFind(&channel_limit_list, chptr) == NULL)
    {
      chptr->limit_time = CurrentTime + 90;
      dlinkAdd(chptr, make_dlink_node(), &channel_limit_list);
    }
  }

  if(dbchannel_get_expirebans(regchptr))
  {
    if(dlinkFind(&channel_expireban_list, chptr) == NULL)
    {
      dlinkAdd(chptr, make_dlink_node(), &channel_expireban_list);
    }
  }
  
  return pass_callback(cs_join_hook, source_p, name);
}

static void *
cs_on_channel_create(va_list args)
{
  struct Channel *chptr = va_arg(args, struct Channel *);
  int tmp;
  dlink_list m_list = { 0 };

  if(chptr->regchan != NULL)
  {
    if(dbchannel_get_mlock(chptr->regchan) != NULL)
      enforce_mode_lock(chanserv, chptr, NULL, NULL, &tmp);

    if(!IsConnecting(me.uplink))
    {
      send_topic(chanserv, chptr, find_client(chanserv->name), 
          dbchannel_get_topic(chptr->regchan)); 
    }

    servicemask_list_invex_masks(dbchannel_get_id(chptr->regchan), &m_list);
    invex_mask_many(chanserv, chptr, &m_list);
    servicemask_list_masks_free(&m_list);

    servicemask_list_quiet_masks(dbchannel_get_id(chptr->regchan), &m_list);
    quiet_mask_many(chanserv, chptr, &m_list);
    servicemask_list_masks_free(&m_list);

    servicemask_list_excpt_masks(dbchannel_get_id(chptr->regchan), &m_list);
    except_mask_many(chanserv, chptr, &m_list);
    servicemask_list_masks_free(&m_list);
  }

  return pass_callback(cs_channel_create_hook, chptr);
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
  dlink_node *ptr;

  if (chan->regchan != NULL)
  {
    dbchannel_free(chan->regchan);
    chan->regchan = NULL;
  }

  if((ptr = dlinkFindDelete(&channel_expireban_list, chan)) != NULL)
    free_dlink_node(ptr);

  if((ptr = dlinkFindDelete(&channel_limit_list, chan)) != NULL)
    free_dlink_node(ptr);

  return pass_callback(cs_channel_destroy_hook, chan);
}

static void *
cs_on_topic_change(va_list args)
{
  struct Channel *chan = va_arg(args, struct Channel *);
  char *setter = va_arg(args, char *);
  DBChannel *regchptr;

  if(chan->regchan == NULL)
    return pass_callback(cs_on_topic_change_hook, chan, setter);

  regchptr = chan->regchan;

  if(dbchannel_get_topic_lock(regchptr))
  {
    if(dbchannel_get_topic(regchptr) != NULL)
    {
      if(chan->topic == NULL || 
          ircncmp(chan->topic, dbchannel_get_topic(regchptr), TOPICLEN) != 0)
      {
        send_topic(chanserv, chan, chanserv_client, dbchannel_get_topic(regchptr)); 
      }
    }
    else
      send_topic(chanserv, chan, chanserv_client, NULL);
  }
  else
  {
    /* Don't set empty topics on burst */
    if(chan->topic != NULL || !IsConnecting(me.uplink))
    {
      m_set_string(chanserv, NULL, chan->chname, "TOPIC", chan->topic,
        2 /*so it's set not displayed*/, NULL, dbchannel_set_topic);
    }
  }

  return pass_callback(cs_on_topic_change_hook, chan, setter);
}

static void *
cs_on_burst_done(va_list args)
{
  dlink_node *ptr;
  struct Client *chanserv_client = chanserv_client;

  DLINK_FOREACH(ptr, global_channel_list.head)
  {
    struct Channel *chptr = (struct Channel *)ptr->data;
    DBChannel *regchptr = chptr->regchan;

    if(regchptr == NULL)
      continue;

    if(dbchannel_get_topic_lock(regchptr))
    {
      if((dbchannel_get_topic(regchptr) == NULL && !EmptyString(chptr->topic)) || 
          (dbchannel_get_topic(regchptr) != NULL && (EmptyString(chptr->topic) ||
           ircncmp(chptr->topic, dbchannel_get_topic(regchptr), TOPICLEN) != 0)))
      {
        send_topic(chanserv, chptr, chanserv_client, dbchannel_get_topic(regchptr));
      }
    }
    else
    {
      if(EmptyString(chptr->topic) && dbchannel_get_topic(regchptr) != NULL)
        send_topic(chanserv, chptr, chanserv_client, dbchannel_get_topic(regchptr));
    }
  }

  return pass_callback(cs_on_burst_done_hook);
}
