/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  packet.c: Deal with incoming messages from the server
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
#include "packet.h"
#include "client.h"
#include "parse.h"

struct Callback *iorecv_cb = NULL;
struct Callback *iosend_cb = NULL;
static char readBuf[READBUF_SIZE];

/*
 * client_dopacket - copy packet to client buf and parse it
 *      client - pointer to client structure for which the buffer data
 *             applies.
 *      buffer - pointr to the buffer containing the newly read data
 *      length - number of valid bytes of data in the buffer
 *
 * Note:
 *      It is implicitly assumed that dopacket is called only
 *      with client_p of "local" variation, which contains all the
 *      necessary fields (buffer etc..)
 */
static void
client_dopacket(struct Client *client, char *buffer, size_t length)
{
  /*
   * Update messages received
  ++me.localClient->recv.messages;
  ++client_p->localClient->recv.messages;

   * Update bytes received
  client_p->localClient->recv.bytes += length;
  me.localClient->recv.bytes += length;
*/

  parse(client, buffer, buffer + length);
}


/* extract_one_line()
 *
 * inputs       - pointer to a dbuf queue
 *              - pointer to buffer to copy data to
 * output       - length of <buffer>
 * side effects - one line is copied and removed from the dbuf
 */
static int
extract_one_line(struct dbuf_queue *qptr, char *buffer)
{
  struct dbuf_block *block;
  int line_bytes = 0, empty_bytes = 0, phase = 0;
  unsigned int idx;

  char c;
  dlink_node *ptr;

  /*
   * Phase 0: "empty" characters before the line
   * Phase 1: copying the line
   * Phase 2: "empty" characters after the line
   *          (delete them as well and free some space in the dbuf)
   *
   * Empty characters are CR, LF and space (but, of course, not
   * in the middle of a line). We try to remove as much of them as we can,
   * since they simply eat server memory.
   *
   * --adx
   */
  DLINK_FOREACH(ptr, qptr->blocks.head)
  {
    block = ptr->data;

    for (idx = 0; idx < block->size; idx++)
    {
      c = block->data[idx];
      if (IsEol(c) || (c == ' ' && phase != 1))
      {
        empty_bytes++;
        if (phase == 1)
          phase = 2;
      }
      else switch (phase)
      {
        case 0: phase = 1;
        case 1: if (line_bytes++ < IRC_BUFSIZE - 2)
                  *buffer++ = c;
                break;
        case 2: *buffer = '\0';
                dbuf_delete(qptr, line_bytes + empty_bytes);
                return IRC_MIN(line_bytes, IRC_BUFSIZE - 2);
      }
    }
  }

  /*
   * Now, if we haven't reached phase 2, ignore all line bytes
   * that we have read, since this is a partial line case.
   */
  if (phase != 2)
    line_bytes = 0;
  else
    *buffer = '\0';

  /* Remove what is now unnecessary */
  dbuf_delete(qptr, line_bytes + empty_bytes);
  return IRC_MIN(line_bytes, IRC_BUFSIZE - 2);
}

/*
 * parse_client_queued - parse client queued messages
 */
static void
parse_client_queued(struct Client *client)
{
  int dolen = 0;

  while (1)
  {
    if (IsDefunct(client->server))
      return;
    if ((dolen = extract_one_line(&client->server->buf_recvq, readBuf)) == 0)
      break;
    client_dopacket(client, readBuf, dolen);
  }
}


void
read_packet(fde_t *fd, void *data)
{
  struct Client *client = (struct Client*)data;
  int length = 0;

  do
  {
    length = recv(fd->fd, readBuf, READBUF_SIZE, 0);
#ifdef _WIN32
    if (length < 0)
      errno = WSAGetLastError();
#endif
    if (length <= 0)
    {
      /*
       * If true, then we can recover from this error.  Just jump out of
       * the loop and re-register a new io-request.
       */
      if (length < 0 && ignoreErrno(errno))
        break;

      ilog(L_ERROR, "Lost server connection, trying reconnect");
      exit_client(me.uplink, &me, "Connection lost");
      me.uplink = NULL;
      return;
    }
    
    execute_callback(iorecv_cb, client, length, readBuf);
    parse_client_queued(client);
  } while (length == sizeof(readBuf));
  comm_setselect(fd, COMM_SELECT_READ, read_packet, client, 0);
}

/*
 * iorecv_default - append a packet to the recvq dbuf
 */
void *
iorecv_default(va_list args)
{
  struct Client *client = va_arg(args, struct Client*);
  int length = va_arg(args, int);
  char *buf = va_arg(args, char *);

  dbuf_put(&client->server->buf_recvq, buf, length);

  return NULL;
}
