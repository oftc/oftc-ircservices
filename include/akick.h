/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  akick.h akick related header file
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

#ifndef INCLUDED_akick_h
#define INCLUDED_akick_h

int akick_add(struct ServiceMask *);
int akick_check_client(struct Service *, struct Channel *, struct Client *);
int akick_enforce(struct Service *, struct Channel *, struct ServiceMask *);
int akick_list(unsigned int, dlink_list *);
void akick_list_free(dlink_list *);
int akick_remove_mask(unsigned int, const char *);
int akick_remove_account(unsigned int, const char *);

#endif
