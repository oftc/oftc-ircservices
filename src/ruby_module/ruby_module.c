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

static dlink_node *ruby_cmode_hook;
static dlink_node *ruby_umode_hook;
static dlink_node *ruby_newusr_hook;
static dlink_node *ruby_privmsg_hook;
static dlink_node *ruby_join_hook;
static dlink_node *ruby_nick_hook;

static VALUE ruby_server_hooks = Qnil;

static void *rb_cmode_hdlr(va_list);
static void *rb_umode_hdlr(va_list);
static void *rb_newusr_hdlr(va_list);
static void *rb_privmsg_hdlr(va_list);
static void *rb_join_hdlr(va_list);
static void *rb_nick_hdlr(va_list);

static void ruby_script_error();

static int rb_do_hook_each(VALUE key, VALUE name, VALUE params);
static void unhook_events(const char* name);

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
  if(parv != NULL)
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

char **
rb_rbarray2carray(VALUE parv)
{
  /*int argc = RARRAY(parv)->len;
  char **argv = (char *)MyMalloc(argc * sizeof(char *));
  int i;
  VALUE tmp;

  for(i = 0; i < argc; i++)
  {
    tmp = rb_ary_shift(parv);
    argv[i] = StringValueCStr(tmp);
  }

  return argv;*/
  return NULL;
}

static void *
rb_cmode_hdlr(va_list args)
{
  struct Client  *source_p = va_arg(args, struct Client*);
  struct Channel *chptr    = va_arg(args, struct Channel*);
  int dir = va_arg(args, int);
  char letter = (char)va_arg(args, int);
  char *param = va_arg(args, char *);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_CMODE);
  VALUE params = rb_ary_new();

  rb_ary_push(params, rb_cclient2rbclient(source_p));
  rb_ary_push(params, rb_cchannel2rbchannel(chptr));
  rb_ary_push(params, INT2NUM(dir));
  rb_ary_push(params, rb_str_new(&letter, 1));

  if(param != NULL)
    rb_ary_push(params, rb_str_new2(param));

  rb_hash_foreach(hooks, rb_do_hook_each, params);

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

  rb_hash_foreach(hooks, rb_do_hook_each, params);

  return pass_callback(ruby_umode_hook, user, what, mode);
}

static void *
rb_newusr_hdlr(va_list args)
{
  struct Client *newuser = va_arg(args, struct Client *);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_NEWUSR);
  VALUE params = rb_ary_new();

  rb_ary_push(params, rb_cclient2rbclient(newuser));
  rb_hash_foreach(hooks, rb_do_hook_each, params);

  return pass_callback(ruby_newusr_hook, newuser);
}

static void *
rb_privmsg_hdlr(va_list args)
{
  struct Client *source = va_arg(args, struct Client *);
  struct Channel *channel = va_arg(args, struct Channel *);
  char *message = va_arg(args, char *);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_PRIVMSG);
  VALUE params = rb_ary_new();

  rb_ary_push(params, rb_cclient2rbclient(source));
  rb_ary_push(params, rb_cchannel2rbchannel(channel));
  rb_ary_push(params, rb_str_new2(message));

  rb_hash_foreach(hooks, rb_do_hook_each, params);

  return pass_callback(ruby_privmsg_hook, source, channel, message);
}

static void *
rb_join_hdlr(va_list args)
{
  struct Client* source = va_arg(args, struct Client *);
  char *channel = va_arg(args, char *);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_JOIN);
  VALUE params = rb_ary_new();

  rb_ary_push(params, rb_cclient2rbclient(source));
  rb_ary_push(params, rb_str_new2(channel));

  rb_hash_foreach(hooks, rb_do_hook_each, params);

  return pass_callback(ruby_join_hook, source, channel);
}

static void *
rb_nick_hdlr(va_list args)
{
  struct Client *source = va_arg(args, struct Client *);
  char *oldnick = va_arg(args, char *);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_NICK);
  VALUE params = rb_ary_new();

  rb_ary_push(params, rb_cclient2rbclient(source));
  rb_ary_push(params, rb_str_new2(oldnick));

  rb_hash_foreach(hooks, rb_do_hook_each, params);

  return pass_callback(ruby_nick_hook, source, oldnick);
}

void
rb_add_hook(VALUE self, VALUE hook, int type)
{
  VALUE hooks = rb_ary_entry(ruby_server_hooks, type);
  VALUE newhook = rb_ary_new();
  rb_ary_push(newhook, self);
  rb_ary_push(newhook, hook);
  rb_hash_aset(hooks, rb_iv_get(self, "@ServiceName"), newhook);
}

void
unhook_events(const char* name)
{
  VALUE rname = rb_str_new2(name);
  int type;
  VALUE hooks;

  ilog(L_DEBUG, "Unhooking ruby hooks for: %s", name);

  for(type = 0; type < RB_HOOKS_COUNT; ++type)
  {
    hooks = rb_ary_entry(ruby_server_hooks, type);
    rb_hash_delete(hooks, rname);
  }
}

static int
rb_do_hook_each(VALUE key, VALUE value, VALUE params)
{
  VALUE self, command, command_id, fc2params;
  self = rb_ary_entry(value, 0);
  command = rb_ary_entry(value, 1);
  command_id = rb_intern(StringValueCStr(command));

  fc2params = rb_ary_new();
  rb_ary_push(fc2params, self);
  rb_ary_push(fc2params, command_id);
  rb_ary_push(fc2params, RARRAY(params)->len);
  rb_ary_push(fc2params, (VALUE)RARRAY(params)->ptr);

  if(!do_ruby(RB_CALLBACK(rb_singleton_call), (VALUE)fc2params))
    ilog(L_DEBUG, "RUBY ERROR: Failed to call: %s", StringValueCStr(command));

  return 1;
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

  ilog(L_DEBUG, "RUBY INFO: Loading ruby module: %s", path);

  rb_protect(RB_CALLBACK(rb_load_file), (VALUE)path, &status);

  if(ruby_handle_error(status))
  {
    ilog(L_DEBUG, "RUBY INFO: Failed to load file %s", path);
    return 0;
  }

  do_ruby(RB_CALLBACK(ruby_exec), (VALUE)NULL);

  strlcpy(classname, fname, strlen(fname)-2);

  klass = rb_protect(RB_CALLBACK(rb_path2class), (VALUE)(classname), &status);

  if(ruby_handle_error(status))
    return 0;

  ilog(L_DEBUG, "RUBY INFO: Loaded Class %s", classname);

  params = rb_ary_new();

  rb_ary_push(params, klass);
  rb_ary_push(params, rb_intern("new"));
  rb_ary_push(params, 0);
  rb_ary_push(params, (VALUE)NULL);

  self = rb_protect(RB_CALLBACK(rb_singleton_call), params, &status);

  if(ruby_handle_error(status))
    return 0;

  ilog(L_TRACE, "RUBY INFO: Initialized Class %s", classname);

  if(ServicesState.namesuffix)
    strlcat(classname, ServicesState.namesuffix, sizeof(classname));

  service = find_service(classname);

  if(service != NULL)
  {
    rb_gc_register_address(&self);
    service->data = (void *)self;
    return 1;
  }

  return 0;
}

int
unload_ruby_module(const char* name)
{
  char namet[PATH_MAX];
  struct Service *service;
  VALUE fc2params;

  strlcpy(namet, name, sizeof(namet));
  service = find_service(namet);

  if(service == NULL && ServicesState.namesuffix)
  {
    strlcat(namet, ServicesState.namesuffix, sizeof(namet));
    service = find_service(namet);
  }

  if(service == NULL)
  {
    ilog(L_DEBUG, "Unabled to find service: %s", namet);
    return -1;
  }

  ilog(L_DEBUG, "Unloading ruby module: %s", namet);

  fc2params = rb_ary_new();
  rb_ary_push(fc2params, (VALUE)service->data);
  rb_ary_push(fc2params, rb_intern("unload"));
  rb_ary_push(fc2params, 0);
  rb_ary_push(fc2params, (VALUE)NULL);

  if(!do_ruby(RB_CALLBACK(rb_singleton_call), fc2params))
    ilog(L_DEBUG, "Failed to call %s's unload method", namet);

  unhook_events(namet);
  serv_clear_messages(service);
  unload_languages(service->languages);

  exit_client(find_client(service->name), &me, "Service unloaded");
  hash_del_service(service);
  dlinkDelete(&service->node, &services_list);

  rb_gc_unregister_address(service->data);

  ilog(L_DEBUG, "Unloaded ruby module: %s", namet);

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
  Init_RegChannel();
  Init_NickStruct();

  /* Place holder for hooks */
  ruby_server_hooks = rb_ary_new();
  for(i=0; i < RB_HOOKS_COUNT; ++i)
    rb_ary_push(ruby_server_hooks, rb_hash_new());

  ruby_cmode_hook = install_hook(on_cmode_change_cb, rb_cmode_hdlr);
  ruby_umode_hook = install_hook(on_umode_change_cb, rb_umode_hdlr);
  ruby_newusr_hook = install_hook(on_newuser_cb, rb_newusr_hdlr);
  ruby_privmsg_hook = install_hook(on_privmsg_cb, rb_privmsg_hdlr);
  ruby_join_hook = install_hook(on_join_cb, rb_join_hdlr);
  ruby_nick_hook = install_hook(on_nick_change_cb, rb_nick_hdlr);
}

void
cleanup_ruby(void)
{
  ruby_finalize();
}
