/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  ircd_getopt.h: A header for the getopt() command line option calls.
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

#ifndef INCLUDED_libio_misc_ircd_getopt_h
#define INCLUDED_libio_misc_ircd_getopt_h
enum argtypes { INTEGER, YESNO, STRING, USAGE, ENDEBUG };
struct lgetopt {
  const char *opt; /* name of the argument */
  void *argloc; /* where we store the argument to it (-option argument) */
  argtypes argtype;
  const char *desc; /* description of the argument, usage for printing help */
};

LIBIO_EXTERN void parseargs(int *, char ***, struct lgetopt *);

#endif /* INCLUDED_libio_misc_ircd_getopt_h */
