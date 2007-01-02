/* TODO: add copyright block */

#ifndef INCLUDED_nickserv_h
#define INCLUDED_nickserv_h

struct Nick
{
  dlink_node node;
  
  unsigned int id;
  char nick[NICKLEN+1];
  char pass[PASSLEN+1];
  char salt[SALTLEN+1];
  char cloak[HOSTLEN+1];
  char *email;
  char *url;
  char *last_realname;
  char *last_host;
  char *last_quit;
  unsigned int status;
  unsigned char enforce;
  unsigned char secure;
  unsigned char verified;
  unsigned char cloak_on;
  unsigned char admin;
  unsigned char email_verified;
  unsigned int language;
  time_t reg_time;
  time_t last_seen;
  time_t last_quit_time;
  time_t nick_reg_time;
};

/* Language defines */

#define NS_HELP_SHORT 1
#define NS_HELP_LONG 2
#define NS_HELP_REG_SHORT 3
#define NS_HELP_REG_LONG 4
#define NS_HELP_ID_SHORT 5
#define NS_HELP_ID_LONG 6
#define NS_HELP_DROP_SHORT 7
#define NS_HELP_DROP_LONG 8
#define NS_HELP_ACCESS_SHORT 9
#define NS_HELP_ACCESS_LONG 10
#define NS_HELP_ACCESS_ADD_SHORT 11
#define NS_HELP_ACCESS_ADD_LONG 12
#define NS_HELP_ACCESS_LIST_SHORT 13
#define NS_HELP_ACCESS_LIST_LONG 14
#define NS_HELP_ACCESS_DEL_SHORT 15
#define NS_HELP_ACCESS_DEL_LONG 16
#define NS_HELP_SET_SHORT 17
#define NS_HELP_SET_LONG 18
#define NS_HELP_SET_LANG_SHORT 19
#define NS_HELP_SET_LANG_LONG 20
#define NS_HELP_SET_PASS_SHORT 21
#define NS_HELP_SET_PASS_LONG 22
#define NS_HELP_SET_URL_SHORT 23
#define NS_HELP_SET_URL_LONG 24
#define NS_HELP_SET_EMAIL_SHORT 25
#define NS_HELP_SET_EMAIL_LONG 26
#define NS_HELP_SET_ENFORCE_SHORT 27
#define NS_HELP_SET_ENFORCE_LONG 28
#define NS_HELP_SET_SECURE_SHORT 29
#define NS_HELP_SET_SECURE_LONG 30
#define NS_HELP_SET_CLOAK_SHORT 31
#define NS_HELP_SET_CLOAK_LONG 32
#define NS_HELP_SET_CLOAKSTRING_SHORT 33
#define NS_HELP_SET_CLOAKSTRING_LONG 34
#define NS_HELP_SET_MASTER_SHORT 35
#define NS_HELP_SET_MASTER_LONG 36
#define NS_HELP_GHOST_SHORT 37
#define NS_HELP_GHOST_LONG 38
#define NS_HELP_LINK_SHORT 39
#define NS_HELP_LINK_LONG 40
#define NS_HELP_UNLINK_SHORT 41
#define NS_HELP_UNLINK_LONG 42
#define NS_HELP_RECOVER_SHORT 43
#define NS_HELP_RECOVER_LONG 44
#define NS_HELP_REGAIN_SHORT 45
#define NS_HELP_REGAIN_LONG 46
#define NS_HELP_SUSPEND_SHORT 47
#define NS_HELP_SUSPEND_LONG 48
#define NS_HELP_FORBID_SHORT 49
#define NS_HELP_FORBID_LONG 50
#define NS_HELP_INFO_SHORT 51
#define NS_HELP_INFO_LONG 52
#define NS_HELP_SENDPASS_SHORT 53
#define NS_HELP_SENDPASS_LONG 54
#define NS_ALREADY_REG 55
#define NS_REG_COMPLETE 56
#define NS_REG_FAIL 57
#define NS_REG_FIRST 58
#define NS_IDENTIFIED 59
#define NS_IDENT_FAIL 60
#define NS_CURR_LANGUAGE 61
#define NS_AVAIL_LANGUAGE 62
#define NS_LANGUAGE_SET 63
#define NS_LANGUAGE_UNAVAIL 64
#define NS_SET_SUCCESS 65
#define NS_SET_FAILED 66
#define NS_SET_VALUE 67
#define NS_NEED_IDENTIFY 68
#define NS_NICK_DROPPED 69
#define NS_NICK_DROPFAIL 70
#define NS_ACCESS_ADD 71
#define NS_ACCESS_ADDFAIL 72
#define NS_ACCESS_INVALID 73
#define NS_ACCESS_START 74
#define NS_ACCESS_ENTRY 75
#define NS_ACCESS_DEL 76
#define NS_GHOST_SUCCESS 77
#define NS_GHOST_FAILED 78
#define NS_GHOST_NOSELF 79
#define NS_GHOST_NOTONLINE 80
#define NS_LINK_NOMASTER 81
#define NS_LINK_BADPASS 82
#define NS_LINK_NOSELF 83
#define NS_LINK_FAIL 84
#define NS_LINK_OK 85
#define NS_UNLINK_FAILED 86
#define NS_UNLINK_OK 87
#define NS_UNLINK_NOLINK 88
#define NS_INFO 89
#define NS_INFO_OPTION 90
#define NS_NICK_IN_USE 91
#define NS_NICK_IN_USE_IWILLCHANGE 92
#define NS_NICK_AUTOID 93
#define NS_NICK_FORBID_IWILLCHANGE 94
#define NS_FORBID_OK 95
#define NS_FORBID_FAIL 96
#define NS_NOREG_GUEST 97
#define NS_NOREG_FORBID 98
#define NS_LANG_LIST 99
#define NS_MASTER_NOT_LINKED 100
#define NS_MASTER_SET_OK 101
#define NS_MASTER_SET_FAIL 102
#define NS_UNFORBID_NOT_FORBID 103
#define NS_UNFORBID_OK 104
#define NS_UNFORBID_FAIL 105
#define NS_REGAINED 106
#define NS_REGAIN_FAIL 107

#endif /* INCLUDED_nickserv_h */
