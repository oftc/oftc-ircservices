/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  dynlink.h: Low level interface for module support.
 *
 *  Copyright (C) 2002-2006 by the past and present ircd coders, and others.
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

#if defined(HAVE_DLOPEN) || defined(_WIN32) || defined(HAVE_MACH_O_DYLD_H)

#define SHARED_MODULES

// Loads a module given by name. void ** receives module base address
// (best we can determine). Return value: module handle or NULL.

void *modload(const char *, void **);

// Returns symbol address from a loaded module given by handle.

void *modsym(void *, const char *);

// Unloads a module specified by handle.

void modunload(void *);

// moderror can be called after modload if it failed to get a detailed
// error description.

const char *moderror(void);

#endif
