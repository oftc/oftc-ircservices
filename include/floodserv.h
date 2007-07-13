/* TODO: add copyright block */

#ifndef INCLUDED_floodserv_h
#define INCLUDED_floodserv_h

#include "floodserv-lang.h"

#define FS_MSG_COUNT 5
#define FS_MSG_TIME  60
#define FS_LNE_TIME  3

#define FS_GMSG_COUNT 10
#define FS_GMSG_TIME  60

#define FS_KILL_MSG "This host triggered network flood protection. "\
    "please mail support@oftc.net if you feel this is in error, quoting "\
    "this message."
#define FS_KILL_DUR 2592000 /* 30 Days */

/* GC Timer, how often the routine should fire */
/* Default once every min */
#define FS_GC_EVENT_TIMER 60
/* The smallest age at which to free a queue */
/* Default every 10 mins */
#define FS_GC_EXPIRE_TIME 600

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
