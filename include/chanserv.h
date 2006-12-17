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
  char forbidden;
  char priv;
  char restricted_ops;
  char topic_lock;
  char secure;
  char verbose;
};

struct AKick
{
  unsigned int id;
  char *channel;
  unsigned int target;
  unsigned int setter;
  char *mask;
  char *reason;
  time_t time_set;
  time_t duration;
};

#define CHACCESS_BAN        0x0001
#define CHACCESS_AUTODEOP   0x0002
#define CHACCESS_VOICE      0x0004
#define CHACCESS_OP         0x0008
#define CHACCESS_INVITE     0x0010
#define CHACCESS_UNBAN      0x0020
#define CHACCESS_AKICK      0x0040
#define CHACCESS_CLEAR      0x0080
#define CHACCESS_SET        0x0100
#define CHACCESS_ACCESS     0x0200

#define CHACCESS_AUTOVOICE  0x1000
#define CHACCESS_AUTOOP     0x2000

// FIXME i didnt come up with a good name.  sorry -mc
struct CHACCESS_LALA {
  char *name;
  int  level;
};

#define CS_HELP_REG_SHORT 1
#define CS_HELP_REG_LONG 2
#define CS_HELP_SHORT 3
#define CS_HELP_LONG 4
#define CS_HELP_SET_FOUNDER_SHORT 5
#define CS_HELP_SET_FOUNDER_LONG 6
#define CS_HELP_SET_SUCC_SHORT 7
#define CS_HELP_SET_SUCC_LONG 8
#define CS_HELP_SET_DESC_SHORT 9
#define CS_HELP_SET_DESC_LONG 10
#define CS_HELP_SET_URL_SHORT 11
#define CS_HELP_SET_URL_LONG 12
#define CS_HELP_SET_EMAIL_SHORT 13
#define CS_HELP_SET_EMAIL_LONG 14
#define CS_HELP_SET_ENTRYMSG_SHORT 15
#define CS_HELP_SET_ENTRYMSG_LONG 16
#define CS_HELP_SET_TOPIC_SHORT 17
#define CS_HELP_SET_TOPIC_LONG 18
#define CS_HELP_SET_TOPICLOCK_SHORT 19
#define CS_HELP_SET_TOPICLOCK_LONG 20
#define CS_HELP_SET_PRIVATE_SHORT 21
#define CS_HELP_SET_PRIVATE_LONG 22
#define CS_HELP_SET_RESTRICTED_SHORT 23
#define CS_HELP_SET_RESTRICTED_LONG 24
#define CS_HELP_SET_SECURE_SHORT 25
#define CS_HELP_SET_SECURE_LONG 26
#define CS_HELP_SET_VERBOSE_SHORT 27
#define CS_HELP_SET_VERBOSE_LONG 28
#define CS_HELP_SET_MLOCK_SHORT 29
#define CS_HELP_SET_MLOCK_LONG 30
#define CS_HELP_SET_AUTOLIMIT_SHORT 31
#define CS_HELP_SET_AUTOLIMIT_LONG 32
#define CS_HELP_SET_CLEARBANS_SHORT 33
#define CS_HELP_SET_CLEARBANS_LONG 34
#define CS_HELP_SET_SHORT 35
#define CS_HELP_SET_LONG 36
#define CS_HELP_AKICK_ADD_SHORT 37
#define CS_HELP_AKICK_ADD_LONG 38
#define CS_HELP_AKICK_DEL_SHORT 39
#define CS_HELP_AKICK_DEL_LONG 40
#define CS_HELP_AKICK_LIST_SHORT 41
#define CS_HELP_AKICK_LIST_LONG 42
#define CS_HELP_AKICK_VIEW_SHORT 43
#define CS_HELP_AKICK_VIEW_LONG 44
#define CS_HELP_AKICK_ENFORCE_SHORT 45
#define CS_HELP_AKICK_ENFORCE_LONG 46
#define CS_HELP_AKICK_COUNT_SHORT 47
#define CS_HELP_AKICK_COUNT_LONG 48
#define CS_HELP_AKICK_SHORT 49
#define CS_HELP_AKICK_LONG 50
#define CS_HELP_DROP_SHORT 51
#define CS_HELP_DROP_LONG 52
#define CS_HELP_INFO_SHORT 53
#define CS_HELP_INFO_LONG 54
#define CS_HELP_OP_SHORT 55
#define CS_HELP_OP_LONG 56
#define CS_HELP_UNBAN_SHORT 57
#define CS_HELP_UNBAN_LONG 58
#define CS_HELP_INVITE_SHORT 59
#define CS_HELP_INVITE_LONG 60
#define CS_HELP_CLEAR_SHORT 61
#define CS_HELP_CLEAR_LONG 62
#define CS_REGISTER_NICK 63
#define CS_ALREADY_REG 64
#define CS_REG_SUCCESS 65
#define CS_REG_FAIL 66
#define CS_NAMESTART_HASH 67
#define CS_NOT_ONCHAN 68
#define CS_NOT_OPPED 69
#define CS_NOT_REG 70
#define CS_OWN_CHANNEL_ONLY 71
#define CS_DROPPED 72
#define CS_DROP_FAILED 73
#define CS_SET_SUCCESSOR 74
#define CS_SET_DESCRIPTION 75
#define CS_SET_ENTRYMSG 76
#define CS_SET_FOUNDER 77
#define CS_SET_FOUNDER_FAILED 78
#define CS_SET_SUCC 79
#define CS_SET_SUCC_FAILED 80
#define CS_SET_DESC 81
#define CS_SET_DESC_FAILED 82
#define CS_SET_URL 83
#define CS_SET_URL_FAILED 84
#define CS_SET_EMAIL 85
#define CS_SET_EMAIL_FAILED 86
#define CS_SET_MSG 87
#define CS_SET_MSG_FAILED 88
#define CS_SET_TOPIC 89
#define CS_SET_TOPIC_FAILED 90
#define CS_SET_FLAG 91
#define CS_SET_SUCCESS 92
#define CS_SET_FAILED 93
#define CS_NOT_EXIST 94
#define CS_INFO_CHAN 95
#define CS_AKICK_NONICK 96
#define CS_AKICK_ADDED 97
#define CS_AKICK_ADDFAIL 98
#define CS_AKICK_LIST 99
#define CS_AKICK_LISTEND 100

#endif
