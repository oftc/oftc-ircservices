/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  nickname.h nickname related header file
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
 *  $Id: msg.h 957 2007-05-07 16:52:26Z stu $
 */

#ifndef INCLUDED_nickname_h
#define INCLUDED_nickname_h

typedef struct
{
  dlink_node node;

  unsigned int id;
  unsigned int nickid;
  unsigned int pri_nickid;
  char nick[NICKLEN+1];
  char pass[PASSLEN+1];
  char salt[SALTLEN+1];
  char cloak[HOSTLEN+1];
  char *email;
  char *url;
  char *last_realname;
  char *last_host;
  char *last_quit;
  unsigned int status;
  unsigned int language;
  unsigned char enforce;
  unsigned char secure;
  unsigned char verified;
  unsigned char cloak_on;
  unsigned char admin;
  unsigned char email_verified;
  unsigned char priv;
  time_t reg_time;
  time_t last_seen;
  time_t last_quit_time;
  time_t nick_reg_time;
} Nickname;

struct AccessEntry
{
  unsigned int id;
  char *value;
  unsigned int nickname_id;
};

Nickname* nickname_find(const char *);
int nickname_register(Nickname *);
int nickname_delete(Nickname *);

int nickname_forbid(const char *);
int nickname_delete_forbid(const char *);
int nickname_is_forbid(const char *);


int nickname_link(Nickname *, Nickname *);
int nickname_unlink(Nickname *);
int nickname_link_count(Nickname *);

char *nickname_nick_from_id(int, int);
int nickname_id_from_nick(const char *, int);
int nickname_set_master(Nickname *, const char *);

int nickname_save(Nickname *);

int nickname_accesslist_add(struct AccessEntry *);
int nickname_accesslist_list(Nickname *, dlink_list *);
int nickname_accesslist_check(Nickname *, const char *);
int nickname_accesslist_delete(Nickname *, const char *);
void nickname_accesslist_free(dlink_list *);

int nickname_cert_add(struct AccessEntry *);
int nickname_cert_list(Nickname *, dlink_list *);
int nickname_cert_check(Nickname *, const char *, struct AccessEntry **);
int nickname_cert_delete(Nickname *, const char *);
void nickname_certlist_free(dlink_list *);

int nickname_link_list(unsigned int, dlink_list *);
void nickname_link_list_free(dlink_list *);

int nickname_chan_list(unsigned int, dlink_list *);
void nickname_chan_list_free(dlink_list *);

int nickname_list_all(dlink_list *);
void nickname_list_all_free(dlink_list *);

int nickname_list_regular(dlink_list *);
void nickname_list_regular_free(dlink_list *);

int nickname_list_forbid(dlink_list *);
void nickname_list_forbid_free(dlink_list *);

int nickname_list_admins(dlink_list *);
void nickname_list_admins_free(dlink_list *);

Nickname *nickname_new();
inline void nickname_free(Nickname *);

unsigned int nickname_reset_pass(Nickname *, char **);

/* Nickname getters */
dlink_node nickname_get_node(Nickname *);

unsigned int nickname_get_id(Nickname *);
unsigned int nickname_get_nickid(Nickname *);
unsigned int nickname_get_pri_nickid(Nickname *);
inline const char *nickname_get_nick(Nickname *);
const char *nickname_get_pass(Nickname *);
const char *nickname_get_salt(Nickname *);
const char *nickname_get_cloak(Nickname *);
const char *nickname_get_email(Nickname *);
const char *nickname_get_url(Nickname *);
const char *nickname_get_last_realname(Nickname *);
const char *nickname_get_last_host(Nickname *);
const char *nickname_get_last_quit(Nickname *);
unsigned int nickname_get_status(Nickname *);
unsigned int nickname_get_language(Nickname *);
unsigned char nickname_get_enforce(Nickname *);
unsigned char nickname_get_secure(Nickname *);
unsigned char nickname_get_verified(Nickname *);
unsigned char nickname_get_cloak_on(Nickname *);
unsigned char nickname_get_admin(Nickname *);
unsigned char nickname_get_email_verified(Nickname *);
unsigned char nickname_get_priv(Nickname *);
time_t nickname_get_reg_time(Nickname *);
time_t nickname_get_last_seen(Nickname *);
time_t nickname_get_last_quit_time(Nickname *);

/* Nickname setters */
inline int nickname_set_id(Nickname *, unsigned int);
inline int nickname_set_nickid(Nickname *, unsigned int);
inline int nickname_set_pri_nickid(Nickname *, unsigned int);
inline int nickname_set_nick(Nickname *, const char *);
inline int nickname_set_pass(Nickname *, const char *);
inline int nickname_set_salt(Nickname *, const char *);
inline int nickname_set_cloak(Nickname *, const char *);
inline int nickname_set_email(Nickname *, const char *);
inline int nickname_set_url(Nickname *, const char *);
inline int nickname_set_last_realname(Nickname *, const char *);
inline int nickname_set_last_host(Nickname *, const char *);
inline int nickname_set_last_quit(Nickname *, const char *);
inline int nickname_set_status(Nickname *, unsigned int);
inline int nickname_set_language(Nickname *, unsigned int);
inline int nickname_set_enforce(Nickname *, unsigned char);
inline int nickname_set_secure(Nickname *, unsigned char);
inline int nickname_set_verified(Nickname *, unsigned char);
inline int nickname_set_cloak_on(Nickname *, unsigned char);
inline int nickname_set_admin(Nickname *, unsigned char);
inline int nickname_set_email_verified(Nickname *, unsigned char);
inline int nickname_set_priv(Nickname *, unsigned char);
inline int nickname_set_reg_time(Nickname *, time_t);
inline int nickname_set_last_seen(Nickname *, time_t);
inline int nickname_set_last_quit_time(Nickname *, time_t);
#endif
