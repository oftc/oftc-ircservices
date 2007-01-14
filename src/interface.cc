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

#include <string>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <tr1/unordered_map>

#include "stdinc.h"
#include "language.h"
#include "interface.h"
#include "connection.h"
#include "client.h"
#include "parse.h"

using std::string;
using std::runtime_error;
using std::vector;

struct LanguageFile ServicesLanguages[LANG_LAST];
struct ModeList *ServerModeList;

vector<Service *> service_list;
unordered_map<string, Service *> service_hash;

void
init_interface()
{
  load_language(ServicesLanguages, "services.en");
}

void
Service::init()
{
  if(_name.length() == 0)
    throw runtime_error("Need a service name");

  service_list.push_back(this);
  service_hash[_name] = this;

  _client = new Client(_name, "services", me->name(), me->gecos());
  _client->init();
  _client->set_id(UID::generate());
}

void
Service::add_message(ServiceMessage *message)
{
  message_map[message->name()] = message;
}

void
Service::handle_message(Connection *connection, Client *source, 
    string const& message)
{
  irc_string str = message;
  vector<string> strings;
  ServiceMessage *smsg;

  strings = str.split(" ", 1);

  if((smsg = message_map[strings[0]]) == NULL)
  {
    std::cout << "Unknown message: " << strings[0] << std::endl;
    return;
  }

  strings = str.split(" ");
    
  smsg->handler(source, strings);

  std::cout << "Service " << _name << " got message " << message << " from " << 
    source->name() << std::endl;
}
