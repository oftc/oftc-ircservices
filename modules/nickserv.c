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
#include "client.h"
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
#include "nickserv.h"
#include "crypt.h"
#include "dbmail.h"
#include "kill.h"

static struct Service *nickserv = NULL;
static struct Client *nickserv_client = NULL;

static dlink_node *ns_umode_hook;
static dlink_node *ns_nick_hook;
static dlink_node *ns_newuser_hook;
static dlink_node *ns_quit_hook;
static dlink_node *ns_certfp_hook;
static dlink_node *ns_on_auth_req_hook;
static dlink_node *ns_on_identify_hook;

static dlink_list nick_enforce_list = { NULL, NULL, 0 };
static dlink_list nick_release_list = { NULL, NULL, 0 };

static int guest_number;

static int set_nickname_password(Nickname *, const char *);

static void process_enforce_list(void *);
static void process_release_list(void *);

static void *ns_on_umode_change(va_list);
static void *ns_on_newuser(va_list);
static void *ns_on_nick_change(va_list);
static void *ns_on_quit(va_list);
static void *ns_on_certfp(va_list);
static void *ns_on_auth_requested(va_list);
static void *ns_on_identify(va_list);

static void m_drop(struct Service *, struct Client *, int, char *[]);
static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_identify(struct Service *, struct Client *, int, char *[]);
static void m_register(struct Service *, struct Client *, int, char *[]);
static void m_link(struct Service *, struct Client *, int, char *[]);
static void m_unlink(struct Service *, struct Client *, int, char *[]);
static void m_info(struct Service *,struct Client *, int, char *[]);
static void m_forbid(struct Service *,struct Client *, int, char *[]);
static void m_unforbid(struct Service *,struct Client *, int, char *[]);
static void m_regain(struct Service *,struct Client *, int, char *[]);
static void m_sudo(struct Service *, struct Client *, int, char *[]);
static void m_sendpass(struct Service *, struct Client *, int, char *[]);
static void m_list(struct Service *, struct Client *, int, char *[]);
static void m_status(struct Service *, struct Client *, int, char*[]);
static void m_enslave(struct Service *, struct Client *, int, char*[]);
static void m_dropnick(struct Service *, struct Client *, int, char*[]);
static void m_checkverify(struct Service *, struct Client *, int, char*[]);

static void m_set_language(struct Service *, struct Client *, int, char *[]);
static void m_set_password(struct Service *, struct Client *, int, char *[]);
static void m_set_enforce(struct Service *, struct Client *, int, char *[]);
static void m_set_secure(struct Service *, struct Client *, int, char *[]);
static void m_set_url(struct Service *, struct Client *, int, char *[]);
static void m_set_email(struct Service *, struct Client *, int, char *[]);
static void m_set_cloak(struct Service *, struct Client *, int, char *[]);
static void m_cloakstring(struct Service *, struct Client *, int, char *[]);
static void m_set_master(struct Service *, struct Client *, int, char *[]);
static void m_set_private(struct Service *, struct Client *, int, char *[]);
static void m_resetpass(struct Service *, struct Client *, int, char *[]);

static void m_access_add(struct Service *, struct Client *, int, char *[]);
static void m_access_list(struct Service *, struct Client *, int, char *[]);
static void m_access_del(struct Service *, struct Client *, int, char *[]);

static void m_cert_add(struct Service *, struct Client *, int, char *[]);
static void m_cert_list(struct Service *, struct Client *, int, char *[]);
static void m_cert_del(struct Service *, struct Client *, int, char *[]);

static void m_ajoin_add(struct Service *, struct Client *, int, char *[]);
static void m_ajoin_list(struct Service *, struct Client *, int, char *[]);
static void m_ajoin_del(struct Service *, struct Client *, int, char *[]);

static int m_set_flag(struct Service *, struct Client *, char *, char *,
    unsigned char (*)(Nickname *), int (*)(Nickname *, unsigned char));
static int m_set_string(struct Service *, struct Client *, const char *,
    const char *, int, const char *(*)(Nickname *),
    int(*)(Nickname *, const char*));

static struct ServiceMessage register_msgtab = {
  NULL, "REGISTER", 0, 2, 2, 0, USER_FLAG, NS_HELP_REG_SHORT, NS_HELP_REG_LONG, 
  m_register 
};

static struct ServiceMessage identify_msgtab = {
  NULL, "IDENTIFY", 0, 1, 2, 0, USER_FLAG, NS_HELP_ID_SHORT, NS_HELP_ID_LONG,
  m_identify
};

static struct ServiceMessage id_msgtab = {
  NULL, "ID", 0, 1, 2, SFLG_ALIAS, USER_FLAG, NS_HELP_ID_SHORT, NS_HELP_ID_LONG,
  m_identify
};

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, 2, 0, USER_FLAG, NS_HELP_SHORT, NS_HELP_LONG,
  m_help
};

static struct ServiceMessage drop_msgtab = {
  NULL, "DROP", 0, 0, 1, 0, IDENTIFIED_FLAG, NS_HELP_DROP_SHORT, NS_HELP_DROP_LONG,
  m_drop
};

static struct ServiceMessage set_sub[] = {
  { NULL, "LANGUAGE", 0, 0, 1, 0, IDENTIFIED_FLAG, NS_HELP_SET_LANG_SHORT, 
    NS_HELP_SET_LANG_LONG, m_set_language },
  { NULL, "PASSWORD", 0, 1, 1, 0, IDENTIFIED_FLAG, NS_HELP_SET_PASS_SHORT, 
    NS_HELP_SET_PASS_LONG, m_set_password },
  { NULL, "URL", 0, 0, 1, 0, IDENTIFIED_FLAG, NS_HELP_SET_URL_SHORT, 
    NS_HELP_SET_URL_LONG, m_set_url },
  { NULL, "EMAIL", 0, 0, 1, 0, IDENTIFIED_FLAG, NS_HELP_SET_EMAIL_SHORT, 
    NS_HELP_SET_EMAIL_LONG, m_set_email },
  { NULL, "ENFORCE", 0, 0, 1, 0, IDENTIFIED_FLAG, NS_HELP_SET_ENFORCE_SHORT, 
    NS_HELP_SET_ENFORCE_LONG, m_set_enforce },
  { NULL, "SECURE", 0, 0, 1, 0, IDENTIFIED_FLAG, NS_HELP_SET_SECURE_SHORT, 
    NS_HELP_SET_SECURE_LONG, m_set_secure },
  { NULL, "CLOAK", 0, 0, 1, 0, IDENTIFIED_FLAG, NS_HELP_SET_CLOAK_SHORT, 
    NS_HELP_SET_CLOAK_LONG, m_set_cloak },
 { NULL, "MASTER", 0, 0, 1, 0, IDENTIFIED_FLAG, NS_HELP_SET_MASTER_SHORT, 
    NS_HELP_SET_MASTER_LONG, m_set_master },
  { NULL, "PRIVATE", 0, 0, 1, 0, IDENTIFIED_FLAG, NS_HELP_SET_PRIVATE_SHORT, 
    NS_HELP_SET_PRIVATE_LONG, m_set_private },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage set_msgtab = {
  set_sub, "SET", 0, 1, 1, 0, IDENTIFIED_FLAG, NS_HELP_SET_SHORT, 
  NS_HELP_SET_LONG, NULL 
};

static struct ServiceMessage cloakstring_msgtab = { 
  NULL, "CLOAKSTRING", 0, 1, 2, 0, ADMIN_FLAG, NS_HELP_CLOAKSTRING_SHORT, 
    NS_HELP_CLOAKSTRING_LONG, m_cloakstring 
}; 

static struct ServiceMessage resetpass_msgtab = {
  NULL, "RESETPASS", 0, 1, 1, 0, ADMIN_FLAG, NS_HELP_RESETPASS_SHORT,
    NS_HELP_RESETPASS_LONG, m_resetpass
};

static struct ServiceMessage cert_sub[] = {
  { NULL, "ADD", 0, 0, 2, 0, IDENTIFIED_FLAG, NS_HELP_CERT_ADD_SHORT, 
    NS_HELP_CERT_ADD_LONG, m_cert_add },
  { NULL, "LIST", 0, 0, 0, 0, IDENTIFIED_FLAG, NS_HELP_CERT_LIST_SHORT, 
    NS_HELP_CERT_LIST_LONG, m_cert_list },
  { NULL, "DEL", 0, 1, 1, 0, IDENTIFIED_FLAG, NS_HELP_CERT_DEL_SHORT, 
    NS_HELP_CERT_DEL_LONG, m_cert_del },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage cert_msgtab = {
  cert_sub, "CERT", 0, 1, 1, 0, IDENTIFIED_FLAG, NS_HELP_CERT_SHORT, 
  NS_HELP_CERT_LONG, NULL
};

static struct ServiceMessage ajoin_sub[] = {
  { NULL, "ADD", 0, 1, 2, 0, IDENTIFIED_FLAG, NS_HELP_AJOIN_ADD_SHORT, 
    NS_HELP_AJOIN_ADD_LONG, m_ajoin_add },
  { NULL, "LIST", 0, 0, 0, 0, IDENTIFIED_FLAG, NS_HELP_AJOIN_LIST_SHORT, 
    NS_HELP_AJOIN_LIST_LONG, m_ajoin_list },
  { NULL, "DEL", 0, 1, 1, 0, IDENTIFIED_FLAG, NS_HELP_AJOIN_DEL_SHORT, 
    NS_HELP_AJOIN_DEL_LONG, m_ajoin_del },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage ajoin_msgtab = {
  ajoin_sub, "AJOIN", 0, 1, 1, 0, IDENTIFIED_FLAG, NS_HELP_AJOIN_SHORT, 
  NS_HELP_AJOIN_LONG, NULL
};

static struct ServiceMessage access_sub[] = {
  { NULL, "ADD", 0, 1, 1, 0, IDENTIFIED_FLAG, NS_HELP_ACCESS_ADD_SHORT, 
    NS_HELP_ACCESS_ADD_LONG, m_access_add },
  { NULL, "LIST", 0, 0, 0, 0, IDENTIFIED_FLAG, NS_HELP_ACCESS_LIST_SHORT, 
    NS_HELP_ACCESS_LIST_LONG, m_access_list },
  { NULL, "DEL", 0, 1, 1, 0, IDENTIFIED_FLAG, NS_HELP_ACCESS_DEL_SHORT, 
    NS_HELP_ACCESS_DEL_LONG, m_access_del },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage access_msgtab = {
  access_sub, "ACCESS", 0, 1, 1, 0, IDENTIFIED_FLAG, NS_HELP_ACCESS_SHORT, NS_HELP_ACCESS_LONG,
  NULL
};

static struct ServiceMessage ghost_msgtab = {
  NULL, "GHOST", 0, 2, 2, SFLG_ALIAS, USER_FLAG, NS_HELP_REGAIN_SHORT, 
  NS_HELP_REGAIN_LONG, m_regain
};

static struct ServiceMessage link_msgtab = {
  NULL, "LINK", 0, 1, 2, 0, IDENTIFIED_FLAG, NS_HELP_LINK_SHORT, NS_HELP_LINK_LONG,
  m_link
};

static struct ServiceMessage unlink_msgtab = {
  NULL, "UNLINK", 0, 0, 0, 0, IDENTIFIED_FLAG, NS_HELP_UNLINK_SHORT, NS_HELP_UNLINK_LONG,
  m_unlink
};

static struct ServiceMessage info_msgtab = {
  NULL, "INFO", 0, 0, 1, 0, USER_FLAG, NS_HELP_INFO_SHORT, NS_HELP_INFO_LONG,
  m_info
};

static struct ServiceMessage forbid_msgtab = {
  NULL, "FORBID", 0, 1, 2, 0, ADMIN_FLAG, NS_HELP_FORBID_SHORT, 
  NS_HELP_FORBID_LONG, m_forbid
};

static struct ServiceMessage unforbid_msgtab = {
  NULL, "UNFORBID", 0, 1, 1, 0, ADMIN_FLAG, NS_HELP_UNFORBID_SHORT,
  NS_HELP_UNFORBID_LONG, m_unforbid
};

static struct ServiceMessage regain_msgtab = {
  NULL, "REGAIN", 0, 1, 2, 0, USER_FLAG, NS_HELP_REGAIN_SHORT, 
  NS_HELP_REGAIN_LONG, m_regain
};

static struct ServiceMessage reclaim_msgtab = {
  NULL, "RECLAIM", 0, 1, 2, 0, USER_FLAG, NS_HELP_REGAIN_SHORT, 
  NS_HELP_REGAIN_LONG, m_regain
};

static struct ServiceMessage sudo_msgtab = {
  NULL, "SUDO", 0, 2, 2, SFLG_NOMAXPARAM, ADMIN_FLAG, NS_HELP_SUDO_SHORT,
  NS_HELP_SUDO_LONG, m_sudo
};

static struct ServiceMessage sendpass_msgtab = {
  NULL, "SENDPASS", 0, 1, 3, 0, USER_FLAG, NS_HELP_SENDPASS_SHORT,
  NS_HELP_SENDPASS_LONG, m_sendpass
};

static struct ServiceMessage list_msgtab = {
  NULL, "LIST", 0, 1, 2, 0, USER_FLAG, NS_HELP_LIST_SHORT,
  NS_HELP_LIST_LONG, m_list
};

static struct ServiceMessage status_msgtab = {
  NULL, "STATUS", 0, 1, 1, 0, USER_FLAG, NS_HELP_STATUS_SHORT,
  NS_HELP_STATUS_LONG, m_status
};

static struct ServiceMessage enslave_msgtab = {
  NULL, "ENSLAVE", 0, 1, 2, 0, IDENTIFIED_FLAG, NS_HELP_ENSLAVE_SHORT, 
  NS_HELP_ENSLAVE_LONG, m_enslave
};

static struct ServiceMessage dropnick_msgtab = {
  NULL, "DROPNICK", 0, 1, 1, 0, OPER_FLAG, NS_HELP_DROPNICK_SHORT,
    NS_HELP_DROPNICK_LONG, m_dropnick
};

static struct ServiceMessage checkverify_msgtab = {
  NULL, "CHECKVERIFY", 0, 0, 1, 0, USER_FLAG, NS_HELP_CHECKVERIFY_SHORT,
  NS_HELP_CHECKVERIFY_LONG, m_checkverify
};

INIT_MODULE(nickserv, "$Revision$")
{
  nickserv = make_service("NickServ");
  clear_serv_tree_parse(&nickserv->msg_tree);
  dlinkAdd(nickserv, &nickserv->node, &services_list);
  hash_add_service(nickserv);
  nickserv_client = introduce_client(nickserv->name, nickserv->name, TRUE);
  load_language(nickserv->languages, "nickserv.en");

  mod_add_servcmd(&nickserv->msg_tree, &drop_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &identify_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &help_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &register_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &set_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &access_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &cert_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &ajoin_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &ghost_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &link_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &unlink_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &info_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &forbid_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &unforbid_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &regain_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &reclaim_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &id_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &sudo_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &cloakstring_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &sendpass_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &list_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &status_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &enslave_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &resetpass_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &dropnick_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &checkverify_msgtab);

  ns_umode_hook       = install_hook(on_umode_change_cb, ns_on_umode_change);
  ns_nick_hook        = install_hook(on_nick_change_cb, ns_on_nick_change);
  ns_newuser_hook     = install_hook(on_newuser_cb, ns_on_newuser);
  ns_quit_hook        = install_hook(on_quit_cb, ns_on_quit);
  ns_certfp_hook      = install_hook(on_certfp_cb, ns_on_certfp);
  ns_on_auth_req_hook = install_hook(on_auth_request_cb, ns_on_auth_requested);
  ns_on_identify_hook = install_hook(on_identify_cb, ns_on_identify);
  
  guest_number = 0;

  eventAdd("process nickserv enforce list", process_enforce_list, NULL, 10);
  eventAdd("process nickserv release list", process_release_list, NULL, 60);
  return nickserv;
}

CLEANUP_MODULE
{
  uninstall_hook(on_umode_change_cb, ns_on_umode_change);
  uninstall_hook(on_nick_change_cb, ns_on_nick_change);
  uninstall_hook(on_newuser_cb, ns_on_newuser);
  uninstall_hook(on_quit_cb, ns_on_quit);
  uninstall_hook(on_certfp_cb, ns_on_certfp);
  uninstall_hook(on_auth_request_cb, ns_on_auth_requested);
  uninstall_hook(on_identify_cb, ns_on_identify);
  eventDelete(process_enforce_list, NULL);
  eventDelete(process_release_list, NULL);
  serv_clear_messages(nickserv);
  exit_client(nickserv_client, &me, "Service unloaded");
  unload_languages(nickserv->languages);
  hash_del_service(nickserv);
  ilog(L_DEBUG, "Unloaded nickserv");
}

static void
guest_user(struct Client *user)
{
  char newname[NICKLEN+1];

  snprintf(newname, NICKLEN, "%s%d", "Guest", guest_number++);
  while((find_client(newname)) != NULL)
  {
    snprintf(newname, NICKLEN, "%s%d", "Guest", guest_number++);
  }
  send_nick_change(nickserv, user, newname);
}

static void
process_enforce_list(void *param)
{
  dlink_node *ptr, *ptr_next;
  
  DLINK_FOREACH_SAFE(ptr, ptr_next, nick_enforce_list.head)
  {
    struct Client *user = (struct Client *)ptr->data;  

    if(CurrentTime > user->enforce_time)
    {
      dlinkDelete(ptr, &nick_enforce_list);
      free_dlink_node(ptr);
      user->enforce_time = 0;

      SetEnforce(user);
      guest_user(user);
   }
  }
}

static void
release_client(struct Client *user, dlink_node *ptr, const char *reason)
{
  if (ptr == NULL)
  {
    dlinkFindDelete(&nick_release_list, user);
  }
  else
  {
    dlinkDelete(ptr, &nick_release_list);
    free_dlink_node(ptr);
  }

  user->enforce_time = 0;
  exit_client(user, &me, reason);
}

static void
process_release_list(void *param)
{
  dlink_node *ptr, *ptr_next;

  DLINK_FOREACH_SAFE(ptr, ptr_next, nick_release_list.head)
  {
    struct Client *user = (struct Client *)ptr->data;

    if(CurrentTime > user->release_time)
    {
      release_client(user, ptr, "Held nickname released");
    }
  }
}

static void
handle_nick_change(struct Service *service, struct Client *client, 
    const char *name, unsigned int message)
{
  struct Client *target;

  if((target = find_client(name)) != NULL)
  {
    if(MyConnect(target))
    {
      dlinkFindDelete(&nick_release_list, target);
      exit_client(target, &me, "Enforcer no longer needed");
      reply_user(service, service, client, message, name);
      send_nick_change(service, client, name);
    }
    else if(target != client)
    {
      target->release_to = client->name;
      strlcpy(target->release_name, name, NICKLEN);
      guest_user(target);
      reply_user(service, service, client, message, name);
    }
    else
    {
      identify_user(client);
      reply_user(service, service, client, message, name);
    }
  }
  else
  {
    reply_user(service, service, client, message, name);
    send_nick_change(service, client, name);
  }

  dlinkFindDelete(&nick_enforce_list, client);
  SetIdentified(client);
}

static void 
m_register(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  Nickname *nick;

  if(strncasecmp(client->name, "guest", 5) == 0)
  {
    reply_user(service, service, client, NS_NOREG_GUEST);
    return;
  }

  if(nickname_is_forbid(client->name))
  {
    reply_user(service, service, client, NS_NOREG_FORBID, client->name);
    return;
  }

  if(strchr(parv[2], '@') == NULL)
  {
    reply_user(service, service, client, NS_INVALID_EMAIL, parv[2]);
    return;
  }

  if(client->nickname != NULL)
  {
    reply_user(service, service, client, NS_ALREADY_REG, client->name);
    return;
  }

  if(strlen(client->name) < 2) /* TODO XXX FIXME configurable? */
  {
    reply_user(service, service, client, NS_REG_FAIL_TOOSHORT);
    return;
  }

  if(CurrentTime - client->firsttime < 60) /* TODO XXX FIXME configurable? */
  {
    ilog(L_NOTICE, "Warning: %s tried to register after %ld seconds online",
        client->name, CurrentTime - client->firsttime);
  }

  if((nick = nickname_find(client->name)) != NULL)
  {
    reply_user(service, service, client, NS_ALREADY_REG, client->name); 
    nickname_free(nick);
    return;
  }

  nick = nickname_new();

  set_nickname_password(nick, parv[1]);

  nickname_set_nick(nick, client->name);
  nickname_set_email(nick, parv[2]);

  if(nickname_register(nick))
  {
    client->nickname = nick;
    identify_user(client);

    reply_user(service, service, client, NS_REG_COMPLETE, client->name);
    global_notice(NULL, "%s!%s@%s registered nick %s\n", client->name, 
        client->username, client->host, nickname_get_nick(nick));

    execute_callback(on_nick_reg_cb, client);
    return;
  }
  reply_user(service, service, client, NS_REG_FAIL, client->name);
}

static void
m_drop(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  struct Client *target;
  Nickname *nick = nickname_find(client->name);
  char *target_nick = NULL;
  char *masterless_channel = NULL;

  assert(nick != NULL);

  /* This might be being executed via sudo, find the real user of the nick */
  if(nickname_get_id(client->nickname) != nickname_get_id(nick))
  {
    target_nick = nickname_nick_from_id(nickname_get_nickid(client->nickname), FALSE);
    target = find_client(target_nick);
  }
  else
  {
    target = client;
    target_nick = client->name;
  }

  nickname_free(nick);

  if(target == client)
  {
    /* A normal user dropping their own nick */
    if(parc == 0)
    {
      /* No auth code, so give them one and do nothing else*/
      char buf[IRC_BUFSIZE+1] = {0};
      char *hmac;

      snprintf(buf, IRC_BUFSIZE, "DROP %ld %d %s", CurrentTime, 
          nickname_get_nickid(target->nickname), target->name);
      hmac = generate_hmac(buf);

      reply_user(service, service, client, NS_DROP_AUTH, service->name, 
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
        reply_user(service, service, client, NS_DROP_AUTH_FAIL, client->name);
        return;
      }

      *auth = '\0';
      auth++;

      snprintf(buf, IRC_BUFSIZE, "DROP %s %d %s", parv[1], 
          nickname_get_nickid(target->nickname), target->name);
      hmac = generate_hmac(buf);

      if(strncmp(hmac, auth, strlen(hmac)) != 0)
      {
        MyFree(hmac);
        reply_user(service, service, client, NS_DROP_AUTH_FAIL, client->name);
        return;
      }

      MyFree(hmac);
      timestamp = atoi(parv[1]);
      if((CurrentTime - timestamp) > 3600)
      {
        reply_user(service, service, client, NS_DROP_AUTH_FAIL, client->name);
        return;
      }
    }
  }

  /* Authentication passed(possibly because they're using sudo), go ahead and
   * drop
   */

  masterless_channel = check_masterless_channels(nickname_get_id(client->nickname));

  if(masterless_channel != NULL)
  {
    reply_user(service, service, client, NS_DROP_FAIL_MASTERLESS, target_nick, masterless_channel);
    MyFree(masterless_channel);
    if(target != client)
      MyFree(target_nick);
    return;
  }

  if(nickname_delete(client->nickname)) 
  {
    if(target != NULL)
    {
      ClearIdentified(target);
      if(target->nickname != NULL)
        nickname_free(target->nickname);
      target->nickname = NULL;
      target->access = USER_FLAG;
      send_umode(nickserv, target, "-R");
      reply_user(service, service, client, NS_NICK_DROPPED, target->name);
      ilog(L_NOTICE, "%s!%s@%s dropped nick %s", client->name, 
        client->username, client->host, target->name);
    }
    else
    {
      reply_user(service, service, client, NS_NICK_DROPPED, target_nick);
      ilog(L_NOTICE, "%s!%s@%s dropped nick %s", client->name, 
        client->username, client->host, target_nick);
      target = find_client(target_nick);
      if (target)
      {
        release_client(target, NULL, "Nick has been dropped");
      }
    }
  }
  else
  {
    reply_user(service, service, client, NS_NICK_DROPFAIL, target_nick);
  }

  if(target != client)
    MyFree(target_nick);
}

static void
m_identify(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  Nickname *nick;
  const char *name;

  if(parc > 1)
    name = parv[2];
  else
    name = client->name;

  if((nick = nickname_find(name)) == NULL)
  {
    reply_user(service, service, client, NS_REG_FIRST, name);
    return;
  }

  if(!check_nick_pass(client, nick, parv[1]))
  {
    nickname_free(nick);
    if(++client->num_badpass > 5)
    {
      kill_user(service, client, "Too many failed password attempts.");
      return;
    }
    reply_user(service, service, client, NS_IDENT_FAIL, name);
    return;
  }

  if(client->nickname != NULL)
    nickname_free(client->nickname);

  client->nickname = nick;
  dlinkFindDelete(&nick_enforce_list, client);

  if(parc > 1)
    handle_nick_change(service, client, name, NS_IDENTIFIED);
  else
  {
    identify_user(client);
    reply_user(service, service, client, NS_IDENTIFIED, name);
  }
}

static void
m_help(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  do_help(service, client, parv[1], parc, parv);
}

static void
m_set_password(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  Nickname *nick;
  
  nick = client->nickname;

  if(set_nickname_password(nick, parv[1]))
    reply_user(service, service, client, NS_SET_PASS_SUCCESS);
  else
    reply_user(service, service, client, NS_SET_PASS_FAILED);
}

static void
m_set_language(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  /* No parameters, list languages */
  if(parc == 0)
  {
    int i;

    reply_user(service, service, client, NS_CURR_LANGUAGE,
        service->languages[nickname_get_language(client->nickname)].name,
        nickname_get_language(client->nickname));

    reply_user(service, service, client, NS_AVAIL_LANGUAGE);

    for(i = 0; i < LANG_LAST; i++)
    {
      if(service->languages[i].name != NULL)
      {
        reply_user(service, service, client, NS_LANG_LIST, i, 
            service->languages[i].name);
      }
    }
  }
  else
  {
    int lang = atoi(parv[1]);

    if(lang > LANG_LAST || lang < 0 || service->languages[lang].name == NULL)
    {
      reply_user(service, service, client, NS_LANGUAGE_UNAVAIL);
      return;
    }

    if(nickname_set_language(client->nickname, lang))
    {
      reply_user(service, service, client, NS_LANGUAGE_SET,
          service->languages[lang].name, lang); 
    }
    else
      reply_user(service, service, client, NS_SET_FAILED, "LANGUAGE", parv[1]);
  }
}

static void
m_set_url(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  m_set_string(service, client, "URL", parv[1], parc,
    &nickname_get_url, &nickname_set_url);
}

static void
m_set_email(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  m_set_string(service, client, "EMAIL", parv[1], parc,
    &nickname_get_email, &nickname_set_email);
}

static void
m_set_cloak(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  unsigned char old = nickname_get_cloak_on(client->nickname);
  Nickname *nick;

  m_set_flag(service, client, parv[1], "CLOAK",
    &nickname_get_cloak_on, &nickname_set_cloak_on);

  if (!old && nickname_get_cloak_on(client->nickname))
  {
    nick = nickname_find(client->name);

    if (nickname_get_id(client->nickname) == nickname_get_id(nick))
      do_cloak(client);

    nickname_free(nick);
  }
}

static void
m_cloakstring(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  Nickname *nick = NULL;
  struct Client *target = find_client(parv[1]);

  if (target != NULL)
    nick = target->nickname;

  /* They may be connected but not identified */
  if (nick == NULL)
  {
    target = NULL;
    nick = nickname_find(parv[1]);
  }

  if(nick == NULL)
  {
    reply_user(service, service, client, NS_REG_FIRST, parv[1]);
    return;
  }
 
  if(parc == 1)
  {
    if(EmptyString(nickname_get_cloak(nick)))
      reply_user(service, service, client, NS_SET_VALUE, "CLOAKSTRING", "Not Set");
    else
      reply_user(service, service, client, NS_SET_VALUE, "CLOAKSTRING", nickname_get_cloak(nick));

    if (target == NULL)
      nickname_free(nick);

    return;
  }

  if(irccmp(parv[2], "-") == 0)
    parv[2] = NULL;
  else
  {
    if(!valid_hostname(parv[2]))
    {
      if (target == NULL)
        nickname_free(nick);

      reply_user(service, service, client, NS_INVALID_CLOAK, parv[2]);
      return;
    }
  }

  if(nickname_set_cloak(nick, parv[2]))
  {
    char *reply = parv[2] == NULL ? "Not set" : parv[2];

    if(parv[2] == NULL)
      reply = "Not set";
    else
      reply = parv[2];

    reply_user(service, service, client, NS_SET_SUCCESS, "CLOAKSTRING", 
        reply);
    ilog(L_NOTICE, "%s set CLOAKSTRING of %s to %s", client->name, nickname_get_nick(nick),
        reply);

    if(target != NULL)
      do_cloak(target);
  }
  else
    reply_user(service, service, client, NS_SET_FAILED, "CLOAKSTRING", parv[2]);

  if (target == NULL)
    nickname_free(nick);
}

static void
m_resetpass(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  Nickname *nick = nickname_find(parv[1]);
  char *new_pass;

  if(nick == NULL)
  {
    reply_user(service, service, client, NS_REG_FIRST, parv[1]);
    return;
  }

  if(!nickname_reset_pass(nick, &new_pass))
  {
    reply_user(service, service, client, NS_RESETPASS_FAIL);
  }
  else
  {
    reply_user(service, service, client, NS_RESETPASS_SUCCESS, parv[1], new_pass);
    MyFree(new_pass);
    ilog(L_NOTICE, "%s reset the password for %s", client->name, parv[1]);
  }

  nickname_free(nick);
}

static void
m_set_secure(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], "SECURE",
    &nickname_get_secure, &nickname_set_secure);
}

static void
m_set_enforce(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], "ENFORCE",
    &nickname_get_enforce, &nickname_set_enforce);
}

static void
m_set_master(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  Nickname *nick = client->nickname;

  if(parc == 0)
  {
    char *prinick = nickname_nick_from_id(nickname_get_id(nick), TRUE);

    reply_user(service, service, client, NS_SET_VALUE, "MASTER", prinick);
    MyFree(prinick);
    return;
  }

  if(nickname_id_from_nick(parv[1], TRUE) != nickname_get_id(nick))
  {
    reply_user(service, service, client, NS_MASTER_NOT_LINKED, parv[1]);
    return;
  }

  if(nickname_set_master(nick, parv[1]))
    reply_user(service, service, client, NS_MASTER_SET_OK, parv[1]);
  else
    reply_user(service, service, client, NS_MASTER_SET_FAIL, parv[1]);
}

static void
m_set_private(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  m_set_flag(service, client, parv[1], "PRIVATE",
    &nickname_get_priv, &nickname_set_priv);
}


static void
m_access_add(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  Nickname *nick = client->nickname;
  struct AccessEntry access;

  if(strchr(parv[1], '@') == NULL)
  {
    reply_user(service, service, client, NS_ACCESS_INVALID, parv[1]);
    return;
  }

  access.id = nickname_get_id(nick);
  access.value = parv[1];
  
  if(nickname_accesslist_add(&access))
    reply_user(service, service, client, NS_ACCESS_ADD, parv[1]);
  else
    reply_user(service, service, client, NS_ACCESS_ADDFAIL, parv[1]);
}

static void
m_access_list(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  Nickname *nick;
  struct AccessEntry *entry = NULL;
  dlink_list list = { 0 };
  dlink_node *ptr;
  int i = 1;

  nick = client->nickname;


  nickname_accesslist_list(nick, &list);

  if(dlink_list_length(&list) == 0)
    reply_user(service, service, client, NS_ACCESS_LIST_NONE);
  else
    reply_user(service, service, client, NS_ACCESS_START);

  DLINK_FOREACH(ptr, list.head)
  {
    entry = ptr->data;
    reply_user(service, service, client, NS_ACCESS_ENTRY, i++, entry->value);
  }

  nickname_accesslist_free(&list);
}

static void
m_access_del(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  Nickname *nick = client->nickname;
  int ret;

  ret = nickname_accesslist_delete(nick, parv[1]);
  if(ret > 0)
    reply_user(service, service, client, NS_ACCESS_DEL, parv[1]);
  else if(ret == 0)
    reply_user(service, service, client, NS_ACCESS_DEL_NONE, parv[1]);
  else
    reply_user(service, service, client, NS_ACCESS_DEL_ERROR);
}

static void
m_cert_add(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  Nickname *nick = client->nickname;
  Nickname *dest_nick = NULL;
  char *certfp, *nickstr;
  struct AccessEntry access = { 0 };
  unsigned int id, nickid;

  id = nickid = 0;

  /* 
   * If we only have one paramter, it's either a nickname of a certfp.  If
   * it's a nick we take the cert from the client. If we have two parameters, 
   * we should have a cert followed by a nickname.
   *
   */
  if(parc == 0)
  {
    certfp = client->certfp;
    nickstr = nick->nick;
  }
  else if(parc == 1)
  {
    certfp = client->certfp;
    dest_nick = nickname_find(parv[1]);
    if(dest_nick != NULL)
    {
      if(*certfp == '\0')
      {
        reply_user(service, service, client, NS_CERT_YOUHAVENONE);
        return;
      }
      nickstr = parv[1];
    }
    else
    {
      certfp = parv[1];
      nickstr = nick->nick;
    }
  }
  else
  {
    certfp = parv[1];
    nickstr = parv[2];
    if((dest_nick = nickname_find(nickstr)) == NULL)
    {
      reply_user(service, service, client, NS_CERT_ADDFAIL_NONICK, nickstr);
      return;
    }
  }

  if(dest_nick != NULL)
  {
    id = nickname_get_id(dest_nick);
    nickid = nickname_get_nickid(dest_nick);
    nickname_free(dest_nick);
  }
  else
    nickid = 0;

  if(strlen(certfp) != 40)
  {
    reply_user(service, service, client, NS_CERT_INVALID, certfp);
    return;
  }

  if(nickname_cert_check(nick, certfp, NULL))
  {
    reply_user(service, service, client, NS_CERT_EXISTS, certfp);
    return;
  }

  if(id != 0 && nickname_get_id(nick) != id)
  {
    reply_user(service, service, client, NS_CERT_ADDFAIL_NOTYOURNICK, nickstr);
    return;
  }

  access.id = nickname_get_id(nick);
  access.value = certfp;
  access.nickname_id = nickid;

  if(nickname_cert_add(&access))
    reply_user(service, service, client, NS_CERT_ADD, access.value, nickstr);
  else
    reply_user(service, service, client, NS_CERT_ADDFAIL, access.value, nickstr);
}

static void
m_cert_list(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  Nickname *nick;
  struct AccessEntry *entry = NULL;
  dlink_list list = { 0 };
  dlink_node *ptr;
  int i = 1;

  nick = client->nickname;

  nickname_cert_list(nick, &list);

  if(dlink_list_length(&list) == 0)
  {
    reply_user(service, service, client, NS_CERT_EMPTY);
    return;
  }

  reply_user(service, service, client, NS_CERT_START);

  DLINK_FOREACH(ptr, list.head)
  {
    char *dest_nick;
    entry = (struct AccessEntry *)ptr->data;
    
    if(entry->nickname_id > 0)
      dest_nick = nickname_nick_from_id(entry->nickname_id, FALSE);
    else
      dest_nick = "";

    reply_user(service, service, client, NS_CERT_ENTRY, i++, entry->value,
      dest_nick);
  }

  nickname_certlist_free(&list);
}

static void
m_cert_del(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  Nickname *nick = client->nickname;
  int ret;

  ret = nickname_cert_delete(nick, parv[1]);

  if(ret < 0)
    reply_user(service, service, client, NS_CERT_DEL_ERROR);
  else if(ret == 0)
    reply_user(service, service, client, NS_CERT_DEL_NONE, parv[1]);
  else if(ret > 0)
    reply_user(service, service, client, NS_CERT_DEL, parv[1]);
}

static void
m_ajoin_add(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  Nickname *nick = client->nickname;
  DBChannel *chan;

  if(*parv[1] != '#')
  {
    reply_user(service, service, client, NS_AJOIN_INVALID_CHAN, parv[1]);
    return;
  }

  chan = dbchannel_find(parv[1]);
  if(chan == NULL)
  {
    reply_user(service, service, client, NS_AJOIN_CHAN_NOT_REG, parv[1]);
    return;
  }

  if(nickname_ajoin_add(nickname_get_id(nick), dbchannel_get_id(chan)))
    reply_user(service, service, client, NS_AJOIN_ADD, parv[1]);
  else
    reply_user(service, service, client, NS_AJOIN_ADDFAIL, parv[1]);

  dbchannel_free(chan);
}

static void
m_ajoin_list(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  Nickname *nick;
  dlink_list list = { 0 };
  dlink_node *ptr;
  int i = 1;

  nick = client->nickname;

  nickname_ajoin_list(nick->id, &list);

  if(dlink_list_length(&list) == 0)
    reply_user(service, service, client, NS_AJOIN_LIST_NONE);
  else
    reply_user(service, service, client, NS_AJOIN_START);

  DLINK_FOREACH(ptr, list.head)
  {
    char *ajoin;
    ajoin = ptr->data;
    reply_user(service, service, client, NS_AJOIN_ENTRY, i++, ajoin);
  }

  nickname_ajoinlist_free(&list);
}

static void
m_ajoin_del(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  Nickname *nick = client->nickname;
  DBChannel *chan;
  int ret;

  chan = dbchannel_find(parv[1]);
  if(chan == NULL)
  {
    reply_user(service, service, client, NS_AJOIN_CHAN_NOT_REG, parv[1]);
    return;
  }

  ret = nickname_ajoin_delete(nick->id, chan->id);

  if(ret > 0)
    reply_user(service, service, client, NS_AJOIN_DEL, parv[1]);
  else if(ret == 0)
    reply_user(service, service, client, NS_AJOIN_DEL_NONE, parv[1]);
  else
    reply_user(service, service, client, NS_AJOIN_DEL_ERROR);

  dbchannel_free(chan);
}

static void
m_regain(struct Service *service, struct Client *client, int parc, char *parv[])
{
  Nickname *nick;

  if(find_client(parv[1]) == NULL)
  {
    reply_user(service, service, client, NS_REGAIN_NOTONLINE, parv[1]);
    return;
  }

  if((nick = nickname_find(parv[1])) == NULL)
  {
    reply_user(service, service, client, NS_REG_FIRST, parv[1]);
    return;
  }

  if(parc == 1 && (!check_nick_pass(client, nick, "") && (nickname_get_secure(nick) || !IsOnAccess(client))))
  {
    nickname_free(nick);
    reply_user(service, service, client, NS_REGAIN_FAILED_SECURITY, parv[1]);   
    return;
  }

  if((parc == 2 && !check_nick_pass(client, nick, parv[2])))
  {
    nickname_free(nick);
    reply_user(service, service, client, NS_REGAIN_FAILED, parv[1]);   
    return;
  }
  
  if(client->nickname != NULL)
    nickname_free(client->nickname);

  client->nickname = nick;

  handle_nick_change(service, client, parv[1], NS_REGAIN_SUCCESS);
}

static void
m_link(struct Service *service, struct Client *client, int parc, char *parv[])
{
  Nickname *nick, *master_nick;

  nick = client->nickname;
  if((master_nick = nickname_find(parv[1])) == NULL)
  {
    reply_user(service, service, client, NS_LINK_NOMASTER, parv[1]);
    return;
  }

  if(nickname_get_id(master_nick) == nickname_get_id(nick))
  {
    nickname_free(master_nick);
    reply_user(service, service, client, NS_LINK_NOSELF);
    return;
  }

  if(parv[2] == NULL || !check_nick_pass(client, master_nick, parv[2]))
  {
    nickname_free(master_nick);
    reply_user(service, service, client, NS_LINK_BADPASS, parv[1]);
    return;
  }

  if(!nickname_get_verified(master_nick) || !nickname_get_verified(nick))
  {
    nickname_free(master_nick);
    reply_user(service, service, client, NS_LINK_NOTVERIFIED, nickname_get_nick(nick), parv[1]);
    return;
  }

  if(!nickname_link(master_nick, nick))
  {
    nickname_free(master_nick);
    reply_user(service, service, client, NS_LINK_FAIL, parv[1]);
    return;
  }

  reply_user(service, service, client, NS_LINK_OK, nickname_get_nick(nick), parv[1]);
  
  nickname_free(master_nick);
  nickname_free(nick);

  client->nickname = nickname_find(parv[1]);
  assert(client->nickname != NULL);
}

static void
m_unlink(struct Service *service, struct Client *client, int parc, char *parv[])
{
  Nickname *nick = client->nickname;
  unsigned int newid;

  if(nickname_link_count(nick) == 0)
  {
    reply_user(service, service, client, NS_UNLINK_FAILED, nickname_get_nick(nick));
    return;
  }

  newid = nickname_unlink(nick);

  if(newid > 0)
  {
    reply_user(service, service, client, NS_UNLINK_OK, nickname_get_nick(nick));
    // In case this was a slave nick, it is now a master of itself
    nickname_free(client->nickname);
    nickname_set_id(nick, newid);
    client->nickname = nickname_find(client->name);
  }
  else
    reply_user(service, service, client, NS_UNLINK_FAILED, nickname_get_nick(nick));
}

static void
m_info(struct Service *service, struct Client *client, int parc, char *parv[])
{
  Nickname *nick;
  struct Client *target;
  struct InfoList *chan = NULL;
  char *name;
  char *link;
  char buf[IRC_BUFSIZE+1] = {0};
  int online = 0;
  int comma;
  dlink_node *ptr;
  dlink_list list = { 0 };

  if(parc == 0)
  {
    if(client->nickname != NULL)
      nick = client->nickname;
    else
    {
      if(nickname_is_forbid(client->name))
      {
        reply_user(service, service, client, NS_NICKFORBID, client->name);
        return;
      }
      if((nick = nickname_find(client->name)) == NULL)
      {
        reply_user(service, service, client, NS_REG_FIRST, client->name);
        return;
      }
   }
    name = client->name;
  }
  else
  {
    if(nickname_is_forbid(parv[1]))
    {
      reply_user(service, service, client, NS_NICKFORBID, parv[1]);
      return;
    }

    if((nick = nickname_find(parv[1])) == NULL)
    {
      reply_user(service, service, client, NS_REG_FIRST,
          parv[1]);
      return;
    }
    name = parv[1];
  }

  reply_user(service, service, client, NS_INFO_START, name, 
      nickname_get_last_realname(nick) != NULL ? nickname_get_last_realname(nick) : "Unknown");

  nickname_link_list(nickname_get_id(nick), &list);
  comma = 0;
  DLINK_FOREACH(ptr, list.head)
  {
    link = (char *)ptr->data;

    if(irccmp(link, name) == 0)
    {
      if((target = find_client(name)) != NULL && IsIdentified(target))
      {
        reply_user(service, service, client, NS_INFO_ONLINE_NONICK, name);
        online = 1;
      }
      continue;
    }

    if((target = find_client(link)) != NULL && IsIdentified(target))
    {
      reply_user(service, service, client, NS_INFO_ONLINE, name, link);
      online = 1;
    }

    if(comma)
      strlcat(buf, ", ", sizeof(buf));
    strlcat(buf, link, sizeof(buf));
    if(!comma)
      comma = 1;
  }

  nickname_link_list_free(&list);

  if(!online)
    reply_time(service, client, NS_INFO_SEENTIME_FULL, nickname_get_last_seen(nick));

  reply_time(service, client, NS_INFO_REGTIME_FULL, nickname_get_reg_time(nick));
  reply_time(service, client, NS_INFO_QUITTIME_FULL, nickname_get_last_quit_time(nick));

  reply_user(service, service, client, NS_INFO,
      (nickname_get_last_quit(nick) == NULL) ? "Unknown" : nickname_get_last_quit(nick),
      (nickname_get_last_host(nick) == NULL) ? "Unknown" : nickname_get_last_host(nick),
      (nickname_get_url(nick) == NULL) ? "Not set" : nickname_get_url(nick),
      (nickname_get_cloak(nick)[0] == '\0') ? "Not set" : nickname_get_cloak(nick));

  if((IsIdentified(client) && (nickname_get_id(client->nickname) == nickname_get_id(nick))) || 
      client->access >= OPER_FLAG)
  {
    reply_user(service, service, client, NS_INFO_EMAIL, nickname_get_email(nick));
    reply_user(service, service, client, NS_INFO_LANGUAGE,
        service->languages[nickname_get_language(nick)].name, nickname_get_language(nick)); 

    reply_user(service, service, client, NS_INFO_OPTION, "ENFORCE", nickname_get_enforce(nick) ? "ON" :
        "OFF");
    reply_user(service, service, client, NS_INFO_OPTION, "SECURE", nickname_get_secure(nick) ? "ON" :
        "OFF");
    reply_user(service, service, client, NS_INFO_OPTION, "PRIVATE", nickname_get_priv(nick) ? "ON" :
        "OFF");
    reply_user(service, service, client, NS_INFO_OPTION, "CLOAK", nickname_get_cloak_on(nick) ? "ON" :
        "OFF");
    reply_user(service, service, client, NS_INFO_OPTION, "VERIFIED", nickname_get_verified(nick) ? "YES" :
        "NO");

    if(*buf != '\0')
      reply_user(service, service, client, NS_INFO_LINKS, buf);

    if(nickname_get_nickid(nick) != nickname_get_pri_nickid(nick))
    {
      char *prinick = nickname_nick_from_id(nickname_get_id(nick), TRUE);

      reply_user(service, service, client, NS_INFO_MASTER, prinick);
      MyFree(prinick);
    }

    if(nickname_group_list(nickname_get_id(nick), &list))
    {
      reply_user(service, service, client, NS_INFO_GROUPS);
      DLINK_FOREACH(ptr, list.head)
      {
        chan = (struct InfoList *)ptr->data;
        reply_user(service, service, client, NS_INFO_GROUP, chan->name,
            chan->level);
      }
      nickname_group_list_free(&list);
    }

    if(nickname_chan_list(nickname_get_id(nick), &list))
    {
      reply_user(service, service, client, NS_INFO_CHANS);
      DLINK_FOREACH(ptr, list.head)
      {
        chan = (struct InfoList *)ptr->data;
        reply_user(service, service, client, NS_INFO_CHAN, chan->name,
            chan->level);
      }
      nickname_chan_list_free(&list);
    }
  }
  else
  {
    if(!nickname_get_priv(nick))
      reply_user(service, service, client, NS_INFO_EMAIL, nickname_get_email(nick));
    
    reply_user(service, service, client, NS_INFO_OPTION, "VERIFIED", nickname_get_verified(nick) ? "YES" :
        "NO");
  }

  if(nick != client->nickname)
    nickname_free(nick);
}

static void
m_forbid(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Client *target;
  char *resv = parv[1];
  char duration_char;
  time_t duration = -1;

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
        reply_user(service, service, client, NS_FORBID_BAD_DURATIONCHAR,
            duration_char);
        return;
    }
  }
  else
    duration = ServicesInfo.def_forbid_dur;
  
  if(duration == -1)
    duration = 0;

  if(!nickname_forbid(resv))
  {
    reply_user(service, service, client, NS_FORBID_FAIL, parv[1]);
    return;
  }

  send_resv(service, resv, "Forbidden nickname", duration);

  if((target = find_client(parv[1])) != NULL)
  {
    reply_user(service, service, target, NS_NICK_FORBID_IWILLCHANGE, 
        target->name);
    target->enforce_time = CurrentTime + 10; /* XXX configurable? */
    dlinkAdd(target, make_dlink_node(), &nick_enforce_list);
  }
  reply_user(service, service, client, NS_FORBID_OK, parv[1]);
}

static void
m_unforbid(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Client *target;
  dlink_node *ptr;

  if(!nickname_is_forbid(parv[1]))
  {
    reply_user(service, service, client, NS_UNFORBID_NOT_FORBID, parv[1]);
    return;
  }

  if(nickname_delete_forbid(parv[1]))
    reply_user(service, service, client, NS_UNFORBID_OK, parv[1]);
  else
    reply_user(service, service, client, NS_UNFORBID_FAIL, parv[1]);

  target = find_client(parv[1]);

  ptr = dlinkFind(&nick_release_list, target);
  if(ptr != NULL)
  {
    exit_client(target, &me, "Held nickname released");
    dlinkDelete(ptr, &nick_release_list);
    free_dlink_node(ptr);
    target->enforce_time = 0;
  }
}

static void 
m_sudo(struct Service *service, struct Client *client, int parc, char *parv[])
{
  Nickname *oldnick, *nick;
  char **newparv;
  char buf[IRC_BUFSIZE] = { '\0' };
  int oldaccess;

  oldnick = client->nickname;
  oldaccess = client->access;

  nick = nickname_find(parv[1]);
  if(nick == NULL)
  {
    reply_user(service, service, client, NS_REG_FIRST, parv[1]);
    return;
  }

  client->nickname = nick;
  if(nickname_get_admin(nick))
    client->access = ADMIN_FLAG;
  else
    client->access = IDENTIFIED_FLAG;

  newparv = MyMalloc(4 * sizeof(char*));

  newparv[0] = parv[0];
  newparv[1] = service->name;

  join_params(buf, parc-1, &parv[2]);

  DupString(newparv[2], buf);

  ilog(L_INFO, "%s executed %s SUDO on %s: %s", client->name, service->name, 
      nickname_get_nick(nick), newparv[2]);

  process_privmsg(1, me.uplink, client, 3, newparv);
  MyFree(newparv[2]);
  MyFree(newparv);

  nickname_free(client->nickname);
  client->nickname = oldnick;
  client->access = oldaccess;
}

/* XXX This steals the concept from SUDO above, but is limited to only DROPing */
static void
m_dropnick(struct Service *service, struct Client *client, int parc, char *parv[])
{
  Nickname *oldnick, *nick;
  char **newparv;
  int oldaccess;

  oldnick = client->nickname;
  oldaccess = client->access;

  nick = nickname_find(parv[1]);
  if(nick == NULL)
  {
    reply_user(service, service, client, NS_REG_FIRST, parv[1]);
    return;
  }

  client->nickname = nick;
  if(nickname_get_admin(nick))
    client->access = ADMIN_FLAG;
  else
    client->access = IDENTIFIED_FLAG;

  newparv = MyMalloc(4 * sizeof(char*));

  newparv[0] = parv[0];
  newparv[1] = service->name;

  DupString(newparv[2], "DROP");

  ilog(L_INFO, "%s executed %s DROPNICK on %s", client->name, service->name,
      nickname_get_nick(nick));

  process_privmsg(1, me.uplink, client, 3, newparv);
  MyFree(newparv[2]);
  MyFree(newparv);

  nickname_free(client->nickname);
  client->nickname = oldnick;
  client->access = oldaccess;
}

static void 
m_sendpass(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  Nickname *nick, *temp;
  char buf[IRC_BUFSIZE+1] = {0};
  char *hmac;
  char *auth;
  int timestamp;

  if((nick = nickname_find(parv[1])) == NULL)
  {
    reply_user(service, service, client, NS_REG_FIRST, parv[1]);
    return;
  }

  if(parc == 1)
  {
    if(dbmail_is_sent(nickname_get_id(nick), nickname_get_email(nick)))
    {
      reply_user(service, service, client, NS_NO_SENDPASS_YET);
      nickname_free(nick);
      return;      
    }

    temp = client->nickname;
    client->nickname = nick;

    snprintf(buf, IRC_BUFSIZE, "SENDPASS %ld %d %s", CurrentTime, 
        nickname_get_id(nick), nickname_get_nick(nick));
    hmac = generate_hmac(buf);

    reply_user(service, service, client, NS_SENDPASS_SENT);

    reply_mail(service, client, NS_SENDPASS_SUBJECT, NS_SENDPASS_BODY,
        nickname_get_nick(nick), client->name, client->username, client->host, nickname_get_nick(nick), 
        service->name, nickname_get_nick(nick), CurrentTime, hmac);

    client->nickname = temp;

    dbmail_add_sent(nickname_get_id(nick), nickname_get_email(nick));
    nickname_free(nick);
    
    MyFree(hmac);
    return;
  }
  else if(parc == 2)
  {
    reply_user(service, service, client, NS_SENDPASS_NEED_PASS);
    nickname_free(nick);
    return;
  }
    
  if((auth = strchr(parv[2], ':')) == NULL)
  {
    reply_user(service, service, client, NS_SENDPASS_AUTH_FAIL, nickname_get_nick(nick));
    return;
  }

  *auth = '\0';
  auth++;

  snprintf(buf, IRC_BUFSIZE, "SENDPASS %s %d %s", parv[2], nickname_get_id(nick), 
      nickname_get_nick(nick));
  hmac = generate_hmac(buf);

  if(strncmp(hmac, auth, strlen(hmac)) != 0)
  {
    MyFree(hmac);
    reply_user(service, service, client, NS_SENDPASS_AUTH_FAIL, nickname_get_nick(nick));
    return;
  }

  MyFree(hmac);
  timestamp = atoi(parv[2]);
  if((CurrentTime - timestamp) > 86400)
  {
    reply_user(service, service, client, NS_SENDPASS_AUTH_FAIL, nickname_get_nick(nick));
    return;
  }

  if(set_nickname_password(nick, parv[3]))
    reply_user(service, service, client, NS_SET_PASS_SUCCESS);
  else
    reply_user(service, service, client, NS_SET_PASS_FAILED);

  nickname_free(nick);
}

static void
m_list(struct Service *service, struct Client *client, int parc, char *parv[])
{
  char *nick;
  int count = 0;
  int qcount = 0;
  dlink_node *ptr;
  dlink_list list = { 0 };

  if(parc == 2 && client->access >= OPER_FLAG)
  {
    if(irccmp(parv[2], "FORBID") == 0)
      qcount = nickname_list_forbid(&list);
    else
    {
      reply_user(service, service, client, NS_LIST_INVALID_OPTION, parv[2]);
      return;
    }
  }

  if(qcount == 0 && client->access >= OPER_FLAG)
    qcount = nickname_list_all(&list);
  else if(qcount == 0)
    qcount = nickname_list_regular(&list);

  if(qcount == 0)
  {
    reply_user(service, service, client, NS_LIST_NO_MATCHES, parv[1]);
    /* TODO XXX FIXME use proper free */
    db_string_list_free(&list);
    return;
  }

  DLINK_FOREACH(ptr, list.head)
  {
    nick = (char *)ptr->data;
    if(match(parv[1], nick))
    {
      count++;
      reply_user(service, service, client, NS_LIST_ENTRY, nick);
    }
    if(count == 50)
      break;
  }

  /* TODO XXX FIXME use proper free */
  db_string_list_free(&list);

  reply_user(service, service, client, NS_LIST_END, count);
}

static void
m_enslave(struct Service *service, struct Client *client, int parc, char *parv[])
{
  Nickname *nick, *slave_nick;

  nick = client->nickname;
  if((slave_nick = nickname_find(parv[1])) == NULL)
  {
    reply_user(service, service, client, NS_LINK_NOSLAVE, parv[1]);
    return;
  }

  if(nickname_get_id(slave_nick) == nickname_get_id(nick))
  {
    nickname_free(slave_nick);
    reply_user(service, service, client, NS_LINK_NOSELF);
    return;
  }

  if(parv[2] == NULL || !check_nick_pass(client, slave_nick, parv[2]))
  {
    nickname_free(slave_nick);
    reply_user(service, service, client, NS_LINK_BADPASS, parv[1]);
    return;
  }

  if(!nickname_get_verified(slave_nick) || !nickname_get_verified(nick))
  {
    nickname_free(slave_nick);
    reply_user(service, service, client, NS_LINK_NOTVERIFIED, nickname_get_nick(nick), parv[1]);
    return;
  }

  if(!nickname_link(nick, slave_nick))
  {
    nickname_free(slave_nick);
    reply_user(service, service, client, NS_LINK_FAIL, parv[1]);
    return;
  }

  reply_user(service, service, client, NS_LINK_OK, parv[1], nickname_get_nick(nick));
  
  nickname_free(slave_nick);
  client->nickname = nickname_find(nick->nick);
  nickname_free(nick);

  assert(client->nickname != NULL);
}

static void*
ns_on_umode_change(va_list args) 
{
  struct Client *user = va_arg(args, struct Client *);
  int what            = va_arg(args, int);
  int umode           = va_arg(args, int);

/*  if(what == MODE_ADD && umode == UMODE_IDENTIFIED)
  {
    if(user->nickname != NULL)
      nickname_free(user->nickname);
    user->nickname = nickname_find(user->name);
    if(user->nickname != NULL)
    {
      dlinkFindDelete(&nick_enforce_list, user);
      identify_user(user);
    }
  }*/
  return pass_callback(ns_umode_hook, user, what, umode);
}

static void *
ns_on_nick_change(va_list args)
{
  struct Client *user = va_arg(args, struct Client *);
  char *oldnick       = va_arg(args, char *);
  Nickname *nick_p;
  struct Client *enforcer;
  char userhost[USERHOSTLEN+1]; 
  int oldid = 0;

  if(IsEnforce(user))
  {
    introduce_client(oldnick, "Enforced Nickname (/msg nickserv help regain)", FALSE);
    enforcer = find_client(oldnick);
    enforcer->release_time = CurrentTime + (1*60*60);
    dlinkAdd(enforcer, make_dlink_node(), &nick_release_list);
    ClearEnforce(user);
  }

  /* This user was using a nick wanted elsewhere, release it now */
  if(user->release_to != NULL)
  {
    struct Client *client = find_client(user->release_to);
    struct Client *target;

    /* in the "real world" it is possible that a user gets deidenfitied in the
     * mean time, check they are still eligible for this nick.
     */
    if(client != NULL && client->nickname != NULL)
    {
      if((target = find_client(nickname_get_nick(client->nickname))) != NULL)
      {
        if(target != client)
          kill_user(nickserv, target, "This nickname is registered and protected");
      }
      send_nick_change(nickserv, client, user->release_name);
      user->release_to = NULL;
      memset(user->release_name, 0, sizeof(user->release_name));
      nickname_set_last_realname(client->nickname, client->info);
      identify_user(client);
    }
    else if(client != NULL)
    {
      if((target = find_client(user->release_name)) != NULL)
      {
        if(target != client)
          kill_user(nickserv, target, "This nickname is registered and protected");
      }
    }
  }

  ilog(L_DEBUG, "%s changing nick to %s", oldnick, user->name);
 
  dlinkFindDelete(&nick_enforce_list, user);

  if(IsIdentified(user))
  {
    ClearIdentified(user);
    user->access = USER_FLAG;
    /* XXX Use unidentify event */
    send_umode(nickserv, user, "-R");

    if(IsSentCert(user))
    {
      ClearSentCert(user);
    }

    if(user->nickname)
    {
      oldid = nickname_get_id(user->nickname);
      nickname_set_last_seen(user->nickname, CurrentTime);
      nickname_free(user->nickname);
      user->nickname = NULL;
    }
  }

  if(nickname_is_forbid(user->name))
  {
    reply_user(nickserv, nickserv, user, NS_NICK_FORBID_IWILLCHANGE, user->name);
    user->enforce_time = CurrentTime + 10; /* XXX configurable? */
    dlinkAdd(user, make_dlink_node(), &nick_enforce_list);
    return pass_callback(ns_nick_hook, user, oldnick);
  }

  if((nick_p = nickname_find(user->name)) == NULL)
  {
    ilog(L_DEBUG, "Nick Change: %s->%s(nick not registered)", oldnick, user->name);
    if(user->nickname != NULL)
    {
      nickname_free(user->nickname);
      user->nickname = NULL;
    }
    return pass_callback(ns_nick_hook, user, oldnick);
  }

  // Linked nick
  if(oldid == nickname_get_id(nick_p))
  {
    if(user->nickname != NULL)
      nickname_free(user->nickname);
    user->nickname = nick_p;
    identify_user(user);
    return pass_callback(ns_nick_hook, user, oldnick);
  }
 
  snprintf(userhost, USERHOSTLEN, "%s@%s", user->username, user->host);

  if(*user->certfp != '\0' && nickname_cert_check(nick_p, user->certfp, NULL))
  {
    if(user->nickname != NULL)
      nickname_free(user->nickname);
    user->nickname = nick_p;
    identify_user(user);
    SetSentCert(user);
    reply_user(nickserv, nickserv, user, NS_IDENTIFY_CERT, user->name);
  }
  else if(nickname_accesslist_check(nick_p, userhost))
  {
    ilog(L_DEBUG, "%s changed nick to %s(found access entry)", oldnick, 
        user->name);
    SetOnAccess(user);
    if(!nickname_get_secure(nick_p))
    {
      if(user->nickname != NULL)
        nickname_free(user->nickname);
      user->nickname = nick_p;
      identify_user(user);
      reply_user(nickserv, nickserv, user, NS_IDENTIFY_ACCESS, user->name);
    }
  }
  else
  {
    if(nickname_get_enforce(nick_p))
    {
      reply_user(nickserv, nickserv, user, NS_NICK_IN_USE_IWILLCHANGE, user->name);
      user->enforce_time = CurrentTime + 30; /* XXX configurable? */
      dlinkAdd(user, make_dlink_node(), &nick_enforce_list);
    }
    else
    {
      reply_user(nickserv, nickserv, user, NS_NICK_IN_USE, user->name);
    }
 
    ilog(L_DEBUG, "%s changed nick to %s(no access entry)", oldnick, user->name);
  }

  if(nick_p != user->nickname)
    nickname_free(nick_p);
  
  return pass_callback(ns_nick_hook, user, oldnick);
}

static void *
ns_on_newuser(va_list args)
{
  struct Client *newuser = va_arg(args, struct Client*);
  Nickname *nick_p;
  char userhost[USERHOSTLEN+1];
  
  if(IsMe(newuser->from))
    return pass_callback(ns_newuser_hook, newuser);

  ilog(L_DEBUG, "New User: %s!", newuser->name);

  if(nickname_is_forbid(newuser->name))
  {
    reply_user(nickserv, nickserv, newuser, NS_NICK_FORBID_IWILLCHANGE, newuser->name);
    newuser->enforce_time = CurrentTime + 10; /* XXX configurable? */
    dlinkAdd(newuser, make_dlink_node(), &nick_enforce_list);
    return pass_callback(ns_newuser_hook, newuser);
  }
 
  if((nick_p = nickname_find(newuser->name)) == NULL)
  {
    ilog(L_DEBUG, "New user: %s(nick not registered)", newuser->name);
    return pass_callback(ns_newuser_hook, newuser);
  }

  if(IsIdentified(newuser))
  {
    if(newuser->nickname != NULL)
      nickname_free(newuser->nickname);
    newuser->nickname = nick_p;
    identify_user(newuser);
    return pass_callback(ns_newuser_hook, newuser);
  }

  snprintf(userhost, USERHOSTLEN, "%s@%s", newuser->username, newuser->host);
  if(nickname_accesslist_check(nick_p, userhost))
  {
    ilog(L_DEBUG, "new user: %s(found access entry)", newuser->name);
    SetOnAccess(newuser);
    if(!nickname_get_secure(nick_p))
    {
      if(newuser->nickname != NULL)
        nickname_free(newuser->nickname);
      newuser->nickname = nick_p;
      identify_user(newuser);
      reply_user(nickserv, nickserv, newuser, NS_IDENTIFY_ACCESS, newuser->name);
    }
  }
  else
  {
    if(nickname_get_enforce(nick_p))
    {
      reply_user(nickserv, nickserv, newuser, NS_NICK_IN_USE_IWILLCHANGE, newuser->name);
      newuser->enforce_time = CurrentTime + 30; /* XXX configurable? */
      dlinkAdd(newuser, make_dlink_node(), &nick_enforce_list);
    }
    else
    {
      reply_user(nickserv, nickserv, newuser, NS_NICK_IN_USE, newuser->name);
    }
    ilog(L_DEBUG, "new user:%s(no access entry)", newuser->name);
  }

  if(nick_p != newuser->nickname)
    nickname_free(nick_p);
  
  return pass_callback(ns_newuser_hook, newuser);
}

static void *
ns_on_quit(va_list args)
{
  struct Client *user     = va_arg(args, struct Client *);
  char          *comment  = va_arg(args, char *);
  Nickname *nick = user->nickname;

  if(IsServer(user))
    return pass_callback(ns_quit_hook, user, comment);

  if(nick)
  {
    const char *cloak = nickname_get_cloak(nick);

    nickname_set_last_quit(nick, comment);

    if(nickname_get_cloak_on(nick) == TRUE && !EmptyString(cloak))
      nickname_set_last_host(nick, cloak);
    else
      nickname_set_last_host(nick, user->host);

    nickname_set_last_realname(nick, user->info);
    nickname_set_last_quit_time(nick, CurrentTime);
    nickname_set_last_seen(nick, CurrentTime);
  }

  dlinkFindDelete(&nick_enforce_list, user);
  if(IsMe(user->from))
    dlinkFindDelete(&nick_release_list, user);

  return pass_callback(ns_quit_hook, user, comment);
}

static void *
ns_on_certfp(va_list args)
{
  struct Client *user = va_arg(args, struct Client *);
  Nickname *nick = nickname_find(user->name);
  struct AccessEntry *cert;
  char *newnick;

  if(nick == NULL)
    return pass_callback(ns_certfp_hook, user);

  if(user->nickname != NULL && nickname_get_nickid(user->nickname) == nickname_get_nickid(nick))
  {
    /* Already identified for this nick */
    nickname_free(nick);
    return pass_callback(ns_certfp_hook, user);
  }

  if(!nickname_cert_check(nick, user->certfp, &cert))
    return pass_callback(ns_certfp_hook, user);
  
  newnick = nickname_nick_from_id(cert->nickname_id, FALSE);

  if(newnick != NULL)
  {
    if(user->nickname != NULL)
      nickname_free(user->nickname);

    user->nickname = nick;
    SetSentCert(user);
    handle_nick_change(nickserv, user, newnick, NS_IDENTIFY_CERT);
  }
  else
  {
    if(user->nickname != NULL) 
      nickname_free(user->nickname); 
    
    user->nickname = nick; 
    dlinkFindDelete(&nick_enforce_list, user); 
    identify_user(user); 
    SetSentCert(user); 
    reply_user(nickserv, nickserv, user, NS_IDENTIFY_CERT, user->name); 
  }

  MyFree(cert);
  return pass_callback(ns_certfp_hook, user);
}

static void *
ns_on_auth_requested(va_list args)
{
  char *use_cert = va_arg(args, char *);
  char *user    = va_arg(args, char *);
  char *n       = va_arg(args, char *);
  char *certfp  = va_arg(args, char *);
  Nickname *nick, *nick2;

  nick = nickname_find(user);
  if(nick == NULL)
  {
    send_auth_reply(nickserv, user, n, 0, "User not found");
  }
  else
  {
    nick2 = nickname_find(n);
    if(nick2 == NULL)
    {
      nickname_free(nick);
      send_auth_reply(nickserv, user, n, 0, "Nickname not found");
      return pass_callback(ns_on_auth_req_hook, user, n, certfp);
    }
    if(nickname_get_id(nick) != nickname_get_id(nick2))
    {
      nickname_free(nick);
      nickname_free(nick2);
      send_auth_reply(nickserv, user, n, 0, "Nickname not on account");
      return pass_callback(ns_on_auth_req_hook, user, n, certfp);
    }
    else
    {
      if(*use_cert == '1')
      {
        if(!nickname_cert_check(nick, certfp, NULL))
          send_auth_reply(nickserv, user, n, 0, "Authentication Failed");
        else
          send_auth_reply(nickserv, user, n, 1, "Success");
      }
      else
      {
        if(!check_nick_pass(NULL, nick, certfp))
          send_auth_reply(nickserv, user, n, 0, "Authentication Failed");
        else
          send_auth_reply(nickserv, user, n, 1, "Success");
      }
      nickname_free(nick);
    }
  }
  return pass_callback(ns_on_auth_req_hook, user);
}

static void *
ns_on_identify(va_list args)
{
  struct Client *uplink = va_arg(args, struct Client *);
  struct Client *client = va_arg(args, struct Client *);
  dlink_list ajoin_list = { 0 };
  dlink_node *ptr;

  if(client->nickname == NULL)
    return pass_callback(ns_on_identify_hook, uplink, client);

  if(nickname_ajoin_list(nickname_get_id(client->nickname), &ajoin_list) == -1)
    return pass_callback(ns_on_identify_hook, uplink, client);

  DLINK_FOREACH(ptr, ajoin_list.head)
  {
    char *channel = (char *)ptr->data;

    send_autojoin(nickserv, client, channel);
  }

  nickname_ajoinlist_free(&ajoin_list);

  return pass_callback(ns_on_identify_hook, uplink, client);
}

static void
m_status(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Client *target;
  char *name = parv[1];         //already checked # params

  if((target = find_client(name)) != NULL)
  {
    if(IsSentCert(target))
    {
      reply_user(service, service, client, NS_STATUS_SSL, name);
      return;  
    }
    else if(IsOnAccess(target))
    {
      reply_user(service, service, client, NS_STATUS_ACCESS, name);
      return;  
    }
    else if(IsIdentified(target))
    {
      reply_user(service, service, client, NS_STATUS_PASS, name);
      return;  
    }
    else
    {
      reply_user(service, service, client, NS_STATUS_NOTREG, name);
      return;  
    }
  }
  else
  {
    reply_user(service, service, client, NS_STATUS_OFFLINE, name);
    return;  
  }
}

static void
m_checkverify(struct Service *service, struct Client *client, int parc, char *parv[])
{
  Nickname *nick;

  if(!IsIdentified(client))
  {
    reply_user(service, service, client, NS_CHECKVERIFY_FAIL);
    return;
  }
  
  if(nickname_get_verified(client->nickname))
  {
    reply_user(service, service, client, NS_CHECKVERIFY_ALREADY);
    return;
  }
  
  /* Refresh from the DB */
  nick = nickname_find(client->name);
  if(!nick)
  {
    /* Handle disappearing nicknames sanely */
    ClearIdentified(client);
    nickname_free(client->nickname);
    client->nickname = NULL;
    reply_user(service, service, client, NS_CHECKVERIFY_FAIL);
    return;
  }
  nickname_free(client->nickname);
  client->nickname = nick;
  
  if(nickname_get_verified(client->nickname))
  {
    send_umode(NULL, client, "+R");
    do_cloak(client);
    reply_user(service, service, client, NS_CHECKVERIFY_SUCCESS);
    return;
  }
  else
  {
    reply_user(service, service, client, NS_CHECKVERIFY_FAIL);
    return;
  }
}

static int
m_set_flag(struct Service *service, struct Client *client,
           char *toggle, char *flagname,
           unsigned char (*get_func)(Nickname *),
           int (*set_func)(Nickname *, unsigned char))
{
  Nickname *nick = client->nickname;
  int on = FALSE;

  if(toggle == NULL)
  {
    on = get_func(nick);
    reply_user(service, service, client, NS_SET_VALUE, flagname,
      on ? "ON" : "OFF");
    return TRUE;
  }

  if(strncasecmp(toggle, "ON", 2) == 0)
    on = TRUE;
  else if (strncasecmp(toggle, "OFF", 3) == 0)
    on = FALSE;
  else
  {
    reply_user(service, service, client, NS_SET_VALUE, flagname,
      on ? "ON" : "OFF");
    return TRUE;
  }

  if(set_func(nick, on))
    reply_user(service, service, client, NS_SET_SUCCESS, flagname,
      on ? "ON" : "OFF");
  else
    reply_user(service, service, client, NS_SET_FAILED, flagname, 
        on ? "ON" : "OFF");

  return TRUE;
}

static int
m_set_string(struct Service * service, struct Client *client,
             const char *field, const char *value, int parc,
             const char *(*get_func)(Nickname *),
             int(*set_func)(Nickname *, const char*))
{
  Nickname *nick = client->nickname;
  int ret = FALSE;

  if(parc == 0)
  {
    const char *resp = get_func(nick);
    reply_user(service, service, client, NS_SET_VALUE, field,
      resp == NULL ? "Not Set" : resp);
    return TRUE;
  }

  if(irccmp(value, "-") == 0)
    value = NULL;

  ret = set_func(nick, value);

  reply_user(service, service, client, ret ? NS_SET_SUCCESS : NS_SET_FAILED, field,
      value == NULL ? "Not Set" : value);

  return ret;
}

static int
set_nickname_password(Nickname *nick, const char *new_password)
{
  char salt[SALTLEN+1];
  char *password, *pass;

  make_random_string(salt, sizeof(salt));
  password = MyMalloc(strlen(new_password) + SALTLEN + 1);
  snprintf(password, strlen(new_password) + SALTLEN + 1, "%s%s", new_password, 
        salt);

  pass = crypt_pass(password, TRUE);
  MyFree(password);
  if(!nickname_set_pass(nick, pass))
    return FALSE;
  if(!nickname_set_salt(nick, salt))
    return FALSE;
  MyFree(pass);
 
  return TRUE;
}
