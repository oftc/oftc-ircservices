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

#endif
