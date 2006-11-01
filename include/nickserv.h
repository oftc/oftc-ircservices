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
  unsigned int language;
  time_t reg_time;
  time_t last_seen;
  time_t last_used;
  time_t last_quit_time;
};

/* Nick flags */
#define NS_FLAG_ADMIN     0x1

#define IsServAdmin(x)    (x)->nickname->flags & NS_FLAG_ADMIN

/* Language defines */

#define NS_ALREADY_REG      0x1
#define NS_REG_COMPLETE     0x2
#define NS_REG_FAIL         0x3
#define NS_REG_FIRST        0x4
#define NS_IDENTIFIED       0x5
#define NS_IDENT_FAIL       0x6
#define NS_HELP_SHORT       0x7
#define NS_HELP_LONG        0x8
#define NS_HELP_REG_SHORT   0x9
#define NS_HELP_REG_LONG    0xa
#define NS_HELP_ID_SHORT    0xb
#define NS_HELP_ID_LONG     0xc
#define NS_CURR_LANGUAGE    0xd
#define NS_AVAIL_LANGUAGE   0xe
#define NS_LANGUAGE_SET     0xf
#define NS_LAST             0x10
#define NS_SET_SUCCESS      0x11
#define NS_SET_FAILED       0x12
#define NS_HELP_DROP_SHORT  0x13
#define NS_HELP_DROP_LONG   0x14
#define NS_NEED_IDENTIFY    0x15

#endif
