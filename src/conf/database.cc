/*
 *  database.c: Defines the database{} block of services.conf.
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

#include "stdinc.h"
#include "language.h"
#include "interface.h"
#include "connection.h"
#include "client.h"
#include "parse.h"

#include "conf/conf.h"

struct DatabaseConf Database = {0};

static dlink_node *hreset, *hverify;

/*
 * reset_database()
 *
 * Sets up default values before a rehash.
 *
 * inputs: none
 * output: none
 */
static void *
reset_database(va_list args)
{
  return pass_callback(hreset);
}

/*
 * verify_database()
 *
 * Checks if required settings are defined.
 *
 * inputs: none
 * output: none
 */
static void *
verify_database(va_list args)
{
  if (!Database.driver[0])
    parse_fatal("driver= field missing in database{} section");

  if (!Database.dbname[0])
    parse_fatal("dbname= field missing in database{} section");

  if(!Database.username[0])
    parse_fatal("username= field missing in database{} section");

  if(!Database.password[0])
    parse_fatal("password= field missing in database{} section");

  return pass_callback(hverify);
}

/*
 * init_database()
 *
 * Defines the serverhide{} conf section.
 *
 * inputs: none
 * output: none
 */
void
init_database(void)
{
  struct ConfSection *s = add_conf_section("database", 2);

  hreset = install_hook(reset_conf, reset_database);
  hverify = install_hook(verify_conf, verify_database);

  add_conf_field(s, "driver", CT_STRING, NULL, &Database.driver);
  add_conf_field(s, "dbname", CT_STRING, NULL, &Database.dbname);
  add_conf_field(s, "username", CT_STRING, NULL, &Database.username);
  add_conf_field(s, "password", CT_STRING, NULL, &Database.password);
  add_conf_field(s, "hostname", CT_STRING, NULL, &Database.hostname);
}
