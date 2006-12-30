/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  msg.h: A header for the message handler structure.
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

#ifndef INCLUDED_msg_h
#define INCLUDED_msg_h

/*
 * MessageHandler
 */
typedef enum HandlerType {
  SERVER_HANDLER,
  ENCAP_HANDLER,
  LAST_HANDLER_TYPE
} HandlerType;

typedef enum ServicesPermission {
  USER_FLAG = 0,
  IDENTIFIED_FLAG,
  OPER_FLAG,
  ADMIN_FLAG
} ServicePermissionType;

typedef enum ChannelPermission {
  CHUSER_FLAG = 0,
  CHIDENTIFIED_FLAG,
  MEMBER_FLAG,
  CHANOP_FLAG,
  MASTER_FLAG
} ChannelPermissionType;

/*
 * MessageHandler function
 * Params:
 * struct Client* client   - connection message originated from
 * struct Client* source   - source of message, may be different from client * int            parc   - parameter count
 * char*          parv[] - parameter vector
 */
typedef void (*MessageHandler)(struct Client*, struct Client*, int, char*[]);
typedef void (*ServiceMessageHandler)(struct Service*, struct Client*, int, char*[]);

struct ServiceMessage
{
  struct ServiceMessage *sub; 
  const char *cmd;
  unsigned int count;       /* number of times command used */
  unsigned int parameters;  /* at least this many args must be passed
                             * or an error will be sent to the user
                             * before the m_func is even called
                             */
  unsigned int flags;       
  unsigned int access;      /* Access level required for using this command */
  unsigned int help_short;  /* Help index to show in generic HELP */
  unsigned int help_long;   /* Help index to show in HELP command */
  /*
   * service_p = service being spoken to
   * source_p = Source client ptr
   * parc = parameter count
   * parv = parameter variable array
   */
  ServiceMessageHandler handler;
};


/*
 * Constants
 */
#define   MFLG_SLOW             0x001   /* Command can be executed roughly
                                         * once per 2 seconds.                
					 */
#define   MFLG_UNREG            0x002   /* Command available to unregistered
                                         * clients.                   */       


#define   SFLG_UNREGOK          0x001   /* This message can be called for an 
                                           unregisted nick/channel */
#define   SFLG_ALIAS            0x002   /* This message is an alias of another
                                           and should not show in help */
#define   SFLG_KEEPARG          0x004   /* Keep the target argument as parv[1],
                                           eg set #foo bar baz becomes '#foo', 
                                           'baz' on command bar */
#define   SFLG_CHANARG          0x008   /* First argument should be a registered 
                                           channel */
#define   SFLG_NICKARG          0x010   /* First argument should be a registered 
                                           nickname */
extern void ms_error(struct Client *, struct Client *, int, char *[]);
#endif /* INCLUDED_msg_h */
