/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  events.c: event processing
 *
 *  Copyright (C) 2010 Stuart Walsh and the OFTC Coding department
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
 *  $Id: services.c 1442 2008-04-13 20:28:24Z swalsh $
 */

#include "stdinc.h"
#include "conf.h"
#include "conf/conf.h"
#include <event.h>
#include <evdns.h>
#include <signal.h>

struct event_base *ev_base = NULL;

/*static void
sigint_callback(int signal, short event, void *arg)
{
//  struct event *sigint = (struct event *)arg;

  exit(EXIT_SUCCESS);
}*/

static void
timer_callback(int fd, short event, void *arg)
{
}

static void
libevent_log_cb(int severity, const char *msg)
{
  ilog(L_DEBUG, "{libevent} %d: %s", severity, msg);
}

int
init_events()
{
  struct timeval tv;
  struct event *timer = MyMalloc(sizeof(struct event));
//  struct event *sigint = MyMalloc(sizeof(struct event));

  ev_base = event_init();

  event_set_log_callback(&libevent_log_cb);

  if(evdns_init() == -1)
  {
    ilog(L_ERROR, "libevent dns init failed");
    return FALSE;
  }

  ilog(L_DEBUG, "libevent init %p", ev_base);

  memset(&tv, 0, sizeof(tv));

  tv.tv_usec = 100;
  event_set(timer, -1, EV_PERSIST, timer_callback, timer);
  event_base_set(ev_base, timer);
  evtimer_add(timer, &tv);

/*  event_set(sigint, SIGINT, EV_SIGNAL|EV_PERSIST, sigint_callback, sigint);
  event_add(sigint, NULL);*/

  return TRUE;
}

int
events_loop()
{
  //ilog(L_DEBUG, "ev_base %p", ev_base);
  return event_base_loop(ev_base, EVLOOP_NONBLOCK);
}

struct event *
events_setup(int fd, short events, void(*cb)(int, short, void *), void *arg)
{
  struct event *ev;

  ilog(L_DEBUG, "Adding event for %d (%d %p %p)", fd, events, cb, arg);
  ev = MyMalloc(sizeof(struct event));
  if(ev == NULL)
    return NULL;

  event_set(ev, fd, events, cb, arg);
  event_base_set(ev_base, ev);

  return ev;
}

struct event *
events_add(int fd, short events, void(*cb)(int, short, void *), void *arg)
{
  struct event *ev;

  ev = events_setup(fd, events, cb, arg);
  if(ev == NULL)
    return NULL;

  event_add(ev, NULL);
  return ev;
}

