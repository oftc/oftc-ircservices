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
#define CS_REGISTER_NICK  5
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
#define CS_DROPPED        16
#define CS_DROP_FAILED    17
#define CS_SET_FOUNDER    18
#define CS_SET_FOUNDER_FAILED 19
#define CS_SET_SUCC       20
#define CS_SET_SUCC_FAILED 21
#define CS_SET_DESC       22
#define CS_SET_DESC_FAILED 23
#define CS_SET_URL       24
#define CS_SET_URL_FAILED 25
#define CS_SET_EMAIL       26
#define CS_SET_EMAIL_FAILED 27
#define CS_SET_MSG          28
#define CS_SET_MSG_FAILED   29

#define CS_SET_FOUNDER_SHORT 30
#define CS_SET_FOUNDER_LONG  31
#define CS_SET_SUCC_SHORT
#define CS_SET_SUCC_LONG
#define CS_SET_DESC_SHORT
#define CS_SET_DESC_LONG
#define CS_SET_URL_SHORT
#define CS_SET_URL_LONG
#define CS_SET_EMAIL_SHORT
#define CS_SET_EMAIL_LONG
#define CS_SET_ENTRYMSG_SHORT
#define CS_SET_ENTRYMSG_LONG

#endif
