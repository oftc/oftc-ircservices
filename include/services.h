/* TODO: add copyright block */

#ifndef INCLUDED_services_h
#define INCLUDED_services_h

void services_die(const char *, int);

#include "services-lang.h"

struct ServicesState_t
{
  char *configfile;
  char *logfile;
  char *dblogfile;
  char *pidfile;
  char *namesuffix;
  int foreground;
  int printversion;
  int debugmode;
  int keepmodules;
  int fully_connected;
};

EXTERN struct ServicesState_t ServicesState;
EXTERN int dorehash;

#endif /* INCLUDED_services_h */
