/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  ruby_module.c: An interface to run ruby scripts
 *
 *  Copyright (C) 2006 TJ Fontaine and the OFTC Coding department
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

#include <ctype.h>
#include <signal.h>
#include <ruby.h>
#include "libruby_module.h"

static void *rb_cmode_hdlr(va_list);
static void *rb_umode_hdlr(va_list);
static void *rb_newusr_hdlr(va_list);

static void ruby_script_error();

char *
strupr(char *s)
{
  char *c;
  for (c=s; c && *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
  return c;
}

static void
ruby_script_error()
{
  VALUE lasterr, array;
  char *err;
  int i;

  if(!NIL_P(ruby_errinfo))
  {
    lasterr = rb_gv_get("$!");
    err = RSTRING(rb_obj_as_string(lasterr))->ptr;

    ilog(L_DEBUG, "RUBY ERROR: Error while executing Ruby Script: %s", err);
    array = rb_funcall(ruby_errinfo, rb_intern("backtrace"), 0);
    ilog(L_DEBUG, "RUBY ERROR: BACKTRACE");
    for (i = 0; i < RARRAY(array)->len; ++i)
      ilog(L_DEBUG, "RUBY ERROR:   %s", RSTRING(RARRAY(array)->ptr[i])->ptr);
  }
}

int
ruby_handle_error(int status)
{
  if(status)
  {
    ruby_script_error();
    //ruby_cleanup(status);
  }
  return status;
}

int
do_ruby(VALUE (*func)(), VALUE args)
{
  int status;

  rb_protect(func, args, &status);

  if(ruby_handle_error(status))
    return 0;

  return 1;
}

VALUE
rb_singleton_call(VALUE data)
{
  VALUE recv, id, argc, argv;
  recv = rb_ary_entry(data, 0);
  id = rb_ary_entry(data, 1);
  argc = rb_ary_entry(data, 2);
  argv = rb_ary_entry(data, 3);
  return rb_funcall2(recv, id, argc, (VALUE *)argv);
}

VALUE
rb_carray2rbarray(int parc, char **parv)
{
  if(parv)
  {
    VALUE rbarray = rb_ary_new2(parc);
    int i;
    char *cur;

    for (i = 0; i <= parc; ++i)
    {
      cur = parv[i];
      if(cur)
        rb_ary_push(rbarray, rb_str_new2(cur));
    }

    return rbarray;
  }
  else
    return rb_ary_new();
}

static void *
rb_cmode_hdlr(va_list args)
{
  struct Client  *source_p = va_arg(args, struct Client*);
  struct Channel *chptr    = va_arg(args, struct Channel*);
  int dir = va_arg(args, int);
  char letter = (char)va_arg(args, int);
  char *param = va_arg(args, char *);

#if 0 
  XXX fix this 
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_CMODE);
  VALUE params = rb_ary_new();

  rb_ary_push(params, rb_cclient2rbclient(client_p));
  rb_ary_push(params, rb_cclient2rbclient(source_p));
  rb_ary_push(params, rb_cchannel2rbchannel(chptr));
  rb_ary_push(params, rb_carray2rbarray(parc, parv));

  rb_do_hook_cb(hooks, params);
#endif

  return pass_callback(ruby_cmode_hook, source_p, chptr, dir, letter, param);
}

static void*
rb_umode_hdlr(va_list args)
{
  struct Client *user = va_arg(args, struct Client *);
  int what = va_arg(args, int);
  int mode = va_arg(args, int);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_UMODE);
  VALUE params = rb_ary_new();
  rb_ary_push(params, rb_cclient2rbclient(user));
  rb_ary_push(params, INT2NUM(what));
  rb_ary_push(params, INT2NUM(mode));

  rb_do_hook_cb(hooks, params);

  return pass_callback(ruby_umode_hook, user, what, mode);
}

static void *
rb_newusr_hdlr(va_list args)
{
  struct Client *newuser = va_arg(args, struct Client *);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_NEWUSR);
  VALUE params = rb_ary_new();

  rb_ary_push(params, rb_cclient2rbclient(newuser));
  rb_do_hook_cb(hooks, params);

  return pass_callback(ruby_newusr_hook, newuser);
}

void
rb_add_hook(VALUE self, VALUE hook, int type)
{
  VALUE hooks = rb_ary_entry(ruby_server_hooks, type);
  VALUE newhook = rb_ary_new();
  rb_ary_push(newhook, rb_iv_get(self, "@ServiceName"));
  rb_ary_push(newhook, self);
  rb_ary_push(newhook, hook);
  rb_ary_push(hooks, newhook);
}

void
rb_do_hook_cb(VALUE hooks, VALUE params)
{
  if(!NIL_P(hooks) && RARRAY(hooks)->len)
  {
    int i;
    VALUE current, service_name, self, command, fc2params;
    ID command_id;

    for(i = 0; i < RARRAY(hooks)->len; ++i)
    {
      current = rb_ary_entry(hooks, i);
      service_name = rb_ary_entry(current, 0);
      self = rb_ary_entry(current, 1);
      command = rb_ary_entry(current, 2);
      command_id = rb_intern(StringValueCStr(command));

      fc2params = rb_ary_new();
      rb_ary_push(fc2params, self);
      rb_ary_push(fc2params, command_id);
      rb_ary_push(fc2params, RARRAY(params)->len);
      rb_ary_push(fc2params, (VALUE)RARRAY(params)->ptr);

      if(!do_ruby(RB_CALLBACK(rb_singleton_call), (VALUE)fc2params))
      {
        ilog(L_DEBUG, "RUBY ERROR: Failed to call: %s", StringValueCStr(command));
      }
    }
  }
}

int
load_ruby_module(const char *name, const char *dir, const char *fname)
{
  int status;
  char path[PATH_MAX];
  char classname[PATH_MAX];
  VALUE klass, self;
  VALUE params;
  struct Service *service;


  memset(classname, 0, sizeof(classname));

  snprintf(path, sizeof(path), "%s/%s", dir, fname);

  ilog(L_TRACE, "RUBY INFO: Loading ruby module: %s", path);

  if(!do_ruby(RB_CALLBACK(rb_load_file), (VALUE)path))
    return 0;

  do_ruby(RB_CALLBACK(ruby_exec), (VALUE)NULL);

  strncpy(classname, fname, strlen(fname)-3);
  if(ServicesState.namesuffix)
    strlcat(classname, ServicesState.namesuffix, sizeof(classname));

  klass = rb_protect(RB_CALLBACK(rb_path2class), (VALUE)(classname), &status);

  if(ruby_handle_error(status))
    return 0;

  ilog(L_TRACE, "RUBY INFO: Loaded Class %s", classname);
  
  params = rb_ary_new();
  rb_ary_push(params, klass);
  rb_ary_push(params, rb_intern("new"));
  rb_ary_push(params, 0);
  rb_ary_push(params, (VALUE)NULL);

  self = rb_protect(RB_CALLBACK(rb_singleton_call), params, &status);

  if(ruby_handle_error(status))
    return 0;
  ilog(L_TRACE, "RUBY INFO: Initialized Class %s", classname);

  service = find_service(classname);
  if(service != NULL)
    service->data = (void *)self;

  return 1;
}

void
init_ruby(void)
{
  int i;

  ruby_init();
  ruby_show_version();
  ruby_init_loadpath();
  Init_ServiceModule();
  Init_ClientStruct();
  Init_ChannelStruct();
  Init_NickStruct();

  /* Place holder for hooks */
  ruby_server_hooks = rb_ary_new();
  for(i=0; i < RB_HOOKS_COUNT; ++i)
    rb_ary_push(ruby_server_hooks, rb_ary_new());

  ruby_cmode_hook = install_hook(on_cmode_change_cb, rb_cmode_hdlr);
  ruby_umode_hook = install_hook(on_umode_change_cb, rb_umode_hdlr);
  ruby_newusr_hook = install_hook(on_newuser_cb, rb_newusr_hdlr);
}

void
cleanup_ruby(void)
{
  ruby_finalize();
}
