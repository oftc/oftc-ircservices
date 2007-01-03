/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  irc_libio.h: libio interface specification.
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

#ifndef INCLUDED_irc_libio_h
#define INCLUDED_irc_libio_h

#ifdef _WIN32
# ifdef IN_LIBIO
#  define LIBIO_EXTERN extern __declspec(dllexport)
# else
#  define LIBIO_EXTERN extern __declspec(dllimport)
# endif
#else /* not _WIN32 */
# define LIBIO_EXTERN extern
#endif

#include <openssl/ssl.h>

#include "misc/event.h"
#include "misc/log.h"
#include "misc/misc.h"
#include "misc/libio_getopt.h"

#include "net/inet_misc.h"
#include "comm/fdlist.h"
#include "comm/fileio.h"
#include "comm/comm.h"

#include "mem/memory.h"

#include "net/irc_getaddrinfo.h"
#include "net/irc_getnameinfo.h"
#include "net/res.h"

#include "string/sprintf_irc.h"
#include "string/pcre.h"
#include "string/irc_string.h"

#endif /* INCLUDED_irc_libio_h */
