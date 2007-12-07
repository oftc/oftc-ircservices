/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  nickname.c - nickname related functions
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
 *  $Id: dbm.c 1260 2007-12-07 08:53:17Z swalsh $
 */

#include "stdinc.h"

int 
nickname_is_forbid(const char *nickname)
{
  char *nick;
  int error;

  nick = db_execute_scalar(GET_FORBID, 1, &error, nickname);
  if(error)
  {
    ilog(L_CRIT, "Database error trying to test forbidden nickname %s", nickname);
    return 0;
  }

  if(nick == NULL)
  {
    MyFree(nick);
    return 0;
  }

  MyFree(nick);
  return 1;
}
