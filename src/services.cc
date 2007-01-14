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

#include <string>
#include <vector>
#include <sstream>
#include <tr1/unordered_map>

#include "stdinc.h"
#include "language.h"
#include "interface.h"
#include "connection.h"
#include "client.h"
#include "parse.h"
#include "dbm.h"

#include "conf.h"
#include "conf/conf.h"
#include "lua_module.h"
#include "ruby_module.h"
#include "module.h"

#include <signal.h>
#include <sys/wait.h>

Server *me;

static void setup_signals();
static void signal_handler(int);
static void sigchld_handler(int);

int main(int argc, char *argv[])
{
  Parser *parser = new Parser();
  Connection *connection = new Connection(parser);
  DBM *dbm = new DBM();

  me = new Server();

  memset(&ServicesInfo, 0, sizeof(ServicesInfo));

  //connected_cb = register_callback("server connected", server_connected);
  //iosend_cb = register_callback("iosend", iosend_default);

  OpenSSL_add_all_digests();
 
  init_interface();
  strcpy(ServicesInfo.logfile, "services.log");
  libio_init(FALSE);
  init_log(ServicesInfo.logfile);
  //init_channel();
  init_conf();
  init_module();
  //init_client();
  //init_parser();
  //init_channel_modes();

  read_services_conf(TRUE);

  dbm->connect();

#ifdef USE_SHARED_MODULES
  if(chdir(MODPATH))
  {
    ilog(L_DEBUG, "Could not load core modules. Terminating!");
    exit(EXIT_FAILURE);
  }

  //init_lua();
//  init_perl();
  //init_ruby();

  boot_modules(1);
  /* Go back to DPATH after checking to see if we can chdir to MODPATH */
  chdir(DPATH);
#else
  load_all_modules(1);
#endif
  
  setup_signals();

  connection->connect();

  for(;;)
  {
    while (eventNextTime() <= CurrentTime)
      eventRun();

    comm_select();
    connection->process_send_queue();
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

  /*cleanup_channel();
  cleanup_conf();
  cleanup_client();
  cleanup_parser();
  cleanup_channel_modes();
  cleanup_log();*/
  //cleanup_ruby();
  cleanup_module();
  //cleanup_interface();

  EVP_cleanup();

//  exit_client(me, me, "Services shutting down");

  //send_queued_all();
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
