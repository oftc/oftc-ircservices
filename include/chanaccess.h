/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  chanaccess.h chanaccess related header file
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
 *  $Id: msg.h 957 2007-05-07 16:52:26Z stu $
 */

#ifndef INCLUDED_chanaccess_h
#define INCLUDED_chanaccess_h

int chanaccess_add(struct ChanAccess *);
int chanaccess_list(unsigned int, dlink_list *);
void chanaccess_list_free(dlink_list *);
struct ChanAccess * chanaccess_find(unsigned int, unsigned int);
struct ChanAccess * chanaccess_find_exact(unsigned int, unsigned int, unsigned int);
int chanaccess_remove(struct ChanAccess *);
int chanaccess_count(unsigned int);

#endif
