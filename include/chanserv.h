#ifndef CHANSERV_H
#define CHANSERV_H

struct RegChannel
{
  dlink_node node;
  
  unsigned int id;
  int founder;
  int successor;
  char channel[CHANNELLEN+1];
  char *description;
  char *entrymsg; 
  char *url;
  char *email;
  char *topic;
  unsigned int flags;
};

/* channel flag defines */
#define CHSET_KEEPTOPIC  0x00000001
#define CHSET_SECUREOPS  0x00000002
#define CHSET_PRIVATE    0x00000004
#define CHSET_TOPICLOCK  0x00000008
#define CHSET_RESTRICTED 0x00000010
#define CHSET_LEAVEOPS   0x00000020
#define CHSET_SECURE     0x00000040
#define CHSET_VERBOSE    0x00000800

#define  SetChanKeeptopic(x)    ((x)->flags |= CHSET_KEEPTOPIC)
#define  SetChanTopiclock(x)    ((x)->flags |= CHSET_TOPICLOCK)
#define  SetChanPrivate(x)      ((x)->flags |= CHSET_PRIVATE)
#define  SetChanRestricted(x)   ((x)->flags |= CHSET_RESTRICTED)
#define  SetChanSecure(x)       ((x)->flags |= CHSET_SECURE)
#define  SetChanSecureops(x)    ((x)->flags |= CHSET_SECUREOPS)
#define  SetChanLeaveops(x)     ((x)->flags |= CHSET_LEAVEOPS)
#define  SetChanVerbose(x)      ((x)->flags |= CHSET_VERBOSE)

#define  ClearChanKeeptopic(x)    ((x)->flags &= ~CHSET_KEEPTOPIC)
#define  ClearChanTopiclock(x)    ((x)->flags &= ~CHSET_TOPICLOCK)
#define  ClearChanPrivate(x)      ((x)->flags &= ~CHSET_PRIVATE)
#define  ClearChanRestricted(x)   ((x)->flags &= ~CHSET_RESTRICTED)
#define  ClearChanSecure(x)       ((x)->flags &= ~CHSET_SECURE)
#define  ClearChanSecureops(x)    ((x)->flags &= ~CHSET_SECUREOPS)
#define  ClearChanLeaveops(x)     ((x)->flags &= ~CHSET_LEAVEOPS)
#define  ClearChanVerbose(x)      ((x)->flags &= ~CHSET_VERBOSE)

#define  IsChanKeeptopic(x)    ((x)->flags & CHSET_KEEPTOPIC)
#define  IsChanTopiclock(x)    ((x)->flags & CHSET_TOPICLOCK)
#define  IsChanPrivate(x)      ((x)->flags & CHSET_PRIVATE)
#define  IsChanRestricted(x)   ((x)->flags & CHSET_RESTRICTED)
#define  IsChanSecure(x)       ((x)->flags & CHSET_SECURE)
#define  IsChanSecureops(x)    ((x)->flags & CHSET_SECUREOPS)
#define  IsChanLeaveops(x)     ((x)->flags & CHSET_LEAVEOPS)
#define  IsChanVerbose(x)      ((x)->flags & CHSET_VERBOSE)

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
#define CS_SET_SUCC_SHORT    32
#define CS_SET_SUCC_LONG     33
#define CS_SET_DESC_SHORT    34
#define CS_SET_DESC_LONG     35
#define CS_SET_URL_SHORT     36
#define CS_SET_URL_LONG      37
#define CS_SET_EMAIL_SHORT   38
#define CS_SET_EMAIL_LONG    39
#define CS_SET_ENTRYMSG_SHORT 40
#define CS_SET_ENTRYMSG_LONG  41
#define CS_SET_TOPIC_SHORT    42
#define CS_SET_TOPIC_LONG     43
#define CS_SET_TOPIC          44
#define CS_SET_TOPIC_FAILED   45
#define CS_NOT_EXIST          46
#define CS_INFO_CHAN          47
#define CS_SET_FLAG             48
#define CS_SET_SUCCESS          49
#define CS_SET_FAILED           50
#define CS_SET_KEEPTOPIC_SHORT  51
#define CS_SET_KEEPTOPIC_LONG   52
#define CS_SET_TOPICLOCK_SHORT  53
#define CS_SET_TOPICLOCK_LONG   54
#define CS_SET_PRIVATE_SHORT    55
#define CS_SET_PRIVATE_LONG     56
#define CS_SET_RESTRICTED_SHORT 57
#define CS_SET_RESTRICTED_LONG  58
#define CS_SET_SECURE_SHORT     59
#define CS_SET_SECURE_LONG      60
#define CS_SET_SECUREOPS_SHORT  61
#define CS_SET_SECUREOPS_LONG   62
#define CS_SET_LEAVEOPS_SHORT   63 
#define CS_SET_LEAVEOPS_LONG    64
#define CS_SET_VERBOSE_SHORT    65
#define CS_SET_VERBOSE_LONG     66
#define CS_SET_SUCCESSOR        67
#define CS_SET_DESCRIPTION      68
#define CS_SET_ENTRYMSG         69

#endif

