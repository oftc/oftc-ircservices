/*
 *  servicesinfo.c: Defines the servicesinfo{} block of services.conf.
 *
 *  Copyright (C) 2005 by the Hybrid Development Team.
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

#include <string>
#include <vector>
#include <tr1/unordered_map>
#include <netdb.h>

#include "stdinc.h"
#include "language.h"
#include "interface.h"
#include "connection.h"
#include "client.h"
#include "parse.h"

#include "conf/conf.h"

struct ServicesInfoConf ServicesInfo = {};
char new_uid[TOTALSIDUID + 1] = {0};

/*
 * reset_servicesinfo()
 *
 * Sets up default values before a rehash.
 *
 * inputs: none
 * output: none
 */
static void 
reset_servicesinfo()
{
  memset(&ServicesInfo.vhost, 0, sizeof(ServicesInfo.vhost));
#ifdef IPV6
  memset(&ServicesInfo.vhost6, 0, sizeof(ServicesInfo.vhost6));
#endif
}

/*
 * verify_servicesinfo()
 *
 * Checks if required settings are defined.
 *
 * inputs: none
 * output: none
 */
static void
verify_servicesinfo()
{
  if(me->name() == "")
    parse_fatal("name= field missing in servicesinfo{} section");

  if(me->gecos() == "")
    parse_fatal("description= field missing in servicesinfo{} section");

  if(conf_cold && me->id() != "")
  {
    //
  }

  recalc_fdlimit(NULL);
}

static void
si_set_name(void *value, void *unused)
{
  char *name = (char *) value;

  if (strlen(name) > HOSTLEN)
  {
    if (conf_cold)
      parse_fatal("server name too long (max %d)", HOSTLEN);
    else
      parse_error("server name too long (max %d)", HOSTLEN);
  }
  else if (conf_cold)
    me->set_name(name);
  else if (strcmp(me->name().c_str(), name) != 0)
    parse_error("cannot change server name on rehash");
}

static void
si_set_sid(void *value, void *unused)
{
  char *sid = (char *) value;

  /* XXX */
  if (!IsDigit(sid[0]) || !IsAlNum(sid[1]) || !IsAlNum(sid[2]) || sid[3] != 0)
    parse_error("invalid SID, must match [0-9][0-9A-Z][0-9A-Z]");
  else if (conf_cold)
    me->set_id(sid);
}

static void
si_set_description(void *value, void *unused)
{
  me->set_gecos((char*)value);
}

static void
si_set_vhost(void *value, void *where)
{
  char *addrstr = (char *) value;
  struct irc_ssaddr *dest = (struct irc_ssaddr *) where;
  struct addrinfo hints, *res;

  if (*addrstr == '*')
    return;

  memset(&hints, 0, sizeof(hints));

#ifdef IPV6
  hints.ai_family   = (where == ServicesInfo.vhost6 ? AF_INET6 : AF_INET);
#else
  hints.ai_family   = AF_INET;
#endif
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE | AI_NUMERICHOST;

  if (irc_getaddrinfo(addrstr, NULL, &hints, &res))
    parse_error("invalid netmask");
  else
  {
    memcpy(dest, res->ai_addr, res->ai_addrlen);
    dest->ss.ss_family = res->ai_family;
    dest->ss_len = res->ai_addrlen;
    irc_freeaddrinfo(res);
  }
}

static void
si_set_rsa_private_key(void *value, void *unused)
{
}

static void
si_set_ssl_certificate(void *value, void *unused)
{
}

/*
 * init_servicesinfo()
 *
 * Defines the serverhide{} conf section.
 *
 * inputs: none
 * output: none
 */
void
init_servicesinfo(void)
{
  struct ConfSection *s = add_conf_section("servicesinfo", 2);

//  hreset = install_hook(reset_conf, reset_servicesinfo);
//  hverify = install_hook(verify_conf, verify_servicesinfo);

  add_conf_field(s, "name", CT_STRING, si_set_name, NULL);
  add_conf_field(s, "sid", CT_STRING, si_set_sid, NULL);
  add_conf_field(s, "description", CT_STRING, si_set_description, NULL);
  add_conf_field(s, "vhost", CT_STRING, si_set_vhost, &ServicesInfo.vhost);
#ifdef IPV6
  add_conf_field(s, "vhost6", CT_STRING, si_set_vhost, &ServicesInfo.vhost6);
#endif
#ifdef HAVE_LIBCRYPTO
  add_conf_field(s, "rsa_private_key_file", CT_STRING, si_set_rsa_private_key,
    NULL);
  add_conf_field(s, "ssl_certificate_file", CT_STRING, si_set_ssl_certificate,
    NULL);
#endif
}
