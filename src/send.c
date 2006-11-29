/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  send.c: Sending messages to servers and clients
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  Some parts:
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

#include "stdinc.h"

static void send_message(struct Client *, char *, int);

/*
 * iosend_default - append a packet to the client's sendq.
 */
void *
iosend_default(va_list args)
{
  struct Client *to = va_arg(args, struct Client *);
  int length = va_arg(args, int);
  char *buf = va_arg(args, char *);

  dbuf_put(&to->server->buf_sendq, buf, length);
  return NULL;
}

/* send_queued_all()
 *
 * input        - NONE
 * output       - NONE
 * side effects - try to flush sendq of each client
 */
void
send_queued_all(void)
{
  dlink_node *ptr;

  DLINK_FOREACH(ptr, global_server_list.head)
    send_queued_write((struct Client *) ptr->data);
}


/* send_format()
 *
 * inputs - buffer to format into
 *              - size of the buffer
 *    - format pattern to use
 *    - var args
 * output - number of bytes formatted output
 * side effects - modifies sendbuf
 */
static inline int
send_format(char *lsendbuf, int bufsize, const char *pattern, va_list args)
{
  int len;

  /*
   * from rfc1459
   *
   * IRC messages are always lines of characters terminated with a CR-LF
   * (Carriage Return - Line Feed) pair, and these messages shall not
   * exceed 512 characters in length,  counting all characters
   * including the trailing CR-LF.
   * Thus, there are 510 characters maximum allowed
   * for the command and its parameters.  There is no provision for
   * continuation message lines.  See section 7 for more details about
   * current implementations.
   */
  len = vsnprintf(lsendbuf, bufsize - 1, pattern, args);
  if (len > bufsize - 2)
    len = bufsize - 2;  /* required by some versions of vsnprintf */

  lsendbuf[len++] = '\r';
  lsendbuf[len++] = '\n';
  return len;
}


/* sendto_server()
 *
 * inputs - pointer to destination client
 *    - var args message
 * output - NONE
 * side effects - send message to single client
 */
void
sendto_server(struct Client *to, const char *pattern, ...)
{
  va_list args;
  char buffer[IRC_BUFSIZE];
  int len;

  if (to->from != NULL)
    to = to->from;
  if (IsDead(to->server))
    return; /* This socket has already been marked as dead */

  va_start(args, pattern);
  len = send_format(buffer, IRC_BUFSIZE, pattern, args);
  va_end(args);

  send_message(to, buffer, len);
}

/*
 ** send_message
 **      Internal utility which appends given buffer to the sockets
 **      sendq.
 */
static void
send_message(struct Client *to, char *buf, int len)
{
  assert(!IsMe(to));
  assert(&me != to);
  assert(len <= IRC_BUFSIZE);

  execute_callback(iosend_cb, to, len, buf);
  if (dbuf_length(&to->server->buf_sendq) >
      (IsServer(to) ? (unsigned int) 1024 : (unsigned int) 4096))
    send_queued_write(to);
}

/*
 ** send_queued_write
 **      This is called when there is a chance that some output would
 **      be possible. This attempts to empty the send queue as far as
 **      possible, and then if any data is left, a write is rescheduled.
 */
void
send_queued_write(struct Client *to)
{
  int retlen;
  struct dbuf_block *first;

  /*
   ** Once socket is marked dead, we cannot start writing to it,
   ** even if the error is removed...
   */
  if (IsDead(to->server))
    return;  /* no use calling send() now */

  /* Next, lets try to write some data */

  if (dbuf_length(&to->server->buf_sendq))
  {
    do {
      first = to->server->buf_sendq.blocks.head->data;

      retlen = send(to->server->fd.fd, first->data, first->size, 0);

      if (retlen <= 0)
      {
#ifdef _WIN32
        errno = WSAGetLastError();
#endif
        break;
      }

      dbuf_delete(&to->server->buf_sendq, retlen);

      /* We have some data written .. update counters */
    }
    while (dbuf_length(&to->server->buf_sendq));

    if (retlen <= 0)
    {
      dead_link_on_write(to, errno);
      return;
    }
  }
}


