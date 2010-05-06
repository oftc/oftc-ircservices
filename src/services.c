/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  services.c: Main services functions
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
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

#include "stdinc.h"
#include "conf.h"
#include "conf/conf.h"
#include "ruby_module.h"
#include "python_module.h"
#include "parse.h"
#include "dbm.h"
#include "language.h"
#include "nickname.h"
#include "interface.h"
#include "client.h"
#include "channel_mode.h"
#include "channel.h"
#include "packet.h"
#include "mqueue.h"
#include "send.h"
#include "events.h"
#include "event.h"

#include <signal.h>
#include <sys/wait.h>

struct Client me;
struct ServicesState_t ServicesState = { 0 };
int dorehash = 0;

static struct lgetopt myopts[] = {
  {"configfile", &ServicesState.configfile,
   STRING, "File to use for services.conf"},
  {"logfile",    &ServicesState.logfile,
   STRING, "File to use for services.log"},
  {"pidfile",    &ServicesState.pidfile,
   STRING, "File to use for process ID"},
  {"namesuffix", &ServicesState.namesuffix,
   STRING, "Suffix to use on service names"},
  {"foreground", &ServicesState.foreground,
   YESNO, "Run in foreground (don't detach)"},
  {"version",    &ServicesState.printversion,
   YESNO, "Print version and exit"},
  {"debugmode",  &ServicesState.debugmode,
   YESNO, "Set debug mode and disable all interaction"},
  {"keepmodules", &ServicesState.keepmodules,
   YESNO, "Stops modules from being unloaded.  Helps debugging memory leaks"},
  {"help", NULL, USAGE, "Print this text"},
  {NULL, NULL, STRING, NULL},
};

static void setup_signals();
static void signal_handler(int);
static void sigchld_handler(int);

static void
check_pidfile(const char *filename)
{
#ifndef _WIN32
  FBFILE *fb;
  char buff[32];
  pid_t pidfromfile;

  // Don't do logging here, since we don't have log() initialised
  if ((fb = fbopen(filename, "r")))
  {
    if (fbgets(buff, 20, fb) == NULL)
    {
      /* log(L_ERROR, "Error reading from pid file %s (%s)", filename,
       * strerror(errno));
       */
    }
    else
    {
      pidfromfile = atoi(buff);

      if (!kill(pidfromfile, 0))
      {
        // log(L_ERROR, "Server is already running");
        printf("oftc-ircservices: daemon is already running\n");
        exit(-1);
      }
    }

    fbclose(fb);
  }
  else if (errno != ENOENT)
  {
    // log(L_ERROR, "Error opening pid file %s", filename);
  }
#endif
}


#ifndef _WIN32
/*
 * print_startup - print startup information
 */
static void
print_startup(int pid)
{
  printf("oftc-ircservices: version %s\n", VERSION); 
  printf("oftc-ircservices: pid %d\n", pid);
  printf("oftc-ircservices: running in %s mode from %s\n",
         ServicesState.foreground ? "foreground" : "background", DPATH);
}

static void
make_daemon(void)
{
  int pid;

  if ((pid = fork()) < 0)
  {
    perror("fork");
    exit(EXIT_FAILURE);
  }
  else if (pid > 0)
  {
    print_startup(pid);
    exit(EXIT_SUCCESS);
  }

  setsid();
}
#endif

static void
write_pidfile(const char *filename)
{
  FBFILE *fb;

  if ((fb = fbopen(filename, "w")))
  {
    char buff[32];
    unsigned int pid = (unsigned int)getpid();
    size_t nbytes = ircsprintf(buff, "%u\n", pid);

    if ((fbputs(buff, fb, nbytes) == -1))
      ilog(L_ERROR, "Error writing %u to pid file %s (%s)",
           pid, filename, strerror(errno));

    fbclose(fb);
  }
  else
  {
    ilog(L_ERROR, "Error opening pid file %s", filename);
  }
}

int main(int argc, char *argv[])
{
#ifndef _WIN32
  if (geteuid() == 0)
  {
    fprintf(stderr, "Running IRC services is root is not recommended.");
    return 1;
  }
  setup_corefile();
#endif
  memset(&ServicesInfo, 0, sizeof(ServicesInfo));
  memset(&ServicesState, 0, sizeof(ServicesState));

  ServicesState.configfile = CPATH; 
  ServicesState.logfile    = LPATH;
  ServicesState.pidfile    = PPATH;
  ServicesState.fully_connected = 0;

  parseargs(&argc, &argv, myopts);

  if(ServicesState.printversion)
  {
    printf("oftc-ircservices: version: %s\n", VERSION);
    exit(EXIT_SUCCESS);
  }

  if(chdir(DPATH))
  {
    perror("chdir");
    exit(EXIT_FAILURE);
  }

#ifndef _WIN32
  if(!ServicesState.foreground)
    make_daemon();
  else
    print_startup(getpid());
#endif

  setup_signals();
  memset(&me, 0, sizeof(me));

  libio_init(!ServicesState.foreground);
  init_events();
  iorecv_cb = register_callback("iorecv", iorecv_default);
  connected_cb = register_callback("server connected", server_connected);
  iosend_cb = register_callback("iosend", iosend_default);

  OpenSSL_add_all_digests();
 
  init_interface();
  check_pidfile(ServicesState.pidfile);
  init_log(ServicesState.logfile);

#ifdef HAVE_RUBY
  init_ruby();
  signal(SIGSEGV, SIG_DFL);
#endif

  init_channel();
  init_conf();
  init_client();
  init_parser();
  init_channel_modes();
  init_mqueue();

  me.from = me.servptr = &me;
  SetServer(&me);
  SetMe(&me);
  dlinkAdd(&me, &me.node, &global_client_list);
  
  read_services_conf(TRUE);
  init_db();
  init_uid();
 
#ifdef HAVE_PYTHON
  init_python();
#endif

  write_pidfile(ServicesState.pidfile);
  ilog(L_NOTICE, "Services Ready");

  db_load_driver();
#ifdef USE_SHARED_MODULES
  if(chdir(MODPATH))
  {
    ilog(L_ERROR, "Could not load core modules from %s: %s",
         MODPATH, strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* Go back to DPATH after checking to see if we can chdir to MODPATH */
  chdir(DPATH);
#else
  load_all_modules(1);
#endif

  boot_modules(1);
  
  connect_server();

  for(;;)
  {
    while (eventNextTime() <= CurrentTime)
      eventRun();

    execute_callback(do_event_cb);

    if(events_loop() == -1)
    {
      ilog(L_CRIT, "libevent returned error %d", errno);
      services_die("Libevent returned some sort of error", NO);
      break;
    }

    comm_select();
    send_queued_all();

    if(dorehash)
    {
      ilog(L_INFO, "Got SIGHUP, reloading configuration");
      read_services_conf(NO);
      dorehash = 0;
    }
  }

  return 0;
}

static void
setup_signals()
{
 struct sigaction act;

  act.sa_flags = 0;
  act.sa_handler = SIG_IGN;

  sigemptyset(&act.sa_mask);

  sigaddset(&act.sa_mask, SIGPIPE);
  sigaction(SIGPIPE, &act, 0);

  sigaddset(&act.sa_mask, SIGALRM);
  sigaction(SIGALRM, &act, 0);

  act.sa_handler = signal_handler;
  sigemptyset(&act.sa_mask);

  sigaddset(&act.sa_mask, SIGHUP);
  sigaction(SIGHUP, &act, 0);

  sigaddset(&act.sa_mask, SIGINT);
  sigaction(SIGINT, &act, 0);

  sigaddset(&act.sa_mask, SIGTERM);
  sigaction(SIGTERM, &act, 0);

  sigaddset(&act.sa_mask, SIGUSR1);
  sigaction(SIGUSR1, &act, 0);

  act.sa_handler = sigchld_handler;
  sigaddset(&act.sa_mask, SIGCHLD);
  sigaction(SIGCHLD, &act, 0);
}

void
services_die(const char *msg, int rboot)
{
  ilog(L_NOTICE, "Dying: %s", msg);

  cleanup_channel_modes();
  cleanup_conf();
#ifdef HAVE_RUBY
  cleanup_ruby();
#endif
  cleanup_db();
  cleanup_modules();

  EVP_cleanup();

  send_queued_all();
  exit_client(&me, &me, "Services shutting down");
  send_queued_all();

  if(me.uplink != NULL)
    MyFree(me.uplink->server);
 
  cleanup_client();
  cleanup_channel();
  cleanup_interface();
  cleanup_mqueue();
  unregister_callback(iorecv_cb);
  unregister_callback(connected_cb);
  unregister_callback(iosend_cb);
  libio_cleanup();
  exit(rboot);
}

static void
sigchld_handler(int sig)
{
  int status;
  waitpid(-1, &status, WNOHANG);
}

static void
signal_handler(int signum)
{
  ilog(L_DEBUG, "Got signal %d!", signum);
  switch(signum)
  {
    case SIGTERM:
      services_die("got SIGTERM", NO);
      break;
    case SIGINT:
      services_die("got SIGINT", NO);
      break;
    case SIGHUP:
      dorehash = 1;
      break;
    case SIGUSR1:
      break;
  }
}
