/*
 *  mail.c: Defines the mail{} block of services.conf.
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
 *  $Id: mail.c 372 2006-12-04 10:07:48Z stu $
 */

#include "stdinc.h"
#include "conf/conf.h"

struct MailConf Mail = {0};

static dlink_node *hreset, *hverify;

/*
 * reset_mail()
 *
 * Sets up default values before a rehash.
 *
 * inputs: none
 * output: none
 */
static void *
reset_mail(va_list args)
{
  return pass_callback(hreset);
}

/*
 * verify_mail()
 *
 * Checks if required settings are defined.
 *
 * inputs: none
 * output: none
 */
static void *
verify_mail(va_list args)
{
  if (EmptyString(Mail.command))
    parse_fatal("command= field missing in mail{} section");

  if (EmptyString(Mail.from_address))
    parse_fatal("from_address= field missing in mail{} section");

  return pass_callback(hverify);
}

/*
 * init_mail()
 *
 * Defines the mail{} conf section.
 *
 * inputs: none
 * output: none
 */
void
init_mail(void)
{
  struct ConfSection *s = add_conf_section("mail", 2);

  hreset = install_hook(reset_conf, reset_mail);
  hverify = install_hook(verify_conf, verify_mail);

  add_conf_field(s, "command", CT_STRING, NULL, &Mail.command);
  add_conf_field(s, "from_address", CT_STRING, NULL, &Mail.from_address);
  add_conf_field(s, "expire_time", CT_TIME, NULL, &Mail.expire_time);
}

void
cleanup_mail()
{
  struct ConfSection *s = find_conf_section("mail");
  delete_conf_section(s);
  MyFree(s);
  MyFree(Mail.command);
  MyFree(Mail.from_address);
}
