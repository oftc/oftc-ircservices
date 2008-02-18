/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  m_error.c: Handles the ERROR function
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
#include "parse.h"
#include "language.h"
#include "dbm.h"
#include "nickname.h"
#include "interface.h"
#include "msg.h"
#include "client.h"

struct Message error_msgtab = {
 "ERROR", 0, 0, 1, 0, 0, 0,
  { ms_error, m_ignore }
};

void
ms_error(struct Client *client, struct Client *source, int parc, char *parv[])
{
  const char *para;

  para = (parc > 1 && *parv[1] != '\0') ? parv[1] : "<>";

  ilog(L_DEBUG, "Received ERROR message from %s: %s", source->name, para);

  if (client == source)
    ilog(L_DEBUG, "ERROR :from %s -- %s", client->name, para);
  else
    ilog(L_DEBUG, "ERROR :from %s via %s -- %s", source->name, client->name, para);
}

