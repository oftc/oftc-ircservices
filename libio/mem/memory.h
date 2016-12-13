/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  memory.h: A header for the memory functions.
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

#ifndef INCLUDED_libio_mem_memory_h
#define INCLUDED_libio_mem_memory_h
LIBIO_EXTERN void (* outofmemory) (void);

LIBIO_EXTERN void *MyMalloc(size_t);
LIBIO_EXTERN void *MyRealloc(void *, size_t);
LIBIO_EXTERN void MyFree(void *);
LIBIO_EXTERN void _DupString(char **, const char *);
LIBIO_EXTERN void mem_frob(void *, int);

#define DupString(x,y) _DupString(&(x), (y))

#endif /* INCLUDED_libio_mem_memory_h */
