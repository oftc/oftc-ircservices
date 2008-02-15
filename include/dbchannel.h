/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  dbchannel.h channel related header file(database side)
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

#ifndef INCLUDED_dbchannel_h
#define INCLUDED_dbchannel_h

struct RegChannel *dbchannel_find(const char *);
int dbchannel_delete(struct RegChannel *);
int dbchannel_forbid(const char *);
int dbchannel_delete_forbid(const char *);
int dbchannel_register(struct RegChannel *, struct Nick *);
int dbchannel_is_forbid(const char *);
int dbchannel_masters_list(unsigned int, dlink_list *);
void dbchannel_masters_list_free(dlink_list *);
int dbchannel_masters_count(unsigned int, int *);
int dbchannel_list_all(dlink_list *);
void dbchannel_list_all_free(dlink_list *);
int dbchannel_list_regular(dlink_list *);
void dbchannel_list_regular_free(dlink_list *);
int dbchannel_list_forbid(dlink_list *);
void dbchannel_list_forbid_free(dlink_list *);

#endif
