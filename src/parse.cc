/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  parse.c: The message parser
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  Some parts:
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
 *
 *  this->program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  this->program is distributed in the hope that it will be useful,
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
#include <tr1/unordered_map>

#include "stdinc.h"
#include "language.h"
#include "interface.h"
#include "connection.h"
#include "parse.h"
#include "client.h"

using std::string;
using std::vector;

vector<string> 
irc_string::split(const string& delim, size_type count)
{
  size_type pos, off, c;
  vector<string> strings;

  off = c = 0;

  pos = this->find(delim, 0);

  while(pos != npos)
  {
    strings.push_back(this->substr(off, pos - off));
    off += pos - off + delim.length();
    if(count != 0 && c++ >= count-1)
    {
      strings.push_back(this->substr(off));
      return strings;
    }
    pos = this->find(delim, off);
  }

  strings.push_back(this->substr(off));
  return strings;
}

Message::~Message()
{
}

void
Parser::parse_line(Connection *uplink, string const& line)
{
  string sender, command;
  irc_string s = line;
  vector<string> strings, args;
  Message *m;
  BaseClient *source;

  if(s[0] == ':')
  {
    s = s.substr(1);
    strings = s.split(" ", 1);
    sender = strings[0];
    s = strings[1];
    strings = s.split(" ", 1);

    source = Client::find(sender);

    if(source == NULL)
    {
      ilog(L_DEBUG, "Source not found! %s", sender.c_str());
      return;
    }
  }
  else
  {
    strings = s.split(" ", 1);
    source = uplink->serv();
  }

  command = strings[0];

  if(strings[1][0] == ':')
  {
    args.push_back(strings[1].substr(1));
  }
  else
  {
    bool done = false;

    while(!done)
    {
      s = strings[1];

      strings = s.split(" ", 1);
      args.push_back(strings[0]);
      if(strings.size() == 1)
        break;
      if(strings[1][0] == ':')
      {
        args.push_back(strings[1].substr(1));
        done = true;
      }
    }
  }

  if((m = message_map[command]) != NULL)
    m->handler(uplink->serv(), source, args);
  else
    ilog(L_DEBUG, "Unhandled Command: '%s'", line.c_str());    
}

void
Parser::add_message(Message *message)
{
  message_map[message->name()] = message;
};
