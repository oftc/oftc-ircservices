/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  logging.c: Defines the logging{} block of services.conf.
 *  
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  Some parts:
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
 *  $Id: $
 */

#include "stdinc.h"
#include "conf/conf.h"
#include "dbm.h"
#include "parse.h"

struct LoggingConf Logging;

static dlink_node *hreset, *hverify;

/*
 * reset_logging()
 *
 * Sets up default values before a rehash.
 *
 * inputs: none
 * output: none
 */
static void *
reset_logging(va_list args)
{
  set_file_log_level(L_NOTICE);
  set_gnotice_log_level(L_NOTICE);

  memset(&Logging, 0, sizeof(Logging));
  Logging.use_logging = YES;

  return pass_callback(hreset);
}

/*
 * verify_logging()
 *
 * Reopens log files.
 *
 * inputs: none
 * output: none
 */
static void *
verify_logging(va_list args)
{
  if (Logging.use_logging)
    reopen_log(ServicesState.logfile);

  db_reopen_log();
  parse_reopen_log();

  return pass_callback(hverify);
}

static void
set_log_path(void *value, void *where)
{
  strlcpy((char *) where, (const char *) value, PATH_MAX + 1);
}

static void
conf_log_level(void *list, void *logtype)
{
  int i;
  char *value;
  struct { char *name; int level; } levels[7] = {
    {"L_CRIT", L_CRIT},
    {"L_ERROR", L_ERROR},
    {"L_WARN", L_WARN},
    {"L_NOTICE", L_NOTICE},
    {"L_TRACE", L_TRACE},
    {"L_INFO", L_INFO},
    {"L_DEBUG", L_DEBUG}
  };

  if (dlink_list_length((dlink_list *) list) != 1)
  {
    error:
    parse_error("invalid log level");
    return;
  }

  value = (char *) ((dlink_list *) list)->head->data;

  for (i = 0; i < 7; i++)
    if (!irccmp(value, levels[i].name))
    {
      if((long)logtype == 0)
        set_gnotice_log_level(levels[i].level);
      else
        set_file_log_level(levels[i].level);
      return;
    }

  goto error;
}

/*
 * init_logging()
 *
 * Defines the logging{} conf section.
 *
 * inputs: none
 * output: none
 */
void
init_logging(void)
{
  char *short_fields[] = { "fsqllog", "fserviceslog", "fdebuglog", "fparselog" };
  char *long_fields[] = { "fname_sqllog", "fname_serviceslog", "fname_debuglog",
    "fname_parselog"};
  char *paths[] = { Logging.sqllog, Logging.serviceslog, Logging.debuglog, Logging.parselog };
  int i;
  struct ConfSection *s = add_conf_section("logging", 2);
  
  hreset = install_hook(reset_conf, reset_logging);
  hverify = install_hook(verify_conf, verify_logging);

  add_conf_field(s, "use_logging", CT_BOOL, NULL, &Logging.use_logging);
  add_conf_field(s, "logpath", CT_STRING, NULL, NULL);

  for (i = 0; i < 4; i++)
  {
    add_conf_field(s, short_fields[i], CT_STRING, set_log_path, paths[i]);
    add_conf_field(s, long_fields[i], CT_STRING, set_log_path, paths[i]);
  }

  add_conf_field(s, "gnotice_log_level", CT_LIST, conf_log_level, (void*)0);
  add_conf_field(s, "file_log_level", CT_LIST, conf_log_level, (void*)1);
}

void
cleanup_logging()
{
  struct ConfSection *s = find_conf_section("logging");

  delete_conf_section(s);
  MyFree(s);
}
