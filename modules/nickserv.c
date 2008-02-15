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
#include "chanserv.h"
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
#include "nickname.h"
#include "nickserv.h"
#include "crypt.h"

static struct Service *nickserv = NULL;
static struct Client *nickserv_client = NULL;

static dlink_node *ns_umode_hook;
static dlink_node *ns_nick_hook;
static dlink_node *ns_newuser_hook;
static dlink_node *ns_quit_hook;
static dlink_node *ns_certfp_hook;

static dlink_list nick_enforce_list = { NULL, NULL, 0 };
static dlink_list nick_release_list = { NULL, NULL, 0 };

static int guest_number;

static void process_enforce_list(void *);
static void process_release_list(void *);

static void *ns_on_umode_change(va_list);
static void *ns_on_newuser(va_list);
static void *ns_on_nick_change(va_list);
static void *ns_on_quit(va_list);
static void *ns_on_certfp(va_list);
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

static void m_access_add(struct Service *, struct Client *, int, char *[]);
static void m_access_list(struct Service *, struct Client *, int, char *[]);
static void m_access_del(struct Service *, struct Client *, int, char *[]);

static void m_cert_add(struct Service *, struct Client *, int, char *[]);
static void m_cert_list(struct Service *, struct Client *, int, char *[]);
static void m_cert_del(struct Service *, struct Client *, int, char *[]);

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

static struct ServiceMessage cert_sub[] = {
  { NULL, "ADD", 0, 0, 1, 0, IDENTIFIED_FLAG, NS_HELP_CERT_ADD_SHORT, 
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
  NULL, "UNFORBID", 0, 1, 1, 0, ADMIN_FLAG, NS_HELP_FORBID_SHORT, 
  NS_HELP_FORBID_LONG, m_unforbid
};

static struct ServiceMessage regain_msgtab = {
  NULL, "REGAIN", 0, 2, 2, 0, USER_FLAG, NS_HELP_REGAIN_SHORT, 
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
  mod_add_servcmd(&nickserv->msg_tree, &ghost_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &link_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &unlink_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &info_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &forbid_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &unforbid_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &regain_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &id_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &sudo_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &cloakstring_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &sendpass_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &list_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &status_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &enslave_msgtab);
  
  ns_umode_hook       = install_hook(on_umode_change_cb, ns_on_umode_change);
  ns_nick_hook        = install_hook(on_nick_change_cb, ns_on_nick_change);
  ns_newuser_hook     = install_hook(on_newuser_cb, ns_on_newuser);
  ns_quit_hook        = install_hook(on_quit_cb, ns_on_quit);
  ns_certfp_hook      = install_hook(on_certfp_cb, ns_on_certfp);
  
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
process_release_list(void *param)
{
  dlink_node *ptr, *ptr_next;
  
  DLINK_FOREACH_SAFE(ptr, ptr_next, nick_release_list.head)
  {
    struct Client *user = (struct Client *)ptr->data;  

    if(CurrentTime > user->release_time)
    {
      exit_client(user, &me, "Held nickname released");
      dlinkDelete(ptr, &nick_release_list);
      free_dlink_node(ptr);
      user->enforce_time = 0;
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
  struct Nick *nick;
  char *pass;
  char password[PASSLEN+SALTLEN+1];
 
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
    
  if((nick = nickname_find(client->name)) != NULL)
  {
    reply_user(service, service, client, NS_ALREADY_REG, client->name); 
    free_nick(nick);
    return;
  }

  nick = MyMalloc(sizeof(struct Nick));

  make_random_string(nick->salt, sizeof(nick->salt));
  snprintf(password, sizeof(password), "%s%s", parv[1], nick->salt);

  pass = crypt_pass(password, TRUE);
  strlcpy(nick->pass, pass, sizeof(nick->pass));
  strlcpy(nick->nick, client->name, sizeof(nick->nick));
  DupString(nick->email, parv[2]);
  MyFree(pass);

  if(nickname_register(nick))
  {
    client->nickname = nick;
    identify_user(client);

    reply_user(service, service, client, NS_REG_COMPLETE, client->name);
    global_notice(NULL, "%s!%s@%s registered nick %s\n", client->name, 
        client->username, client->host, nick->nick);

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
  struct Nick *nick = nickname_find(client->name);
  char *target_nick = NULL;

  assert(nick != NULL);

  /* This might be being executed via sudo, find the real user of the nick */
  if(client->nickname->id != nick->id)
  {
    target_nick = nickname_nick_from_id(client->nickname->nickid, FALSE);
    target = find_client(target_nick);
  }
  else
  {
    target = client;
    target_nick = client->name;
  }

  free_nick(nick);

  if(target == client)
  {
    /* A normal user dropping their own nick */
    if(parc == 0)
    {
      /* No auth code, so give them one and do nothing else*/
      char buf[IRC_BUFSIZE+1] = {0};
      char *hmac;

      snprintf(buf, IRC_BUFSIZE, "DROP %ld %d %s", CurrentTime, 
          target->nickname->nickid, target->name);
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
          target->nickname->nickid, target->name);
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

  if(nickname_delete(client->nickname)) 
  {
    if(target != NULL)
    {
      ClearIdentified(target);
      if(target->nickname != NULL)
        free_nick(target->nickname);
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
  struct Nick *nick;
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
    free_nick(nick);
    if(++client->num_badpass > 5)
    {
      kill_user(service, client, "Too many failed password attempts.");
      return;
    }
    reply_user(service, service, client, NS_IDENT_FAIL, name);
    return;
  }

  if(client->nickname != NULL)
    free_nick(client->nickname);

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
  struct Nick *nick;
  char *pass;
  char password[PASSLEN+SALTLEN+1];
  
  nick = client->nickname;

  snprintf(password, sizeof(password), "%s%s", parv[1], nick->salt);
  
  pass = crypt_pass(password, 1);
  /* XXX: what about the salt?  shouldn't we make a new one and store that too? -- weasel */
  if(db_set_string(SET_NICK_PASSWORD, client->nickname->id, pass))
  {
    reply_user(service, service, client, NS_SET_SUCCESS, "PASSWORD", "hidden");
  }
  else
  {
    reply_user(service, service, client, NS_SET_FAILED, "PASSWORD", "hidden");
    MyFree(pass);
    return;
  }
  strlcpy(nick->pass, pass, sizeof(nick->pass));
  MyFree(pass);
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
        service->languages[client->nickname->language].name,
        client->nickname->language);

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
    
    if(db_set_number(SET_NICK_LANGUAGE, client->nickname->id, lang))
    {
      client->nickname->language = lang;
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
  struct Nick *nick = client->nickname;
  
  if(parc == 0)
  {
    reply_user(service, service, client, NS_SET_VALUE, 
        "URL", (nick->url == NULL) ? "Not set" : nick->url);
    return;
  }
  
  if(irccmp(parv[1], "-") == 0)
    parv[1] = NULL;
  
  if(db_set_string(SET_NICK_URL, nick->id, parv[1]))
  {
    nick->url = replace_string(nick->url, 
        (parv[1] == NULL) ? "Not set" : parv[1]);
    reply_user(service, service, client, NS_SET_SUCCESS, "URL", nick->url);
  }
  else
    reply_user(service, service, client, NS_SET_FAILED, "URL", parv[1]);
}

static void
m_set_email(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  struct Nick *nick = client->nickname;
  
  if(parc == 0)
  {
    reply_user(service, service, client, NS_SET_VALUE, "EMAIL", nick->email);
    return;
  }
    
  if(db_set_string(SET_NICK_EMAIL, nick->id, parv[1]))
  {
    nick->email = replace_string(nick->email, parv[1]);
    reply_user(service, service, client, NS_SET_SUCCESS, "EMAIL", nick->email);
  }
  else
    reply_user(service, service, client, NS_SET_FAILED, "EMAIL", parv[1]);
}

static void
m_set_cloak(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Nick *nick = client->nickname;
  unsigned char flag;
  
  if(parc == 0)
  {
    reply_user(service, service, client, NS_SET_VALUE, "CLOAK", 
        nick->cloak_on ? "ON" : "OFF");
    return;
  }

  if(strcasecmp(parv[1], "ON") == 0)
    flag = 1;
  else if(strcasecmp(parv[1], "OFF") == 0)
    flag = 0;
  else
  {
    reply_user(service, service, client, NS_SET_VALUE, "CLOAK", 
        nick->cloak_on ? "ON" : "OFF");
    return;
  }

  if(db_set_bool(SET_NICK_CLOAKON, nick->id, flag))
  {
    nick->cloak_on = flag;
    reply_user(service, service, client, NS_SET_SUCCESS, "CLOAK", flag ? "ON" : "OFF");
  }
  else
    reply_user(service, service, client, NS_SET_FAILED, "CLOAK", parv[1]);
}

static void
m_cloakstring(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Nick *nick = nickname_find(parv[1]);

  if(nick == NULL)
  {
    reply_user(service, service, client, NS_REG_FIRST, parv[1]);
    return;
  }
 
  if(parc == 1)
  {
    char *reply;

    reply = (nick->cloak[0] == '\0') ? "Not set" : nick->cloak;
    reply_user(service, service, client, NS_SET_VALUE, "CLOAKSTRING", reply);
    free_nick(nick);
    return;
  }

  if(irccmp(parv[2], "-") == 0)
    parv[2] = NULL;
  else
  {
    if(!valid_hostname(parv[2]))
    {
      free_nick(nick);
      reply_user(service, service, client, NS_INVALID_CLOAK, parv[2]);
      return;
    }
  }
    
  if(db_set_string(SET_NICK_CLOAK, nick->id, parv[2]))
  {
    char *reply = parv[2] == NULL ? "Not set" : parv[2];

    if(parv[2] == NULL)
    {
      reply = "Not set";
      memset(nick->cloak, 0, sizeof(nick->cloak));
    }
    else
    {
      reply = parv[2];
      strlcpy(nick->cloak, parv[2], sizeof(nick->cloak));
    }
    reply_user(service, service, client, NS_SET_SUCCESS, "CLOAKSTRING", 
        reply);
    ilog(L_NOTICE, "%s set CLOAKSTRING of %s to %s", client->name, nick->nick,
        reply);
  }
  else
    reply_user(service, service, client, NS_SET_FAILED, "CLOAKSTRING", parv[2]);

  free_nick(nick);
}

static void
m_set_secure(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Nick *nick = client->nickname;
  unsigned char flag;
  
  if(parc == 0)
  {
    reply_user(service, service, client, NS_SET_VALUE, "SECURE", 
        nick->secure ? "ON" : "OFF");
    return;
  }

  if(strcasecmp(parv[1], "ON") == 0)
    flag = 1;
  else if(strcasecmp(parv[1], "OFF") == 0)
    flag = 0;
  else
  {
    reply_user(service, service, client, NS_SET_VALUE, "SECURE", 
        nick->secure ? "ON" : "OFF");
    return;
  }

  if(db_set_bool(SET_NICK_SECURE, nick->id, flag))
  {
    nick->secure = flag;
    reply_user(service, service, client, NS_SET_SUCCESS, "SECURE", flag ? "ON" : "OFF");
  }
  else
    reply_user(service, service, client, NS_SET_FAILED, "SECURE", parv[1]);
}

static void
m_set_enforce(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Nick *nick = client->nickname;
  unsigned char flag;
  
  if(parc == 0)
  {
    reply_user(service, service, client, NS_SET_VALUE, "ENFORCE", 
       nick->enforce ? "ON" : "OFF");
    return;
  }

  if(strcasecmp(parv[1], "ON") == 0)
    flag = 1;
  else if(strcasecmp(parv[1], "OFF") == 0)
    flag = 0;
  else
  {
    reply_user(service, service, client, NS_SET_VALUE, "ENFORCE", 
        nick->enforce ? "ON" : "OFF");
    return;
  }

  if(db_set_bool(SET_NICK_ENFORCE, nick->id, flag))
  {
    nick->enforce = flag;
    reply_user(service, service, client, NS_SET_SUCCESS, "ENFORCE", flag ? "ON" : "OFF");
  }
  else
    reply_user(service, service, client, NS_SET_FAILED, "ENFORCE", parv[1]);
}

static void
m_set_master(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Nick *nick = client->nickname;

  if(parc == 0)
  {
    char *prinick = nickname_nick_from_id(nick->id, TRUE);

    reply_user(service, service, client, NS_SET_VALUE, "MASTER", prinick);
    MyFree(prinick);
    return;
  }

  if(nickname_id_from_nick(parv[1], TRUE) != nick->id)
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
  struct Nick *nick = client->nickname;
  unsigned char flag;
  
  if(parc == 0)
  {
    reply_user(service, service, client, NS_SET_VALUE, "PRIVATE", 
       nick->priv ? "ON" : "OFF");
    return;
  }

  if(strcasecmp(parv[1], "ON") == 0)
    flag = 1;
  else if(strcasecmp(parv[1], "OFF") == 0)
    flag = 0;
  else
  {
    reply_user(service, service, client, NS_SET_VALUE, "PRIVATE", 
        nick->priv ? "ON" : "OFF");
    return;
  }

  if(db_set_bool(SET_NICK_PRIVATE, nick->id, flag))
  {
    nick->priv = flag;
    reply_user(service, service, client, NS_SET_SUCCESS, "PRIVATE", flag ? "ON" : "OFF");
  }
  else
    reply_user(service, service, client, NS_SET_FAILED, "PRIVATE", parv[1]);
}


static void
m_access_add(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Nick *nick = client->nickname;
  struct AccessEntry access;

  if(strchr(parv[1], '@') == NULL)
  {
    reply_user(service, service, client, NS_ACCESS_INVALID, parv[1]);
    return;
  }

  access.id = nick->id;
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
  struct Nick *nick;
  struct AccessEntry *entry = NULL;
  dlink_list list = { 0 };
  dlink_node *ptr;
  int i = 1;

  nick = client->nickname;

  reply_user(service, service, client, NS_ACCESS_START);

  nickname_accesslist_list(nick, &list);

  DLINK_FOREACH(ptr, list.head)
    reply_user(service, service, client, NS_ACCESS_ENTRY, i++, entry->value);

  nickname_accesslist_free(&list);
}

static void
m_access_del(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Nick *nick = client->nickname;
  int index, ret;

  index = atoi(parv[1]);
  ret = nickname_accesslist_delete(nick, parv[1], index);
  
  reply_user(service, service, client, NS_ACCESS_DEL, ret);
}

static void
m_cert_add(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Nick *nick = client->nickname;
  char *certfp;
  struct AccessEntry access = { 0 };

  if(parv[1] == NULL)
  {
    if(*client->certfp == '\0')
    {
      reply_user(service, service, client, NS_CERT_YOUHAVENONE);
      return;
    }
    certfp = client->certfp;
  }
  else
  {
    if(strlen(parv[1]) != 40)
    {
      reply_user(service, service, client, NS_CERT_INVALID, parv[1]);
      return;
    }
    certfp = parv[1];
  }

  if(nickname_cert_check(nick, certfp))
  {
    reply_user(service, service, client, NS_CERT_EXISTS, certfp);
    return;
  }
  access.id = nick->id;
  access.value = certfp;
  if(nickname_cert_add(&access))
    reply_user(service, service, client, NS_CERT_ADD, access.value);
  else
    reply_user(service, service, client, NS_CERT_ADDFAIL, access.value);
}

static void
m_cert_list(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Nick *nick;
  struct AccessEntry *entry = NULL;
  dlink_list list = { 0 };
  dlink_node *ptr;
  int i = 1;

  nick = client->nickname;

  nickname_cert_list(nick, &list);

  reply_user(service, service, client, NS_CERT_START);

  DLINK_FOREACH(ptr, list.head)
  {
    entry = (struct AccessEntry *)ptr->data;

    reply_user(service, service, client, NS_CERT_ENTRY, i++, entry->value);
    MyFree(entry->value);
    MyFree(entry);
  }
  
  nickname_certlist_free(&list);
}

static void
m_cert_del(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Nick *nick = client->nickname;
  int index, ret;

  index = atoi(parv[1]);
  ret = nickname_cert_delete(nick, parv[1], index);
  
  reply_user(service, service, client, NS_CERT_DEL, ret);
}

static void
m_regain(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Nick *nick;

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

  if(parc == 1 && (nick->secure || !IsOnAccess(client)))
  {
    free_nick(nick);
    reply_user(service, service, client, NS_REGAIN_FAILED_SECURITY, parv[1]);   
    return;
  }

  if((parc == 2 && !check_nick_pass(client, nick, parv[2])))
  {
    free_nick(nick);
    reply_user(service, service, client, NS_REGAIN_FAILED, parv[1]);   
    return;
  }
  
  if(client->nickname != NULL)
    free_nick(client->nickname);

  client->nickname = nick;

  handle_nick_change(service, client, parv[1], NS_REGAIN_SUCCESS);
}

static void
m_link(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Nick *nick, *master_nick;

  nick = client->nickname;
  if((master_nick = nickname_find(parv[1])) == NULL)
  {
    reply_user(service, service, client, NS_LINK_NOMASTER, parv[1]);
    return;
  }

  if(master_nick->id == nick->id)
  {
    free_nick(master_nick);
    reply_user(service, service, client, NS_LINK_NOSELF);
    return;
  }

  if(!check_nick_pass(client, master_nick, parv[2]))
  {
    free_nick(master_nick);
    reply_user(service, service, client, NS_LINK_BADPASS, parv[1]);
    return;
  }

  if(!nickname_link(master_nick, nick))
  {
    free_nick(master_nick);
    reply_user(service, service, client, NS_LINK_FAIL, parv[1]);
    return;
  }

  reply_user(service, service, client, NS_LINK_OK, nick->nick, parv[1]);
  
  free_nick(master_nick);
  free_nick(nick);

  client->nickname = nickname_find(parv[1]);
  assert(client->nickname != NULL);
}

static void
m_unlink(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Nick *nick = client->nickname;

  if((nick->id = nickname_unlink(nick)) > 0)
  {
    reply_user(service, service, client, NS_UNLINK_OK, client->name);
    // In case this was a slave nick, it is now a master of itself
    free_nick(client->nickname);
    client->nickname = nickname_find(client->name);
  }
  else
    reply_user(service, service, client, NS_UNLINK_FAILED, client->name);
}

static void
m_info(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Nick *nick;
  struct Client *target;
  struct InfoChanList *chan = NULL;
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
      nick->last_realname != NULL ? nick->last_realname : "Unknown");

  nickname_link_list(nick->id, &list);
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
    reply_time(service, client, NS_INFO_SEENTIME_FULL, nick->last_seen);

  reply_time(service, client, NS_INFO_REGTIME_FULL, nick->reg_time);
  reply_time(service, client, NS_INFO_QUITTIME_FULL, nick->last_quit_time);
 
  reply_user(service, service, client, NS_INFO, 
      (nick->last_quit == NULL) ? "Unknown" : nick->last_quit,  
      (nick->url == NULL) ? "Not set" : nick->url, 
      (nick->cloak[0] == '\0') ? "Not set" : nick->cloak);

  if((IsIdentified(client) && (client->nickname->id == nick->id)) || 
      client->access >= OPER_FLAG)
  {
    reply_user(service, service, client, NS_INFO_EMAIL, nick->email);
    reply_user(service, service, client, NS_INFO_LANGUAGE,
        service->languages[nick->language].name, nick->language); 

    reply_user(service, service, client, NS_INFO_OPTION, "ENFORCE", nick->enforce ? "ON" :
        "OFF");
    reply_user(service, service, client, NS_INFO_OPTION, "SECURE", nick->secure ? "ON" :
        "OFF");
    reply_user(service, service, client, NS_INFO_OPTION, "PRIVATE", nick->priv ? "ON" :
        "OFF");
    reply_user(service, service, client, NS_INFO_OPTION, "CLOAK", nick->cloak_on ? "ON" :
        "OFF");

    if(*buf != '\0')
      reply_user(service, service, client, NS_INFO_LINKS, buf);

    if(nick->nickid != nick->pri_nickid)
    {
      char *prinick = nickname_nick_from_id(nick->id, TRUE);

      reply_user(service, service, client, NS_INFO_MASTER, prinick);
      MyFree(prinick);
    }

    if(nickname_chan_list(nick->id, &list))
    {
      reply_user(service, service, client, NS_INFO_CHANS);
      DLINK_FOREACH(ptr, list.head)
      {
        chan = (struct InfoChanList *)ptr->data;
        reply_user(service, service, client, NS_INFO_CHAN, chan->channel,
            chan->level);
      }
      nickname_chan_list_free(&list);
    }
  }
  else if(!nick->priv)
    reply_user(service, service, client, NS_INFO_EMAIL, nick->email);
 
  if(nick != client->nickname)
    free_nick(nick);
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
  struct Nick *oldnick, *nick;
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
  if(nick->admin)
    client->access = ADMIN_FLAG;
  else
    client->access = IDENTIFIED_FLAG;

  newparv = MyMalloc(4 * sizeof(char*));

  newparv[0] = parv[0];
  newparv[1] = service->name;

  join_params(buf, parc-1, &parv[2]);

  DupString(newparv[2], buf);

  ilog(L_INFO, "%s executed %s SUDO on %s: %s", client->name, service->name, 
      nick->nick, newparv[2]);

  process_privmsg(1, me.uplink, client, 3, newparv);
  MyFree(newparv[2]);
  MyFree(newparv);

  free_nick(client->nickname);
  client->nickname = oldnick;
  client->access = oldaccess;
}

static void 
m_sendpass(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Nick *nick, *temp;
  char buf[IRC_BUFSIZE+1] = {0};
  char password[PASSLEN+1] = {0};
  char *hmac;
  char *auth;
  char *pass;
  int timestamp;

  if((nick = nickname_find(parv[1])) == NULL)
  {
    reply_user(service, service, client, NS_REG_FIRST, parv[1]);
    return;
  }

  if(parc == 1)
  {
    if(db_is_mailsent(nick->id, nick->email))
    {
      reply_user(service, service, client, NS_NO_SENDPASS_YET);
      free_nick(nick);
      return;      
    }

    temp = client->nickname;
    client->nickname = nick;

    snprintf(buf, IRC_BUFSIZE, "SENDPASS %ld %d %s", CurrentTime, 
        nick->id, nick->nick);
    hmac = generate_hmac(buf);

    reply_user(service, service, client, NS_SENDPASS_SENT);

    reply_mail(service, client, NS_SENDPASS_SUBJECT, NS_SENDPASS_BODY,
        nick->nick, client->name, client->username, client->host, nick->nick, 
        service->name, nick->nick, CurrentTime, hmac);

    client->nickname = temp;

    db_add_sentmail(nick->id, nick->email);
    free_nick(nick);
    
    MyFree(hmac);
    return;
  }
  else if(parc == 2)
  {
    reply_user(service, service, client, NS_SENDPASS_NEED_PASS);
    free_nick(nick);
    return;
  }
    
  if((auth = strchr(parv[2], ':')) == NULL)
  {
    reply_user(service, service, client, NS_SENDPASS_AUTH_FAIL, nick->nick);
    return;
  }

  *auth = '\0';
  auth++;

  snprintf(buf, IRC_BUFSIZE, "SENDPASS %s %d %s", parv[2], nick->id, 
      nick->nick);
  hmac = generate_hmac(buf);

  if(strncmp(hmac, auth, strlen(hmac)) != 0)
  {
    MyFree(hmac);
    reply_user(service, service, client, NS_SENDPASS_AUTH_FAIL, nick->nick);
    return;
  }

  MyFree(hmac);
  timestamp = atoi(parv[2]);
  if((CurrentTime - timestamp) > 86400)
  {
    reply_user(service, service, client, NS_SENDPASS_AUTH_FAIL, nick->nick);
    return;
  }

  snprintf(password, sizeof(password), "%s%s", parv[3], nick->salt);
  
  pass = crypt_pass(password, 1);
  /* XXX: what about the salt?  shouldn't we make a new one and store that too? -- weasel */
  if(db_set_string(SET_NICK_PASSWORD, nick->id, pass))
    reply_user(service, service, client, NS_SET_SUCCESS, "PASSWORD", "hidden");
  else
  {
    reply_user(service, service, client, NS_SET_FAILED, "PASSWORD", "hidden");
    MyFree(pass);
    return;
  }
  strlcpy(nick->pass, pass, sizeof(nick->pass));
  MyFree(pass);

  free_nick(nick);
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
  struct Nick *nick, *slave_nick;

  nick = client->nickname;
  if((slave_nick = nickname_find(parv[1])) == NULL)
  {
    reply_user(service, service, client, NS_LINK_NOSLAVE, parv[1]);
    return;
  }

  if(slave_nick->id == nick->id)
  {
    free_nick(slave_nick);
    reply_user(service, service, client, NS_LINK_NOSELF);
    return;
  }

  if(!check_nick_pass(client, slave_nick, parv[2]))
  {
    free_nick(slave_nick);
    reply_user(service, service, client, NS_LINK_BADPASS, parv[1]);
    return;
  }

  if(!nickname_link(nick, slave_nick))
  {
    free_nick(slave_nick);
    reply_user(service, service, client, NS_LINK_FAIL, parv[1]);
    return;
  }

  reply_user(service, service, client, NS_LINK_OK, parv[1], nick->nick);
  
  free_nick(slave_nick);
  free_nick(nick);

  client->nickname = nickname_find(parv[1]);
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
      free_nick(user->nickname);
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
  struct Nick *nick_p;
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
      if((target = find_client(client->nickname->nick)) != NULL)
      {
        if(target != client)
          kill_user(nickserv, target, "This nickname is registered and protected");
      }
      send_nick_change(nickserv, client, user->release_name);
      user->release_to = NULL;
      memset(user->release_name, 0, sizeof(user->release_name));
      db_set_string(SET_NICK_LAST_REALNAME, client->nickname->id, client->info);
      identify_user(client);
    }
    send_nick_change(nickserv, client, user->release_name);
    user->release_to = NULL;
    memset(user->release_name, 0, sizeof(user->release_name));
    identify_user(client);
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
      oldid = user->nickname->id;
      db_set_number(SET_NICK_LAST_SEEN, user->nickname->nickid, CurrentTime);
      free_nick(user->nickname);
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
      free_nick(user->nickname);
      user->nickname = NULL;
    }
    return pass_callback(ns_nick_hook, user, oldnick);
  }

  // Linked nick
  if(oldid == nick_p->id)
  {
    if(user->nickname != NULL)
      free_nick(user->nickname);
    user->nickname = nick_p;
    identify_user(user);
    return pass_callback(ns_nick_hook, user, oldnick);
  }
 
  snprintf(userhost, USERHOSTLEN, "%s@%s", user->username, user->host);

  if(*user->certfp != '\0' && nickname_cert_check(nick_p, user->certfp))
  {
    if(user->nickname != NULL)
      free_nick(user->nickname);
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
    if(!nick_p->secure)
    {
      if(user->nickname != NULL)
        free_nick(user->nickname);
      user->nickname = nick_p;
      identify_user(user);
      reply_user(nickserv, nickserv, user, NS_IDENTIFY_ACCESS, user->name);
    }
  }
  else
  {
    if(nick_p->enforce)
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
    free_nick(nick_p);
  
  return pass_callback(ns_nick_hook, user, oldnick);
}

static void *
ns_on_newuser(va_list args)
{
  struct Client *newuser = va_arg(args, struct Client*);
  struct Nick *nick_p;
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
      free_nick(newuser->nickname);
    newuser->nickname = nick_p;
    identify_user(newuser);
    return pass_callback(ns_newuser_hook, newuser);
  }

  snprintf(userhost, USERHOSTLEN, "%s@%s", newuser->username, newuser->host);
  if(nickname_accesslist_check(nick_p, userhost))
  {
    ilog(L_DEBUG, "new user: %s(found access entry)", newuser->name);
    SetOnAccess(newuser);
    if(!nick_p->secure)
    {
      if(newuser->nickname != NULL)
        free_nick(newuser->nickname);
      newuser->nickname = nick_p;
      identify_user(newuser);
      reply_user(nickserv, nickserv, newuser, NS_IDENTIFY_ACCESS, newuser->name);
    }
  }
  else
  {
    if(nick_p->enforce)
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
    free_nick(nick_p);
  
  return pass_callback(ns_newuser_hook, newuser);
}

static void *
ns_on_quit(va_list args)
{
  struct Client *user     = va_arg(args, struct Client *);
  char          *comment  = va_arg(args, char *);
  struct Nick *nick = user->nickname;

  if(IsServer(user))
    return pass_callback(ns_quit_hook, user, comment);

  if(nick)
  {
    db_set_string(SET_NICK_LAST_QUIT, nick->id, comment);
    db_set_string(SET_NICK_LAST_HOST, nick->id, user->host);
    db_set_string(SET_NICK_LAST_REALNAME, nick->id, user->info);
    db_set_number(SET_NICK_LAST_QUITTIME, nick->id, CurrentTime);
    db_set_number(SET_NICK_LAST_SEEN, nick->nickid, CurrentTime);
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
  struct Nick *nick = nickname_find(user->name);

  if(nick == NULL)
    return pass_callback(ns_certfp_hook, user);

  if(user->nickname != NULL && user->nickname->nickid == nick->nickid) 
  {
    /* Already identified for this nick */
    free_nick(nick);
    return pass_callback(ns_certfp_hook, user);
  }

  if(nickname_cert_check(nick, user->certfp))
  {
    if(user->nickname != NULL)
      free_nick(user->nickname);
    user->nickname = nick;
    dlinkFindDelete(&nick_enforce_list, user);
    identify_user(user);
    SetSentCert(user);
    reply_user(nickserv, nickserv, user, NS_IDENTIFY_CERT, user->name);
  }
  
  return pass_callback(ns_certfp_hook, user);
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
