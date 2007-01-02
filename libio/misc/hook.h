/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  hook.h: A header for the hooks into parts of ircd.
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

#ifndef INCLUDED_libio_misc_hook_h
#define INCLUDED_libio_misc_hook_h

#define HOOK_V2

typedef void *CBFUNC(va_list);

struct Callback
{
  char *name;
  dlink_list chain;
  dlink_node node;
  unsigned int called;
  time_t last;
};

LIBIO_EXTERN dlink_list callback_list;  /* listing/debugging purposes */

LIBIO_EXTERN struct Callback *register_callback(const char *, CBFUNC *);
LIBIO_EXTERN void *execute_callback(struct Callback *, ...);
LIBIO_EXTERN struct Callback *find_callback(const char *);
LIBIO_EXTERN dlink_node *install_hook(struct Callback *, CBFUNC *);
LIBIO_EXTERN void uninstall_hook(struct Callback *, CBFUNC *);
LIBIO_EXTERN void *pass_callback(dlink_node *, ...);

#define is_callback_present(c) (!!dlink_list_length(&c->chain))

#endif /* INCLUDED_libio_misc_hook_h */
