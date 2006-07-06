#ifndef CHANSERV_H
#define CHANSERV_H

struct RegChannel
{
  dlink_node node;
  
  unsigned int id;
  char founder[NICKLEN+1];
  char channel[CHANNELLEN+1];
};

#define CS_HELP_REG_SHORT 1
#define CS_HELP_REG_LONG 2
#define CS_HELP_SHORT 3 
#define CS_HELP_LONG 4

#endif
