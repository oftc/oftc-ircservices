/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  modules.c: Defines the modules{} block of ircd.conf.
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
#include "language.h"
#include "interface.h"
#include "connection.h"
#include "client.h"
#include "parse.h"

#include "conf/conf.h"
#include "lua_module.h"
#include "ruby_module.h"
#include <ltdl.h>

vector<string> mod_paths;
vector<string> mod_extra;

static dlink_node *hreset, *hpass;

/*
 * h_switch_conf_pass()
 *
 * Hook function called after switching to conf_pass.
 * (After pass 1 we boot all modules.)
 *
 * inputs: none
 * output: none
 */
static void *
h_switch_conf_pass(va_list args)
{
  if (conf_pass == 2)
    boot_modules(conf_cold);

  return pass_callback(hpass);
}

/*
 * h_reset_conf()
 *
 * Hook function called before parsing the conf file.
 * Clears out old paths/extra modules.
 *
 * inputs: none
 * output: none
 */
static void *
h_reset_conf(va_list args)
{
  mod_paths.clear();
  mod_extra.clear();

  return pass_callback(hreset);
}

/*
 * mod_add_path()
 *
 * Deals with path="..." conf entry.
 *
 * inputs: value text
 * output: none
 */
static void
mod_add_path(void *value, void *unused)
{
  if (!chdir((char *) value))
  {
    mod_paths.push_back((char*)value);
    chdir(DPATH);
  }
  else
    parse_error("directory not found");
}

/*
 * mod_add_module()
 *
 * Deals with module="..." conf entry.
 *
 * inputs: value text
 * output: none
 */
static void
mod_add_module(void *value, void *unused)
{
  mod_extra.push_back((char*)value);
}

/*
 * init_modules()
 *
 * Defines the modules{} conf section.
 *
 * inputs: none
 * output: none
 */
void
init_modules()
{
  struct ConfSection *s = add_conf_section("modules", 1);

  hreset = install_hook(reset_conf, h_reset_conf);
  hpass = install_hook(switch_conf_pass, h_switch_conf_pass);

  add_conf_field(s, "path", CT_STRING, mod_add_path, NULL);
  add_conf_field(s, "module", CT_STRING, mod_add_module, NULL);
}
