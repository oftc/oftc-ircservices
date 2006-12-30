/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  client.c: Client functions
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  Some parts:
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
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
#include <vector>
#include <string>
#include <stdexcept>
#include <tr1/unordered_map>

using std::string;
using std::vector;
using std::tr1::unordered_map;

vector<Client *> GlobalClientList;
unordered_map<string, Client *> GlobalClientHash;

Client::Client()
{
}

Client::Client(std::string const& n)
{
  name = n;
}

Client::Client(std::string const& nickname, std::string const& username, 
  std::string const& infostr, std::string const &hostname)
{
  name = nickname.substr(0, NICKLEN);
  user = username.substr(0, USERLEN);
  info = infostr.substr(0, REALLEN);
  host = hostname.substr(0, HOSTLEN);
}

std::string
Client::nuh()
{
  std::string str;

  str = name;
  str.append("!");
  str.append(user);
  str.append("@");
  str.append(host);
  return str;
}

void
Client::introduce()
{
  if(name.length() == 0)
    throw std::runtime_error("No name specified");
  if(user.length() == 0)
    throw std::runtime_error("No username specified");
  if(host.length() == 0)
    throw std::runtime_error("No hostname specified");

  tsinfo = CurrentTime;
  GlobalClientList.push_back(this);
  GlobalClientHash[name.c_str()] = this;
  ilog(L_DEBUG, "Adding C++ client %s", this->nuh().c_str());
  //execute_callback(on_newuser_cb, source_p);
}
