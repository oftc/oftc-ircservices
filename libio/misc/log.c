/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  s_log.c: Logger functions.
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id$
 */

#include "libioinc.h"

#ifdef USE_SYSLOG
# ifdef HAVE_SYS_SYSLOG_H
#  include <sys/syslog.h>
# else
#  ifdef HAVE_SYSLOG_H
#   include <syslog.h>
#  endif
# endif
#endif

struct Service;
void global_notice(struct Service *, char *, ...);

/* some older syslogs would overflow at 2024 */
#define LOG_BUFSIZE 2000

static FBFILE *logFile = NULL;
static int gnotice_logLevel = INIT_LOG_LEVEL;
static int file_logLevel = INIT_LOG_LEVEL;

#ifndef SYSLOG_USERS
static EVH user_log_resync;
void *user_log_fb = NULL;
#endif

#ifdef USE_SYSLOG
static const int sysLogLevel[] =
{
  LOG_CRIT,
  LOG_ERR,
  LOG_WARNING,
  LOG_NOTICE,
  LOG_INFO,
  LOG_INFO,
  LOG_INFO
};
#endif

static const char *logLevelToString[] =
{
  "L_CRIT",
  "L_ERROR",
  "L_WARN",
  "L_NOTICE",
  "L_INFO",
  "L_DEBUG",
  "L_TRACE"
};

/*
 * open_log - open ircd logging file
 * returns true (1) if successful, false (0) otherwise
 */
static int 
open_log(const char *filename)
{
  logFile = fbopen(filename, "a");

  if (logFile == NULL)
  {
#ifdef USE_SYSLOG
    syslog(LOG_ERR, "Unable to open log file: %s: %s",
           filename, strerror(errno));
#endif
    return (0);
  }

  return (1);
}

static void 
write_log(const char *message)
{
  char buf[LOG_BUFSIZE];
  size_t nbytes = 0;

  if (logFile == NULL)
    return;

#ifdef _WIN32
  nbytes = snprintf(buf, sizeof(buf), "[%s] %s\r\n",
#else
  nbytes = snprintf(buf, sizeof(buf), "[%s] %s\n",
#endif
                    smalldate(CurrentTime), message);
  fbputs(buf, logFile, nbytes);
}
   
void
ilog(const int priority, const char *fmt, ...)
{
  char buf[LOG_BUFSIZE];
  va_list args;

  assert(priority > -1);

  if (fmt == NULL)
    return;

  if (priority > file_logLevel && priority > gnotice_logLevel)
    return;

  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);

#ifdef USE_SYSLOG  
  if (priority <= L_DEBUG)
    syslog(sysLogLevel[priority], "%s", buf);
#endif
  if(priority <= file_logLevel)
    write_log(buf);

  if(priority <= gnotice_logLevel)
    global_notice(NULL, buf);
}
  
void
init_log(const char *filename)
{
  open_log(filename);
#ifdef USE_SYSLOG
  openlog("ircd", LOG_PID | LOG_NDELAY, LOG_FACILITY);
#endif
#ifndef SYSLOG_USERS
  eventAddIsh("user_log_resync", user_log_resync, NULL, 60);
#endif
}

void
cleanup_log()
{
  if(logFile != NULL)
    fbclose(logFile);
}

void
reopen_log(const char *filename)
{
  if (logFile != NULL)
    fbclose(logFile);
  open_log(filename);
}

void
set_gnotice_log_level(const int level)
{
  if (L_ERROR < level && level <= L_DEBUG)
    gnotice_logLevel = level;
}

void
set_file_log_level(const int level)
{
  if (L_ERROR < level && level <= L_DEBUG)
    file_logLevel = level;
}

int
get_gnotice_log_level(void)
{
  return(gnotice_logLevel);
}

int
get_file_log_level(void)
{
  return(file_logLevel);
}
  
const char *
get_log_level_as_string(int level)
{
  if (level > L_DEBUG)
    level = L_DEBUG;
  else if (level < L_ERROR)
    level = L_ERROR;

  return(logLevelToString[level]);
}

#ifndef SYSLOG_USERS
/* user_log_resync()
 *
 * inputs	- NONE
 * output	- NONE
 * side effects	-
 */
static void
user_log_resync(void *notused)
{
  if (user_log_fb != NULL)
  {
    fbclose(user_log_fb);
    user_log_fb = NULL;
  }
}
#endif
