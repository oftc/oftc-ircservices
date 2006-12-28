/*
 *  connect.c: Defines the connect{} block of services.conf.
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

#include "stdinc.h"
#include "conf/conf.h"

struct ConnectConf Connect = {0};

static dlink_node *hreset, *hverify;

/*
 * reset_connect()
 *
 * Sets up default values before a rehash.
 *
 * inputs: none
 * output: none
 */
static void *
reset_connect(va_list args)
{
  return pass_callback(hreset);
}

/*
 * verify_connect()
 *
 * Checks if required settings are defined.
 *
 * inputs: none
 * output: none
 */
static void *
verify_connect(va_list args)
{
  if (Connect.name == NULL)
    parse_fatal("name= field missing in connect{} section");

  if (Connect.host == NULL)
    parse_fatal("host= field missing in connect{} section");

  if(Connect.port == 0)
    parse_fatal("port= field missing in connect{} section");

  if(Connect.protocol == NULL)
    parse_fatal("protocol= field missing in connect{} section");

  if(Connect.password == NULL)
    parse_fatal("password= field missing in connect{} section");

  return pass_callback(hverify);
}

/*
 * init_connect()
 *
 * Defines the serverhide{} conf section.
 *
 * inputs: none
 * output: none
 */
void
init_connect(void)
{
  struct ConfSection *s = add_conf_section("connect", 2);

  hreset = install_hook(reset_conf, reset_connect);
  hverify = install_hook(verify_conf, verify_connect);

  add_conf_field(s, "host", CT_STRING, NULL, &Connect.host);
  add_conf_field(s, "name", CT_STRING, NULL, &Connect.name);
  add_conf_field(s, "port", CT_NUMBER, NULL, &Connect.port);
  add_conf_field(s, "protocol", CT_STRING, NULL, &Connect.protocol);
  add_conf_field(s, "password", CT_STRING, NULL, &Connect.password);
}
