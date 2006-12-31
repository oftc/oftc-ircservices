/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  interface.c: Functions for interfacing with service modules
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
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
#include <string>
#include <stdexcept>
#include <vector>

using std::string;
using std::runtime_error;
using std::vector;

struct LanguageFile ServicesLanguages[LANG_LAST];
struct ModeList *ServerModeList;

vector<Service *> service_list;

void
init_interface()
{
  load_language(ServicesLanguages, "services.en");
}

void
Service::introduce()
{
  if(_name.length() == 0)
    throw runtime_error("Need a service name");

  _client = new Client(_name, "services", _name, me->name());
  _client->init();
}
