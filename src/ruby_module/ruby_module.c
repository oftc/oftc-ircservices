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
static dlink_node *ruby_notice_hook;

static VALUE ruby_server_hooks = Qnil;

static void *rb_cmode_hdlr(va_list);
static void *rb_umode_hdlr(va_list);
static void *rb_newusr_hdlr(va_list);
static void *rb_privmsg_hdlr(va_list);
static void *rb_join_hdlr(va_list);
static void *rb_nick_hdlr(va_list);
static void *rb_notice_hdlr(va_list);

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
    VALUE tmp2 = rb_class_path(CLASS_OF(lasterr));
    ilog(L_DEBUG, "RUBY ERROR: %s", StringValueCStr(tmp2));
    VALUE tmp = rb_obj_as_string(lasterr);
    err = StringValueCStr(tmp);
    ilog(L_DEBUG, "RUBY ERROR: Error while executing Ruby Script: %s", err);
    array = rb_funcall(ruby_errinfo, rb_intern("backtrace"), 0);
    ilog(L_DEBUG, "RUBY ERROR: BACKTRACE");
    for (i = 0; i < RARRAY(array)->len; ++i)
    {
      tmp = rb_ary_entry(array, i);
      ilog(L_DEBUG, "RUBY ERROR:   %s", StringValueCStr(tmp));
    }
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
do_rubyv(VALUE recv, ID id, int parc, VALUE *parv)
{
  struct ruby_args args;
  int status;

  args.recv = recv;
  args.id = id;
  args.parc = parc;
  args.parv = parv;

  rb_protect(rb_singleton_call, (VALUE)&args, &status);

  if(ruby_handle_error(status))
    return 0;

  return 1;
}

VALUE
do_ruby_ret(VALUE recv, ID id, int parc, ...)
{
  int i;
  va_list varg;
  struct ruby_args args;
  int status;
  VALUE *parv;
  VALUE ret;

  if(parc > 0)
  {
    parv = ALLOCA_N(VALUE, parc);
    va_start(varg, parc);

    for(i = 0; i < parc; i++)
      parv[i] = va_arg(varg, VALUE);

    va_end(varg);
  }
  else
    parv = 0;

  args.recv = recv;
  args.id = id;
  args.parc = parc;
  args.parv = parv;

  ret = rb_protect(rb_singleton_call, (VALUE)&args, &status);

  if(ruby_handle_error(status))
    return Qnil;

  return ret;
}

int
do_ruby(VALUE recv, ID id, int parc, ...)
{
  int i;
  va_list varg;
  VALUE *parv;

  if(parc > 0)
  {
    parv = ALLOCA_N(VALUE, parc);
    va_start(varg, parc);

    for(i = 0; i < parc; i++)
      parv[i] = va_arg(varg, VALUE);

    va_end(varg);
  }
  else
    parv = 0;

  return do_rubyv(recv, id, parc, parv);
}

VALUE
rb_singleton_call(VALUE args)
{
  struct ruby_args *rarg = (struct ruby_args*)args;

  return rb_funcall2(rarg->recv, rarg->id, rarg->parc, rarg->parv);
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

static void
do_hook(VALUE hooks, int parc, ...)
{
  struct rhook_args arg;
  VALUE *params = 0;
  int i;
  va_list args;

  va_start(args, parc);
  
  if(parc > 0)
    params = ALLOCA_N(VALUE, parc);

  arg.parc = parc;
  arg.parv = params;

  for(i = 0; i < parc; i++)
    params[i] = va_arg(args, VALUE);

  va_end(args);

  rb_hash_foreach(hooks, rb_do_hook_each, (VALUE)&arg);
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

  if(param == NULL)
    do_hook(hooks, 5, rb_cclient2rbclient(source_p), 
        rb_cchannel2rbchannel(chptr), INT2NUM(dir), rb_str_new(&letter, 1), 0);
  else
    do_hook(hooks, 5, rb_cclient2rbclient(source_p), 
        rb_cchannel2rbchannel(chptr), INT2NUM(dir), rb_str_new(&letter, 1), 
        rb_str_new2(param));
 
  return pass_callback(ruby_cmode_hook, source_p, chptr, dir, letter, param);
}

static void*
rb_umode_hdlr(va_list args)
{
  struct Client *user = va_arg(args, struct Client *);
  int what = va_arg(args, int);
  int mode = va_arg(args, int);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_UMODE);

  do_hook(hooks, 3, rb_cclient2rbclient(user), INT2NUM(what), INT2NUM(mode));

  return pass_callback(ruby_umode_hook, user, what, mode);
}

static void *
rb_newusr_hdlr(va_list args)
{
  struct Client *newuser = va_arg(args, struct Client *);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_NEWUSR);

  do_hook(hooks, 1, rb_cclient2rbclient(newuser));

  return pass_callback(ruby_newusr_hook, newuser);
}

static void *
rb_privmsg_hdlr(va_list args)
{
  struct Client *source = va_arg(args, struct Client *);
  struct Channel *channel = va_arg(args, struct Channel *);
  char *message = va_arg(args, char *);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_PRIVMSG);

  do_hook(hooks, 3, rb_cclient2rbclient(source), 
      rb_cchannel2rbchannel(channel), rb_str_new2(message));

  return pass_callback(ruby_privmsg_hook, source, channel, message);
}

static void *
rb_join_hdlr(va_list args)
{
  struct Client* source = va_arg(args, struct Client *);
  char *channel = va_arg(args, char *);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_JOIN);

  do_hook(hooks, 2, rb_cclient2rbclient(source), rb_str_new2(channel));

  return pass_callback(ruby_join_hook, source, channel);
}

static void *
rb_nick_hdlr(va_list args)
{
  struct Client *source = va_arg(args, struct Client *);
  char *oldnick = va_arg(args, char *);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_NICK);

  do_hook(hooks, 2, rb_cclient2rbclient(source), rb_str_new2(oldnick));

  return pass_callback(ruby_nick_hook, source, oldnick);
}

static void *
rb_notice_hdlr(va_list args)
{
  struct Client *source = va_arg(args, struct Client *);
  struct Channel *channel = va_arg(args, struct Channel *);
  char *message = va_arg(args, char *);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_NOTICE);

  do_hook(hooks, 3, rb_cclient2rbclient(source), rb_cchannel2rbchannel(channel),
      rb_str_new2(message));

  return pass_callback(ruby_notice_hook, source, channel, message);
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
rb_do_hook_each(VALUE key, VALUE value, VALUE arg)
{
  VALUE self, command, command_id;
  struct rhook_args *args = (struct rhook_args*)arg;

  self = rb_ary_entry(value, 0);
  command = rb_ary_entry(value, 1);
  command_id = rb_intern(StringValueCStr(command));

  if(!do_rubyv(self, command_id, args->parc, args->parv))
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

  rb_protect(RB_CALLBACK(ruby_exec), (VALUE)NULL, &status);

  if(ruby_handle_error(status))
  {
    ilog(L_DEBUG, "RUBY INFO: Failed to load file %s", path);
    return 0;
  }

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

  self = do_ruby_ret(klass, rb_intern("new"), 0);

  if(self == Qnil)
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

  if(!do_ruby((VALUE)service->data, rb_intern("unload"), 0))
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
  int i, status;
  char path[PATH_MAX];

  ruby_init();
  ruby_show_version();
  ruby_init_loadpath();

  snprintf(path, sizeof(path), "%s/%s", MODPATH, "ServiceBase.rb");
  status = 0;
  rb_protect(RB_CALLBACK(rb_load_file), (VALUE)path, &status);
  if(ruby_handle_error(status))
  {
    ilog(L_CRIT, "Failed to load ruby module Service aborting");
    return;
  }

  rb_protect(RB_CALLBACK(ruby_exec), (VALUE)NULL, &status);

  if(ruby_handle_error(status))
  {
    ilog(L_CRIT, "Failed to exec after loading module Service");
    return;
  }

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
  ruby_notice_hook = install_hook(on_notice_cb, rb_notice_hdlr);

  /* pin any ruby address we keep on the C side */
  rb_gc_register_address(&ruby_server_hooks);
}

void
cleanup_ruby(void)
{
  ruby_finalize();
}
