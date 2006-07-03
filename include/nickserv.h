#ifndef NICKSERV_H
#define NICKSERV_H

struct Nick
{
  dlink_node node;
  
  unsigned int id;
  char nick[NICKLEN+1];
  char pass[35];
  unsigned int status;
  unsigned int flags;
  unsigned short language;
  time_t reg_time;
  time_t last_seen;
  time_t last_used;
  time_t last_quit_time;
};

/* Nick flags */
#define NS_FLAG_ADMIN     0x1

#define IsServAdmin(x)    (x)->nickname->flags & NS_FLAG_ADMIN

/* Language defines */

#define NS_ALREADY_REG    0
#define NS_REG_COMPLETE   1
#define NS_REG_FAIL       2
#define NS_REG_FIRST      3
#define NS_IDENTIFIED     4
#define NS_IDENT_FAIL     5
#define NS_HELP_SHORT     6
#define NS_HELP_LONG      7
#define NS_HELP_REG_SHORT 8
#define NS_HELP_REG_LONG  9
#define NS_HELP_ID_SHORT  10
#define NS_HELP_ID_LONG   11
#define NS_LAST           12

#endif
