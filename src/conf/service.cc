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
 *  $Id: /local/oftc-ircservices/branches/ootest/src/conf/connect.cc 1630 2006-12-28T00:33:26.643840Z stu  $
 */

#include "stdinc.h"
#include "conf/conf.h"

#include <vector>

using std::vector;

vector<ServiceConf *> ServiceConfs;
ServiceConf tmpservice = { 0 };
static dlink_node *hreset;

/*
 * reset_service()
 *
 * Sets up default values before a rehash.
 *
 * inputs: none
 * output: none
 */
static void *
reset_service(va_list args)
{
  return pass_callback(hreset);
}

static void
before_service()
{
  MyFree(tmpservice.name);
  MyFree(tmpservice.module);

  memset(&tmpservice, 0, sizeof(tmpservice));
}

static void
after_service()
{
  ServiceConf *service;
  if(tmpservice.name == NULL)
    parse_fatal("name= field missing in service{} section");

  if(tmpservice.module == NULL)
    parse_fatal("module= field missing in service{} section");

  service = (ServiceConf *)MyMalloc(sizeof(ServiceConf));
  memcpy(service, &tmpservice, sizeof(ServiceConf));
  ServiceConfs.push_back(service);
}

/*
 * init_service()
 *
 * Defines the serverhide{} conf section.
 *
 * inputs: none
 * output: none
 */
void
init_service(void)
{
  struct ConfSection *s = add_conf_section("service", 2);

  hreset = install_hook(reset_conf, reset_service);
  s->before = before_service;

  add_conf_field(s, "name", CT_STRING, NULL, &tmpservice.name);
  add_conf_field(s, "module", CT_STRING, NULL, &tmpservice.module);

  s->after = after_service;
}
