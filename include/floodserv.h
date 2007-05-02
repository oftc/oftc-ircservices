/* TODO: add copyright block */

#ifndef INCLUDED_floodserv_h
#define INCLUDED_floodserv_h

#include "floodserv-lang.h"

#define FS_MSG_COUNT 5
#define FS_MSG_TIME  60
#define FS_LNE_TIME  3

#define FS_GMSG_COUNT 10
#define FS_GMSG_TIME  60

#define FS_KILL_MSG "This Host Triggered Network Flood Protection, please mail support@oftc.net" 
#define FS_KILL_DUR 2592000 /* 30 Days */

/* GC Timer, how often the routine should fire */
/* Default every hour */
#define FS_GC_EVENT_TIMER 1200
/* Only interate channels that have at least this many unique hosts in their
 * queues, so to save time on the gc routine */
/* Default 25 */
#define FS_GC_LIST_LENGTH 25
/* The smallest age at which to free a queue, if someone hasn't spoken since
 * the last GC run it's probably safe to free their queue */
/* Default one hour */
#define FS_GC_EXPIRE_TIME 1200

enum MessageQueueType
{
  MQUEUE_CHAN,
  MQUEUE_GLOB,
};

enum MQueueEnforce
{
  MQUEUE_NONE,
  MQUEUE_LINE,
  MQUEUE_MESG,
};

#endif /* INCLUDED_floodserv_h */
