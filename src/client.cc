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

#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <tr1/unordered_map>

#include "stdinc.h"
#include "language.h"
#include "interface.h"
#include "connection.h"
#include "client.h"

using std::string;
using std::stringstream;
using std::vector;
using std::tr1::unordered_map;
using std::runtime_error;

vector<BaseClient *> GlobalClientList;
unordered_map<string, BaseClient *> GlobalClientHash;
unordered_map<string, BaseClient *> GlobalIDHash;

BaseClient::~BaseClient()
{

}

void
BaseClient::init() 
{
  GlobalClientList.push_back(this);
  GlobalClientHash[_name] = this;
  if(_id != "")
    GlobalIDHash[_id] = this;
}

Client::~Client()
{
}

Server::~Server()
{
}

const string
Client::nuh() const
{
  stringstream ss;

  ss << _name << "!" << _username << "@" << _host;
  
  return ss.str();
}

void
Client::init() 
{
  if(_name.length() == 0)
    throw runtime_error("No name specified");
  if(_username.length() == 0)
    throw runtime_error("No username specified");
  if(_host.length() == 0)
    throw runtime_error("No hostname specified");

  _ts = CurrentTime;
  BaseClient::init();

 ilog(L_DEBUG, "Adding C++ client %s", nuh().c_str());
}

void
Server::init() 
{
  BaseClient::init();
  ilog(L_DEBUG, "Adding C++ server %s", _name.c_str());
}

const string&
UID::generate()
{
  add_one_to_uid(TOTALSIDUID - 1);
  return current_uid();
}

void
UID::add_one_to_uid(int i)
{
  if (i != IRC_MAXSID)    /* Not reached server SID portion yet? */
  {
    if (current_uid()[i] == 'Z')
      current_uid()[i] = '0';
    else if (current_uid()[i] == '9')
    {
      current_uid()[i] = 'A';
      add_one_to_uid(i-1);
    }
    else current_uid()[i] = current_uid()[i] + 1;
  }
  else
  {
    /* NOTE: if IRC_MAXUID != 6, this will have to be rewritten */
    if (current_uid()[i] == 'Z')
      current_uid().insert(IRC_MAXSID, "AAAAAA");
    else
      current_uid()[i] = current_uid()[i] + 1;
  }
}
