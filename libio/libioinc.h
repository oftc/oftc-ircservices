/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  libioinc.h: libio standard includes.
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
 *  $Id: libio.h 86 2005-10-05 20:36:04Z adx $
 */

#include "setup.h"
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <openssl/ssl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "irc_libio.h"

#define LEAKED_FDS 10
#define INIT_LOG_LEVEL L_NOTICE
#define IRC_MAX(a, b)  ((a) > (b) ? (a) : (b))
#define IRC_MIN(a, b)  ((a) < (b) ? (a) : (b))
