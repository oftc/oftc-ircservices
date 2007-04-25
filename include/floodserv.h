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
struct FloodMsg
{
  time_t time;
  char *message;
};

struct MessageQueue
{
  char *name;
  int last;
  int max;
  int msg_enforce_time;
  int lne_enforce_time;
  struct FloodMsg **entries;
  unsigned int type;
  struct MessageQueue *hnext;
  struct MessageQueue *next;
  dlink_node node;
};

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
