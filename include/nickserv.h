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

#define NS_ALREADY_REG    1
#define NS_REG_COMPLETE   2
#define NS_REG_FAIL       3
#define NS_REG_FIRST      4
#define NS_IDENTIFIED     5
#define NS_IDENT_FAIL     6
#define NS_HELP_SHORT     7
#define NS_HELP_LONG      8
#define NS_HELP_REG_SHORT 9
#define NS_HELP_REG_LONG  10
#define NS_HELP_ID_SHORT  11
#define NS_HELP_ID_LONG   12
#define NS_CURR_LANGUAGE  13
#define NS_AVAIL_LANGUAGE 14
#define NS_LANGUAGE_SET   15
#define NS_LAST           16

#endif
