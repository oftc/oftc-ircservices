/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  s_misc.c: Yet another miscellaneous functions file.
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

#define IN_MISC_C
#include "libioinc.h"
#include <sys/time.h>
#include <time.h>
#include "../comm/rlimits.h"

struct timeval SystemTime;

static const char *months[] =
{
  "January",   "February", "March",   "April",
  "May",       "June",     "July",    "August",
  "September", "October",  "November","December"
};

static const char *weekdays[] =
{
  "Sunday",   "Monday", "Tuesday", "Wednesday",
  "Thursday", "Friday", "Saturday"
};

char *
date(time_t lclock) 
{
  static char buf[80], plus;
  struct tm *lt, *gm;
  struct tm gmbuf;
  int minswest;

  if (!lclock) 
    lclock = CurrentTime;
  gm = gmtime(&lclock);
  memcpy(&gmbuf, gm, sizeof(gmbuf));
  gm = &gmbuf;
  lt = localtime(&lclock);

  /*
   * There is unfortunately no clean portable way to extract time zone
   * offset information, so do ugly things.
   */
  minswest = (gm->tm_hour - lt->tm_hour) * 60 + (gm->tm_min - lt->tm_min);

  if (lt->tm_yday != gm->tm_yday)
  {
    if ((lt->tm_yday > gm->tm_yday && lt->tm_year == gm->tm_year) ||
        (lt->tm_yday < gm->tm_yday && lt->tm_year != gm->tm_year))
      minswest -= 24 * 60;
    else
      minswest += 24 * 60;
  }

  plus = (minswest > 0) ? '-' : '+';
  if (minswest < 0)
    minswest = -minswest;

  ircsprintf(buf, "%s %s %d %d -- %02u:%02u:%02u %c%02u:%02u",
             weekdays[lt->tm_wday], months[lt->tm_mon],lt->tm_mday,
             lt->tm_year + 1900, lt->tm_hour, lt->tm_min, lt->tm_sec,
             plus, minswest/60, minswest%60);
  return buf;
}

void
date_diff(time_t end, time_t start, struct tm *result)
{
  time_t diff = end - start;

  gmtime_r(&diff, result);

  result->tm_year = result->tm_year - 70;
}

const char *
smalldate(time_t lclock)
{
  static char buf[MAX_DATE_STRING];
  struct tm lt, gm;

  if (!lclock)
    lclock = CurrentTime;

  gmtime_r(&lclock, &gm);
  localtime_r(&lclock, &lt);
  
  ircsprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
             lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
             lt.tm_hour, lt.tm_min, lt.tm_sec);

  return buf;
}

/* small_file_date()
 * Make a small YYYYMMDD formatted string suitable for a
 * dated file stamp. 
 */
char *
small_file_date(time_t lclock)
{
  static char timebuffer[MAX_DATE_STRING];
  struct tm *tmptr;

  if (!lclock)
    time(&lclock);

  tmptr = localtime(&lclock);
  strftime(timebuffer, MAX_DATE_STRING, "%Y%m%d", tmptr);

  return timebuffer;
}


#ifdef _WIN32
/* Copyright (C) 2001 Free Software Foundation, Inc.
 *
 * Get name and information about current kernel.
 */
int
uname(struct utsname *uts)
{
  enum { WinNT, Win95, Win98, WinUnknown };
  OSVERSIONINFO osver;
  SYSTEM_INFO sysinfo;
  DWORD sLength;
  DWORD os = WinUnknown;

  memset(uts, 0, sizeof(*uts));

  osver.dwOSVersionInfoSize = sizeof(osver);
  GetVersionEx(&osver);
  GetSystemInfo(&sysinfo);

  switch (osver.dwPlatformId)
  {
    case VER_PLATFORM_WIN32_NT: /* NT, Windows 2000 or Windows XP */
      if (osver.dwMajorVersion == 4)
        strcpy (uts->sysname, "Windows NT4x");    /* NT4x */
      else if (osver.dwMajorVersion <= 3)
        strcpy (uts->sysname, "Windows NT3x");    /* NT3x */
      else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion  < 1)
        strcpy (uts->sysname, "Windows 2000");    /* 2k */
      else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2)
        strcpy (uts->sysname, "Windows 2003");    /* 2003 */
      else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 1)
        strcpy (uts->sysname, "Windows XP");      /* XP */
      else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0)
        strcpy (uts->sysname, "Windows Vista");   /* Vista */
      os = WinNT;
      break;

    case VER_PLATFORM_WIN32_WINDOWS: /* Win95, Win98 or WinME */
      if ((osver.dwMajorVersion > 4) ||
          ((osver.dwMajorVersion == 4) && (osver.dwMinorVersion > 0)))
      {
        if (osver.dwMinorVersion >= 90)
          strcpy (uts->sysname, "Windows ME"); /* ME */
        else
          strcpy (uts->sysname, "Windows 98"); /* 98 */
        os = Win98;
      }
      else
      {
        strcpy (uts->sysname, "Windows 95"); /* 95 */
        os = Win95;
      }
      break;

    case VER_PLATFORM_WIN32s: /* Windows 3.x */
      strcpy (uts->sysname, "Windows");
      break;
  }

  sprintf(uts->version, "%ld.%02ld",
          osver.dwMajorVersion, osver.dwMinorVersion);

  if (osver.szCSDVersion[0] != '\0' &&
      (strlen (osver.szCSDVersion) + strlen (uts->version) + 1) <
      sizeof (uts->version))
    {
      strcat (uts->version, " ");
      strcat (uts->version, osver.szCSDVersion);
    }

  sprintf (uts->release, "build %ld", osver.dwBuildNumber & 0xFFFF);

  switch (sysinfo.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_PPC:
      strcpy (uts->machine, "ppc");
      break;
    case PROCESSOR_ARCHITECTURE_ALPHA:
      strcpy (uts->machine, "alpha");
      break;
    case PROCESSOR_ARCHITECTURE_MIPS:
      strcpy (uts->machine, "mips");
      break;
    case PROCESSOR_ARCHITECTURE_INTEL:
      /*
       * dwProcessorType is only valid in Win95 and Win98 and WinME
       * wProcessorLevel is only valid in WinNT
       */
      switch (os)
      {
        case Win95:
        case Win98:
          switch (sysinfo.dwProcessorType)
          {
            case PROCESSOR_INTEL_386:
            case PROCESSOR_INTEL_486:
            case PROCESSOR_INTEL_PENTIUM:
              sprintf(uts->machine, "i%ld", sysinfo.dwProcessorType);
              break;
            default:
              strcpy(uts->machine, "i386");
              break;
          }
          break;
        case WinNT:
          switch(sysinfo.wProcessorArchitecture)
            {
            case PROCESSOR_ARCHITECTURE_INTEL:
              sprintf (uts->machine, "x86(%d)", sysinfo.wProcessorLevel);
              break;
            case PROCESSOR_ARCHITECTURE_IA64:
              sprintf (uts->machine, "ia64(%d)", sysinfo.wProcessorLevel);
              break;
#ifdef PROCESSOR_ARCHITECTURE_AMD64
            case PROCESSOR_ARCHITECTURE_AMD64:
              sprintf (uts->machine, "x86_64(%d)", sysinfo.wProcessorLevel);
              break;
#endif
            default:
              sprintf (uts->machine, "unknown(%d)", sysinfo.wProcessorLevel);
              break;
            }
          break;
        default:
          strcpy(uts->machine, "unknown");
      }
      break;
    default:
      strcpy (uts->machine, "unknown");
      break;
  }

  sLength = sizeof(uts->nodename) - 1;
  GetComputerName(uts->nodename, &sLength);
  return 0;
}
#endif

#ifdef HAVE_LIBCRYPTO
char *
ssl_get_cipher(SSL *ssl)
{
  static char buffer[128];
  const char *name = NULL;
  int bits;

  switch (SSL_version(ssl))
  {
    case SSL2_VERSION:
      name = "SSLv2";
      break;

    case SSL3_VERSION:
      name = "SSLv3";
      break;

    case TLS1_VERSION:
      name = "TLSv1";
      break;

    default:
      name = "UNKNOWN";
  }

  SSL_CIPHER_get_bits(SSL_get_current_cipher(ssl), &bits);

  snprintf(buffer, sizeof(buffer), "%s %s-%d",
           name, SSL_get_cipher(ssl), bits);
  
  return buffer;
}
#endif

void
set_time(void)
{
  struct timeval newtime;
#ifdef _WIN32
  FILETIME ft;

  GetSystemTimeAsFileTime(&ft);
  if (ft.dwLowDateTime < 0xd53e8000)
    ft.dwHighDateTime--;
  ft.dwLowDateTime -= 0xd53e8000;
  ft.dwHighDateTime -= 0x19db1de;

  newtime.tv_sec  = (*(uint64_t *) &ft) / 10000000;
  newtime.tv_usec = (*(uint64_t *) &ft) / 10 % 1000000;
#else
  newtime.tv_sec  = 0;
  newtime.tv_usec = 0;
  gettimeofday(&newtime, NULL);
#endif

  if (newtime.tv_sec < CurrentTime)
  {
    ilog(L_CRIT, "System clock is running backwards - (%lu < %lu)",
         (unsigned long)newtime.tv_sec, (unsigned long)CurrentTime);
    set_back_events(CurrentTime - newtime.tv_sec);
  }

  SystemTime.tv_sec  = newtime.tv_sec;
  SystemTime.tv_usec = newtime.tv_usec;
}

/* setup_corefile()
 *
 * inputs       - nothing
 * output       - nothing
 * side effects - setups corefile to system limits.
 * -kre
 */
void
setup_corefile(void)
{
#ifdef HAVE_SYS_RESOURCE_H
  struct rlimit rlim; /* resource limits */

  /* Set corefilesize to maximum */
  if (!getrlimit(RLIMIT_CORE, &rlim))
  {
    rlim.rlim_cur = rlim.rlim_max;
    setrlimit(RLIMIT_CORE, &rlim);
  }
#endif
}

void
libio_init(int daemonn)
{
#ifndef _WIN32
  if (daemonn)
    close_standard_fds();
#endif

  /* It ain't random, but it ought to be a little harder to guess */
  srand(SystemTime.tv_sec ^ (SystemTime.tv_usec | (getpid() << 20)));

  set_time();
  eventInit();
#if USE_BLOCK_ALLOC
  initBlockHeap();
#endif
  init_dlink_nodes();
  dbuf_init();
  fdlist_init();
  init_comm();
#ifndef _WIN32
 init_resolver();
#endif
}

void
libio_cleanup()
{
  fdlist_cleanup();
  cleanup_comm();
  dbuf_cleanup();
  cleanup_dlink_nodes();
  cleanup_log();
}
