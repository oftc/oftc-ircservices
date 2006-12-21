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
#include "lua_module.h"
#include "ruby_module.h"

#include <signal.h>
#include <sys/wait.h>

struct Client me;

static void setup_signals();
static void signal_handler(int);
static void sigchld_handler(int);

int main(int argc, char *argv[])
{
  memset(&ServicesInfo, 0, sizeof(ServicesInfo));

  memset(&me, 0, sizeof(me));

  iorecv_cb = register_callback("iorecv", iorecv_default);
  connected_cb = register_callback("server connected", server_connected);
  iosend_cb = register_callback("iosend", iosend_default);
      
  init_interface();
  strcpy(ServicesInfo.logfile, "services.log");
  libio_init(FALSE);
  init_channel();
  init_conf();
  init_client();
  init_parser();
  init_channel_modes();

  read_services_conf(TRUE);
  init_db();

  init_log(ServicesInfo.logfile);

  ilog(L_DEBUG, "Services starting with name %s description %s sid %s",
      me.name, me.info, me.id);

  me.from = me.servptr = &me;
  SetServer(&me);
  dlinkAdd(&me, &me.node, &global_client_list);
  hash_add_client(&me);

  SetMe(&me);

  db_load_driver();
#ifndef STATIC_MODULES
  if(chdir(MODPATH))
  {
    ilog(L_DEBUG, "Could not load core modules. Terminating!");
    exit(EXIT_FAILURE);
  }

  init_lua();
//  init_perl();
  init_ruby();

  boot_modules(1);
  /* Go back to DPATH after checking to see if we can chdir to MODPATH */
  chdir(DPATH);
#else
  load_all_modules(1);
#endif
  
  setup_signals();

  connect_server();

  for(;;)
  {
    while (eventNextTime() <= CurrentTime)
      eventRun();

    comm_select();
    send_queued_all();
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

  act.sa_handler = sigchld_handler;
  sigaddset(&act.sa_mask, SIGCHLD);
  sigaction(SIGCHLD, &act, 0);
}

void
services_die(const char *msg, int rboot)
{
  ilog(L_NOTICE, "Dying: %s", msg);

/*  cleanup_interface();
  cleanup_channel();
  cleanup_conf();
  cleanup_client();
  cleanup_parser();
  cleanup_channel_modes();
  cleanup_log();*/
  cleanup_db();

  send_queued_all();
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
  }
}
