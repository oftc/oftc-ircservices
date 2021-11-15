/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  comm.c: Network functions.
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

#include "libioinc.h"

#ifndef _WIN32
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#endif

static const char *comm_err_str[] = { "Comm OK", "Error during bind()",
                                      "Error during DNS lookup",
                                      "connect timeout",
                                      "Error during connect()",
                                      "Comm Error" };

struct Callback *setup_socket_cb = NULL;

static void comm_connect_callback(fde_t *, int);
static PF comm_connect_timeout;
static void comm_connect_dns_callback(void *, struct DNSReply *);
static PF comm_connect_tryconnect;

extern void init_netio(void);

/* check_can_use_v6()
 *  Check if the system can open AF_INET6 sockets
 */
int
check_can_use_v6(void)
{
#ifdef IPV6
  int v6;

  if ((v6 = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
    return 0;
  else
  {
#ifdef _WIN32
    closesocket(v6);
#else
    close(v6);
#endif
    return 1;
  }
#else
  return 0;
#endif
}

/* get_sockerr - get the error value from the socket or the current errno
 *
 * Get the *real* error from the socket (well try to anyway..).
 * This may only work when SO_DEBUG is enabled but its worth the
 * gamble anyway.
 */
int
get_sockerr(int fd)
{
#ifndef _WIN32
  int errtmp = errno;
#else
  int errtmp = WSAGetLastError();
#endif
#ifdef SO_ERROR
  int err = 0;
  socklen_t len = sizeof(err);

  if (fd > -1 && !getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*) &err, (socklen_t *)&len))
  {
    if (err)
      errtmp = err;
  }

  errno = errtmp;
#endif
  return errtmp;
}

/*
 * setup_socket()
 *
 * Set the socket non-blocking, and other wonderful bits.
 */
static void *
setup_socket(va_list args)
{
  int fd = va_arg(args, int);
  int opt = 1;

  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &opt, sizeof(opt));

#ifdef IPTOS_LOWDELAY
  opt = IPTOS_LOWDELAY;
  setsockopt(fd, IPPROTO_IP, IP_TOS, (char *) &opt, sizeof(opt));
#endif

#ifndef _WIN32
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
#endif

  return NULL;
}

/*
 * init_comm()
 *
 * Initializes comm subsystem.
 */
void
init_comm(void)
{
  setup_socket_cb = register_callback("setup_socket", setup_socket);
  init_netio();
}

void
cleanup_comm()
{
  unregister_callback(setup_socket_cb);
}

/*
 * stolen from squid - its a neat (but overused! :) routine which we
 * can use to see whether we can ignore this errno or not. It is
 * generally useful for non-blocking network IO related errnos.
 *     -- adrian
 */
int
ignoreErrno(int ierrno)
{
  switch (ierrno)
  {
    case EINPROGRESS:
    case EWOULDBLOCK:
#if EAGAIN != EWOULDBLOCK
    case EAGAIN:
#endif
    case EALREADY:
    case EINTR:
#ifdef ERESTART
    case ERESTART:
#endif
        return 1;
    default:
        return 0;
  }
}

/*
 * comm_settimeout() - set the socket timeout
 *
 * Set the timeout for the fd
 */
void
comm_settimeout(fde_t *fd, time_t timeout, PF *callback, void *cbdata)
{
  assert(fd->flags.open);

  fd->timeout = CurrentTime + (timeout / 1000);
  fd->timeout_handler = callback;
  fd->timeout_data = cbdata;
}

/*
 * comm_setflush() - set a flush function
 *
 * A flush function is simply a function called if found during
 * comm_timeouts(). Its basically a second timeout, except in this case
 * I'm too lazy to implement multiple timeout functions! :-)
 * its kinda nice to have it separate, since this is designed for
 * flush functions, and when comm_close() is implemented correctly
 * with close functions, we _actually_ don't call comm_close() here ..
 * -- originally Adrian's notes
 * comm_close() is replaced with fd_close() in fdlist.c 
 */
void
comm_setflush(fde_t *fd, time_t timeout, PF *callback, void *cbdata)
{
  assert(fd->flags.open);

  fd->flush_timeout = CurrentTime + (timeout / 1000);
  fd->flush_handler = callback;
  fd->flush_data = cbdata;
}

/*
 * comm_checktimeouts() - check the socket timeouts
 *
 * All this routine does is call the given callback/cbdata, without closing
 * down the file descriptor. When close handlers have been implemented,
 * this will happen.
 */
void
comm_checktimeouts(void *notused)
{
  int i;
  fde_t *F;
  PF *hdl;
  void *data;

  for (i = 0; i < FD_HASH_SIZE; ++i)
  {
    for (F = fd_hash[i]; F != NULL; F = fd_next_in_loop)
    {
      assert(F->flags.open);
      fd_next_in_loop = F->hnext;

      /* check flush functions */
      if (F->flush_handler && F->flush_timeout > 0 &&
          F->flush_timeout < CurrentTime)
      {
        hdl = F->flush_handler;
        data = F->flush_data;
        comm_setflush(F, 0, NULL, NULL);
        hdl(F, data);
      }

      /* check timeouts */
      if (F->timeout_handler && F->timeout > 0 &&
          F->timeout < CurrentTime)
      {
        /* Call timeout handler */
        hdl = F->timeout_handler;
        data = F->timeout_data;
        comm_settimeout(F, 0, NULL, NULL);
        hdl(F, data);
      }
    }
  }
}

/*
 * void comm_connect_tcp(int fd, const char *host, unsigned short port,
 *                       struct sockaddr *clocal, int socklen,
 *                       CNCB *callback, void *data, int aftype, int timeout)
 * Input: An fd to connect with, a host and port to connect to,
 *        a local sockaddr to connect from + length(or NULL to use the
 *        default), a callback, the data to pass into the callback, the
 *        address family.
 * Output: None.
 * Side-effects: A non-blocking connection to the host is started, and
 *               if necessary, set up for selection. The callback given
 *               may be called now, or it may be called later.
 */
void
comm_connect_tcp(fde_t *fd, const char *host, unsigned short port,
                 struct sockaddr *clocal, int socklen, CNCB *callback,
                 void *data, int aftype, int timeout)
{
  struct addrinfo hints, *res;
  char portname[PORTNAMELEN+1];

  assert(callback);
  fd->connect.callback = callback;
  fd->connect.data = data;

  fd->connect.hostaddr.ss.ss_family = aftype;
  fd->connect.hostaddr.ss_port = htons(port);

  /* Note that we're using a passed sockaddr here. This is because
   * generally you'll be bind()ing to a sockaddr grabbed from
   * getsockname(), so this makes things easier.
   * XXX If NULL is passed as local, we should later on bind() to the
   * virtual host IP, for completeness.
   *   -- adrian
   */
  if ((clocal != NULL) && (bind(fd->fd, clocal, socklen) < 0))
  { 
    /* Failure, call the callback with COMM_ERR_BIND */
    comm_connect_callback(fd, COMM_ERR_BIND);
    /* ... and quit */
    return;
  }

  /* Next, if we have been given an IP, get the addr and skip the
   * DNS check (and head direct to comm_connect_tryconnect().
   */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

  snprintf(portname, PORTNAMELEN, "%d", port);

  if (irc_getaddrinfo(host, portname, &hints, &res))
  {
    /* Send the DNS request, for the next level */
    fd->dns_query = MyMalloc(sizeof(struct DNSQuery));
    fd->dns_query->ptr = fd;
    fd->dns_query->callback = comm_connect_dns_callback;
    gethost_byname(host, fd->dns_query);
  }
  else
  {
    /* We have a valid IP, so we just call tryconnect */
    /* Make sure we actually set the timeout here .. */
    assert(res != NULL);

    memcpy(&fd->connect.hostaddr, res->ai_addr, res->ai_addrlen);

    fd->connect.hostaddr.ss_len = res->ai_addrlen;
    fd->connect.hostaddr.ss.ss_family = res->ai_family;

    irc_freeaddrinfo(res);
    comm_settimeout(fd, timeout*1000, comm_connect_timeout, NULL);
    comm_connect_tryconnect(fd, NULL);
  }
}

/*
 * comm_connect_callback() - call the callback, and continue with life
 */
static void
comm_connect_callback(fde_t *fd, int status)
{
  CNCB *hdl;

  /* This check is gross..but probably necessary */
  if (fd->connect.callback == NULL)
    return;

  /* Clear the connect flag + handler */
  hdl = fd->connect.callback;
  fd->connect.callback = NULL;

  /* Clear the timeout handler */
  comm_settimeout(fd, 0, NULL, NULL);

  /* Call the handler */
  hdl(fd, status, fd->connect.data);
}

/*
 * comm_connect_timeout() - this gets called when the socket connection
 * times out. This *only* can be called once connect() is initially
 * called ..
 */
static void
comm_connect_timeout(fde_t *fd, void *notused)
{
  /* error! */
  comm_connect_callback(fd, COMM_ERR_TIMEOUT);
}

/*
 * comm_connect_dns_callback() - called at the completion of the DNS request
 *
 * The DNS request has completed, so if we've got an error, return it,
 * otherwise we initiate the connect()
 */
static void
comm_connect_dns_callback(void *vptr, struct DNSReply *reply)
{
  fde_t *F = vptr;

  if (reply == NULL)
  {
    MyFree(F->dns_query);
    F->dns_query = NULL;

    comm_connect_callback(F, COMM_ERR_DNS);
    return;
  }

  /* No error, set a 10 second timeout */
  comm_settimeout(F, 30*1000, comm_connect_timeout, NULL);

  /* Copy over the DNS reply info so we can use it in the connect() */
  /*
   * Note we don't fudge the refcount here, because we aren't keeping
   * the DNS record around, and the DNS cache is gone anyway.. 
   *     -- adrian
   */
  memcpy(&F->connect.hostaddr, &reply->addr, reply->addr.ss_len);
  /* The cast is hacky, but safe - port offset is same on v4 and v6 */
  ((struct sockaddr_in *) &F->connect.hostaddr)->sin_port =
    F->connect.hostaddr.ss_port;
  F->connect.hostaddr.ss_len = reply->addr.ss_len;

  /* Now, call the tryconnect() routine to try a connect() */
  MyFree(F->dns_query);
  F->dns_query = NULL;
  comm_connect_tryconnect(F, NULL);
}

/* static void comm_connect_tryconnect(int fd, void *notused)
 * Input: The fd, the handler data(unused).
 * Output: None.
 * Side-effects: Try and connect with pending connect data for the FD. If
 *               we succeed or get a fatal error, call the callback.
 *               Otherwise, it is still blocking or something, so register
 *               to select for a write event on this FD.
 */
static void
comm_connect_tryconnect(fde_t *fd, void *notused)
{
  int retval;

  /* This check is needed or re-entrant s_bsd_* like sigio break it. */
  if (fd->connect.callback == NULL)
    return;

  /* Try the connect() */
  retval = connect(fd->fd, (struct sockaddr *)&fd->connect.hostaddr, 
                   fd->connect.hostaddr.ss_len);

  /* Error? */
  if (retval < 0)
  {
#ifdef _WIN32
    errno = WSAGetLastError();
#endif
    /*
     * If we get EISCONN, then we've already connect()ed the socket,
     * which is a good thing.
     *   -- adrian
     */
    if (errno == EISCONN)
      comm_connect_callback(fd, COMM_OK);
    else if (ignoreErrno(errno))
      /* Ignore error? Reschedule */
      comm_setselect(fd, COMM_SELECT_WRITE, comm_connect_tryconnect,
                     NULL, 0);
    else
      /* Error? Fail with COMM_ERR_CONNECT */
      comm_connect_callback(fd, COMM_ERR_CONNECT);
    return;
  }

  /* If we get here, we've succeeded, so call with COMM_OK */
  comm_connect_callback(fd, COMM_OK);
}

/*
 * comm_errorstr() - return an error string for the given error condition
 */
const char *
comm_errstr(int error)
{
  if (error < 0 || error >= COMM_ERR_MAX)
    return "Invalid error number!";
  return comm_err_str[error];
}

/*
 * comm_open() - open a socket
 *
 * This is a highly highly cut down version of squid's comm_open() which
 * for the most part emulates socket(), *EXCEPT* it fails if we're about
 * to run out of file descriptors.
 */
int
comm_open(fde_t *F, int family, int sock_type, int proto, const char *note)
{
  int fd;

  /* First, make sure we aren't going to run out of file descriptors */
  if (number_fd >= hard_fdlimit)
  {
    errno = ENFILE;
    return -1;
  }

  /*
   * Next, we try to open the socket. We *should* drop the reserved FD
   * limit if/when we get an error, but we can deal with that later.
   * XXX !!! -- adrian
   */
  fd = socket(family, sock_type, proto);
  if (fd < 0)
  {
#ifdef _WIN32
    errno = WSAGetLastError();
#endif
    return -1; /* errno will be passed through, yay.. */
  }

  execute_callback(setup_socket_cb, fd);

  /* update things in our fd tracking */
  fd_open(F, fd, 1, note);
  return 0;
}

/*
 * comm_accept() - accept an incoming connection
 *
 * This is a simple wrapper for accept() which enforces FD limits like
 * comm_open() does. Returned fd must be either closed or tagged with
 * fd_open (this function no longer does it).
 */
int
comm_accept(fde_t *listener, struct irc_ssaddr *pn)
{
  int newfd;
  socklen_t addrlen = sizeof(struct irc_ssaddr);

  if (number_fd >= hard_fdlimit)
  {
    errno = ENFILE;
    return -1;
  }

  /*
   * Next, do the accept(). if we get an error, we should drop the
   * reserved fd limit, but we can deal with that when comm_open()
   * also does it. XXX -- adrian
   */
  newfd = accept(listener->fd, (struct sockaddr *)pn, (socklen_t *)&addrlen);
  if (newfd < 0)
  {
#ifdef _WIN32
    errno = WSAGetLastError();
#endif
    return -1;
  }

#ifdef IPV6
  remove_ipv6_mapping(pn);
#else
  pn->ss_len = addrlen;
#endif

  execute_callback(setup_socket_cb, newfd);

  /* .. and return */
  return newfd;
}

/* 
 * remove_ipv6_mapping() - Removes IPv4-In-IPv6 mapping from an address
 * This function should really inspect the struct itself rather than relying
 * on inet_pton and inet_ntop.  OSes with IPv6 mapping listening on both
 * AF_INET and AF_INET6 map AF_INET connections inside AF_INET6 structures
 * 
 */
#ifdef IPV6
void
remove_ipv6_mapping(struct irc_ssaddr *addr)
{
  if (addr->ss.ss_family == AF_INET6)
  {
    struct sockaddr_in6 *v6;

    v6 = (struct sockaddr_in6 *)addr;
    if (IN6_IS_ADDR_V4MAPPED(&v6->sin6_addr))
    {
      char v4ip[HOSTIPLEN];
      struct sockaddr_in *v4 = (struct sockaddr_in*)addr;
      inetntop(AF_INET6, &v6->sin6_addr, v4ip, HOSTIPLEN);
      inet_pton(AF_INET, v4ip, &v4->sin_addr);
      addr->ss.ss_family = AF_INET;
      addr->ss_len = sizeof(struct sockaddr_in);
    }
    else 
      addr->ss_len = sizeof(struct sockaddr_in6);
  }
  else
    addr->ss_len = sizeof(struct sockaddr_in);
} 
#endif
