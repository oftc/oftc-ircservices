/* TODO: add copyright block */

#ifndef INCLUDED_nickserv_h
#define INCLUDED_nickserv_h
#include "nickserv-lang.h"

struct Nick
{
  dlink_node node;
  
  unsigned int id;
  unsigned int nickid;
  unsigned int pri_nickid;
  char nick[NICKLEN+1];
  char real_nick[NICKLEN+1];
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
};

#endif /* INCLUDED_nickserv_h */
