/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  comm.h: A header for the network subsystem.
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

#ifndef INCLUDED_libio_comm_comm_h
#define INCLUDED_libio_comm_comm_h

/* Type of IO */
#define	COMM_SELECT_READ  1
#define	COMM_SELECT_WRITE 2

/* How long can comm_select() wait for network events [milliseconds] */
#define SELECT_DELAY 500

/* sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255.ipv6") */
#define HOSTIPLEN   53
#define PORTNAMELEN 6  /* ":31337" */

#ifdef _WIN32
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#define EALREADY     WSAEALREADY
#define EINPROGRESS  WSAEINPROGRESS
#define EISCONN      WSAEISCONN
#define EMSGSIZE     WSAEMSGSIZE
#define EWOULDBLOCK  WSAEWOULDBLOCK
#endif

LIBIO_EXTERN struct Callback *setup_socket_cb;
#ifdef _WIN32
LIBIO_EXTERN void (* dispatch_wm_signal) (int);
#endif

LIBIO_EXTERN int get_sockerr(int);
LIBIO_EXTERN int ignoreErrno(int);

LIBIO_EXTERN void comm_settimeout(fde_t *, time_t, PF *, void *);
LIBIO_EXTERN void comm_setflush(fde_t *, time_t, PF *, void *);
LIBIO_EXTERN void comm_checktimeouts(void *);
LIBIO_EXTERN void comm_connect_tcp(fde_t *, const char *, u_short,
           		     struct sockaddr *, int, CNCB *, void *, int, int);
LIBIO_EXTERN const char *comm_errstr(int status);
LIBIO_EXTERN int comm_open(fde_t *, int, int, int, const char *);
LIBIO_EXTERN int comm_accept(fde_t *, struct irc_ssaddr *);

/* These must be defined in the network IO loop code of your choice */
LIBIO_EXTERN void comm_setselect(fde_t *, unsigned int, PF *, void *, time_t);
#ifdef IN_MISC_C
extern void init_comm(void);
#endif
LIBIO_EXTERN void comm_select(void);
LIBIO_EXTERN int check_can_use_v6(void);
#ifdef IPV6
LIBIO_EXTERN void remove_ipv6_mapping(struct irc_ssaddr *);
#endif

#endif /* INCLUDED_libio_comm_comm_h */
