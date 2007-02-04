/*
 *  conf.h: Includes all configuration headers.
 *
 *  Copyright (C) 2003 by Piotr Nizynski, Advanced IRC Services Project
 *  Copyright (C) 2005 by the Hybrid Development Team.
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

#ifndef INCLUDED_conf_conf_h
#define INCLUDED_conf_conf_h

#include "conf/manager.h"
#include "conf/servicesinfo.h"
#include "conf/database.h"
#include "conf/connect.h"
#include "conf/logging.h"
#ifdef USE_SHARED_MODULES
#include "conf/modules.h"
#endif

#define CONF_FLAGS_DO_IDENTD            0x00000001
#define CONF_FLAGS_LIMIT_IP             0x00000002
#define CONF_FLAGS_NO_TILDE             0x00000004
#define CONF_FLAGS_NEED_IDENTD          0x00000008
/*                                      0x00000010 */
#define CONF_FLAGS_NOMATCH_IP           0x00000020
#define CONF_FLAGS_EXEMPTKLINE          0x00000040
#define CONF_FLAGS_NOLIMIT              0x00000080
#define CONF_FLAGS_IDLE_LINED           0x00000100
#define CONF_FLAGS_SPOOF_IP             0x00000200
#define CONF_FLAGS_SPOOF_NOTICE         0x00000400
#define CONF_FLAGS_REDIR                0x00000800
#define CONF_FLAGS_EXEMPTGLINE          0x00001000
#define CONF_FLAGS_RESTRICTED           0x00002000
#define CONF_FLAGS_CAN_FLOOD            0x00100000
#define CONF_FLAGS_NEED_PASSWORD        0x00200000
/* server flags */
#define CONF_FLAGS_ALLOW_AUTO_CONN      0x00004000
#define CONF_FLAGS_ENCRYPTED            0x00010000
#define CONF_FLAGS_COMPRESSED           0x00020000
#define CONF_FLAGS_TEMPORARY            0x00040000
#define CONF_FLAGS_CRYPTLINK            0x00080000
#define CONF_FLAGS_BURST_AWAY           0x00400000
#define CONF_FLAGS_EXEMPTRESV           0x00800000
#define CONF_FLAGS_TOPICBURST           0x01000000

/* Macros for struct AccessItem */
#define IsLimitIp(x)            ((x)->flags & CONF_FLAGS_LIMIT_IP)
#define IsNoTilde(x)            ((x)->flags & CONF_FLAGS_NO_TILDE)
#define IsConfCanFlood(x)       ((x)->flags & CONF_FLAGS_CAN_FLOOD)
#define IsNeedPassword(x)       ((x)->flags & CONF_FLAGS_NEED_PASSWORD)
#define IsNeedIdentd(x)         ((x)->flags & CONF_FLAGS_NEED_IDENTD)
#define IsNoMatchIp(x)          ((x)->flags & CONF_FLAGS_NOMATCH_IP)
#define IsConfExemptKline(x)    ((x)->flags & CONF_FLAGS_EXEMPTKLINE)
#define IsConfExemptLimits(x)   ((x)->flags & CONF_FLAGS_NOLIMIT)
#define IsConfExemptGline(x)    ((x)->flags & CONF_FLAGS_EXEMPTGLINE)
#define IsConfExemptResv(x)     ((x)->flags & CONF_FLAGS_EXEMPTRESV)
#define IsConfIdlelined(x)      ((x)->flags & CONF_FLAGS_IDLE_LINED)
#define IsConfDoIdentd(x)       ((x)->flags & CONF_FLAGS_DO_IDENTD)
#define IsConfDoSpoofIp(x)      ((x)->flags & CONF_FLAGS_SPOOF_IP)
#define IsConfSpoofNotice(x)    ((x)->flags & CONF_FLAGS_SPOOF_NOTICE)
#define IsConfRestricted(x)     ((x)->flags & CONF_FLAGS_RESTRICTED)
#define IsConfEncrypted(x)      ((x)->flags & CONF_FLAGS_ENCRYPTED)
#define SetConfEncrypted(x)     ((x)->flags |= CONF_FLAGS_ENCRYPTED)
#define ClearConfEncrypted(x)   ((x)->flags &= ~CONF_FLAGS_ENCRYPTED)
#define IsConfCompressed(x)     ((x)->flags & CONF_FLAGS_COMPRESSED)
#define SetConfCompressed(x)    ((x)->flags |= CONF_FLAGS_COMPRESSED)
#define ClearConfCompressed(x)  ((x)->flags &= ~CONF_FLAGS_COMPRESSED)
#define IsConfCryptLink(x)      ((x)->flags & CONF_FLAGS_CRYPTLINK)
#define SetConfCryptLink(x)     ((x)->flags |= CONF_FLAGS_CRYPTLINK)
#define ClearConfCryptLink(x)   ((x)->flags &= ~CONF_FLAGS_CRYPTLINK)
#define IsConfAllowAutoConn(x)  ((x)->flags & CONF_FLAGS_ALLOW_AUTO_CONN)
#define SetConfAllowAutoConn(x) ((x)->flags |= CONF_FLAGS_ALLOW_AUTO_CONN)
#define ClearConfAllowAutoConn(x) ((x)->flags &= ~CONF_FLAGS_ALLOW_AUTO_CONN)
#define IsConfTemporary(x)      ((x)->flags & CONF_FLAGS_TEMPORARY)
#define SetConfTemporary(x)     ((x)->flags |= CONF_FLAGS_TEMPORARY)
#define IsConfRedir(x)          ((x)->flags & CONF_FLAGS_REDIR)
#define IsConfAwayBurst(x)      ((x)->flags & CONF_FLAGS_BURST_AWAY)
#define SetConfAwayBurst(x)     ((x)->flags |= CONF_FLAGS_BURST_AWAY)
#define ClearConfAwayBurst(x)   ((x)->flags &= ~CONF_FLAGS_BURST_AWAY)
#define IsConfTopicBurst(x)     ((x)->flags & CONF_FLAGS_TOPICBURST)
#define SetConfTopicBurst(x)    ((x)->flags |= CONF_FLAGS_TOPICBURST)
#define ClearConfTopicBurst(x)  ((x)->flags &= ~CONF_FLAGS_TOPICBURST)


typedef enum
{
  CONF_TYPE,
  CLASS_TYPE,
  OPER_TYPE,
  CLIENT_TYPE,
  SERVER_TYPE,
  HUB_TYPE,
  LEAF_TYPE,
  KLINE_TYPE,
  DLINE_TYPE,
  EXEMPTDLINE_TYPE,
  CLUSTER_TYPE,
  RKLINE_TYPE,
  RXLINE_TYPE,
  XLINE_TYPE,
  ULINE_TYPE,
  GLINE_TYPE,
  CRESV_TYPE,
  NRESV_TYPE,
  GDENY_TYPE
} ConfType;

struct AccessItem
{
  void *conf_ptr;            /* pointer back to conf */
  dlink_node node;
  unsigned int     status;   /* If CONF_ILLEGAL, delete when no clients */
  unsigned int     flags;
  unsigned int     modes;
  struct irc_ssaddr my_ipnum; /* ip to bind to for outgoing connect */
  struct irc_ssaddr ipnum;      /* ip to connect to */
  char *           host;     /* host part of user@host */
  char *           passwd;
  char *           spasswd;  /* Password to send. */
  char *           reason;
  char *           oper_reason;
  char *           user;     /* user part of user@host */
  int              port;
  char *           fakename;   /* Mask name */
  time_t           hold;     /* Hold action until this time (calendar time) */
  struct ConfItem *class_ptr;  /* Class of connection */
  struct DNSQuery* dns_query;
  int              aftype;
#ifdef HAVE_LIBCRYPTO
  char *           rsa_public_key_file;
  RSA *            rsa_public_key;
  struct EncCapability *cipher_preference;
#endif
  pcre *regexuser;
  pcre *regexhost;
};


struct ConfItem
{
  char *name;           /* Primary key */
  pcre *regexpname;
  dlink_node node;      /* link into known ConfItems of this type */
  unsigned int flags;
  ConfType type;
  dlink_list mask_list;

  union
  {
//    struct MatchItem MatchItem;
    struct AccessItem AccessItem;
//    struct ClassItem ClassItem;
//    struct ResvChannel ResvChannel;
  } conf;
};
#endif /* INCLUDED_conf_conf_h */
