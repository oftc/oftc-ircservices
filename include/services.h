/* TODO: add copyright block */

#ifndef INCLUDED_services_h
#define INCLUDED_services_h

void services_die(const char *, int);

#define SERV_HELP_NOT_AVAIL 1
#define SERV_SUB_HELP_NOT_AVIL 2
#define SERV_HELP_SHORT 3
#define SERV_UNKNOWN_CMD 4
#define SERV_INSUFF_PARAM 5
#define SERV_NOT_IDENTIFIED 6
#define SERV_ACCESS_DENIED 7
#define SERV_UNREG_CHAN 8
#define SERV_NO_ACCESS_CHAN 9
#define SERV_NO_ACCESS 10
#define SERV_UNKNOWN_OPTION 11

struct ServicesState_t
{
  char *configfile;
  char *logfile;
  char *dblogfile;
  char *pidfile;
  char *namesuffix;
  int foreground;
  int printversion;
};

EXTERN struct ServicesState_t ServicesState;
EXTERN int dorehash;

#endif /* INCLUDED_services_h */
