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

/* Language defines */

#define NS_ALREADY_REG    0
#define NS_REG_COMPLETE   1
#define NS_REG_FAIL       2
#define NS_REG_FIRST      3
#define NS_IDENTIFIED     4
#define NS_IDENT_FAIL     5
#define NS_LAST           6

#define _N(c, m) ((c)->nickname == NULL) ? \
            nickserv->language_table[(c)->nickname->language][(m)] : \
            nickserv->language_table[0][(m)]

#endif
