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
#define CS_HELP_REG_LONG  2
#define CS_HELP_SHORT     3 
#define CS_HELP_LONG      4
#define CS_REG_NS_FIRST   5
#define CS_ALREADY_REG    6
#define CS_REG_SUCCESS    7
#define CS_REG_FAIL       8
#define CS_NAMESTART_HASH 9
#define CS_SET_SHORT      10
#define CS_SET_LONG       11
#define CS_NOT_ONCHAN     12
#define CS_NOT_OPPED      13
#define CS_NOT_REG        14
#define CS_OWN_CHANNEL_ONLY 15
#define CS_REGISTERED     16
#define CS_REG_FAILED     17

#endif
