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
static dlink_node *ruby_part_hook;
static dlink_node *ruby_quit_hook;
static dlink_node *ruby_nick_hook;
static dlink_node *ruby_notice_hook;
static dlink_node *ruby_chan_create_hook;
static dlink_node *ruby_chan_delete_hook;
static dlink_node *ruby_ctcp_hook;
static dlink_node *ruby_nick_reg_hook;
static dlink_node *ruby_chan_reg_hook;
static dlink_node *ruby_db_init_hook;
static dlink_node *ruby_eob_hook;

static VALUE ruby_server_hooks = Qnil;
static VALUE ruby_server_events = Qnil;

static void *rb_cmode_hdlr(va_list);
static void *rb_umode_hdlr(va_list);
static void *rb_newusr_hdlr(va_list);
static void *rb_privmsg_hdlr(va_list);
static void *rb_join_hdlr(va_list);
static void *rb_part_hdlr(va_list);
static void *rb_quit_hdlr(va_list);
static void *rb_nick_hdlr(va_list);
static void *rb_notice_hdlr(va_list);
static void *rb_chan_create_hdlr(va_list);
static void *rb_chan_delete_hdlr(va_list);
static void *rb_ctcp_hdlr(va_list);
static void *rb_nick_reg_hdlr(va_list);
static void *rb_chan_reg_hdlr(va_list);
static void *rb_db_init_hdlr(va_list);
static void *rb_eob_hdlr(va_list);

static void ruby_script_error();

static VALUE rb_do_hook_each(VALUE key, VALUE name, VALUE params);
static void unhook_callbacks(const char *);
static void unhook_events(VALUE);

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

VALUE
do_rubyv(VALUE recv, ID id, int parc, VALUE *parv)
{
  struct ruby_args args;
  int status;
  VALUE retval;

  args.recv = recv;
  args.id = id;
  args.parc = parc;
  args.parv = parv;

  retval = rb_protect(rb_singleton_call, (VALUE)&args, &status);

  if(ruby_handle_error(status))
    return Qnil;

  return retval;
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

VALUE
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

void
check_our_type(VALUE obj, VALUE type)
{
  char *objname;
  char *typename;

  if(CLASS_OF(obj) != type)
  {
    DupString(objname, rb_obj_classname(obj));
    DupString(typename, rb_class2name(type));
    rb_raise(rb_eTypeError, "Argument Exception: Expected type %s got %s", typename, objname);
    MyFree(objname);
    MyFree(typename);
  }
}

static VALUE
do_hook(VALUE hooks, int parc, ...)
{
  struct rhook_args arg;
  VALUE *params = 0;
  VALUE keys, ret;
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

  keys = do_ruby_ret(hooks, rb_intern("keys"), 0);
  for(i = 0; i < RARRAY(keys)->len; ++i)
  {
    VALUE key, value;
    key = rb_ary_entry(keys, i);
    value = rb_hash_aref(hooks, key);
    ret = rb_do_hook_each(key, value, (VALUE)&arg);
    if(ret == Qfalse)
      break;
  }

  return ret;
}

static VALUE
rb_do_hook_each(VALUE key, VALUE value, VALUE arg)
{
  VALUE self, command, command_id, ret;
  struct rhook_args *args = (struct rhook_args*)arg;

  self = rb_ary_entry(value, 0);
  command = rb_ary_entry(value, 1);
  command_id = rb_intern(StringValueCStr(command));

  ret = do_rubyv(self, command_id, args->parc, args->parv);

  switch(ret)
  {
    case Qnil:
      ilog(L_DEBUG, "{%s} Returned nil while processing callback: %s", StringValueCStr(key),
        StringValueCStr(command));
      break;
    case Qfalse:
      ilog(L_DEBUG, "{%s} Interrupted Callback Chain in %s", StringValueCStr(key),
        StringValueCStr(command));
      break;
  }

  return ret;
}

static void *
rb_cmode_hdlr(va_list args)
{
  struct Client  *source_p = va_arg(args, struct Client*);
  struct Channel *chptr    = va_arg(args, struct Channel*);
  int dir = va_arg(args, int);
  char letter = (char)va_arg(args, int);
  char *param = va_arg(args, char *);
  VALUE ret;
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_CMODE);

  if(param == NULL)
    ret = do_hook(hooks, 5, client_to_value(source_p),
        rb_cchannel2rbchannel(chptr), INT2NUM(dir), rb_str_new(&letter, 1), 0);
  else
    ret = do_hook(hooks, 5, client_to_value(source_p),
        rb_cchannel2rbchannel(chptr), INT2NUM(dir), rb_str_new(&letter, 1),
        rb_str_new2(param));

  if(ret != Qfalse)
    return pass_callback(ruby_cmode_hook, source_p, chptr, dir, letter, param);
  else
    return NULL;
}

static void*
rb_umode_hdlr(va_list args)
{
  struct Client *user = va_arg(args, struct Client *);
  int what = va_arg(args, int);
  int mode = va_arg(args, int);
  VALUE ret;
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_UMODE);

  ret = do_hook(hooks, 3, client_to_value(user), INT2NUM(what), INT2NUM(mode));

  if(ret != Qfalse)
    return pass_callback(ruby_umode_hook, user, what, mode);
  else
    return NULL;
}

static void *
rb_newusr_hdlr(va_list args)
{
  struct Client *newuser = va_arg(args, struct Client *);
  VALUE ret;
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_NEWUSR);

  ret = do_hook(hooks, 1, client_to_value(newuser));

  if(ret != Qfalse)
    return pass_callback(ruby_newusr_hook, newuser);
  else
    return NULL;
}

static void *
rb_privmsg_hdlr(va_list args)
{
  struct Client *source = va_arg(args, struct Client *);
  struct Channel *channel = va_arg(args, struct Channel *);
  char *message = va_arg(args, char *);
  VALUE ret;
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_PRIVMSG);

  ret = do_hook(hooks, 3, client_to_value(source),
      rb_cchannel2rbchannel(channel), rb_str_new2(message));

  if(ret != Qfalse)
    return pass_callback(ruby_privmsg_hook, source, channel, message);
  else
    return NULL;
}

static void *
rb_join_hdlr(va_list args)
{
  struct Client* source = va_arg(args, struct Client *);
  char *channel = va_arg(args, char *);
  VALUE ret;
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_JOIN);

  ret = do_hook(hooks, 2, client_to_value(source), rb_str_new2(channel));

  if(ret != Qfalse)
    return pass_callback(ruby_join_hook, source, channel);
  else
    return NULL;
}

static void *
rb_part_hdlr(va_list args)
{
  struct Client* client = va_arg(args, struct Client *);
  struct Client* source = va_arg(args, struct Client *);
  struct Channel* channel = va_arg(args, struct Channel *);
  char *reason = va_arg(args, char *);
  VALUE ret;
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_PART);

  ret = do_hook(hooks, 4, client_to_value(client), client_to_value(source),
    rb_cchannel2rbchannel(channel), rb_str_new2(reason));

  if(ret != Qfalse)
    return pass_callback(ruby_part_hook, client, source, channel, reason);
  else
    return NULL;
}

static void *
rb_quit_hdlr(va_list args)
{
  struct Client* client = va_arg(args, struct Client *);
  char *reason = va_arg(args, char *);
  VALUE ret;
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_QUIT);

  ret = do_hook(hooks, 2, client_to_value(client), rb_str_new2(reason));

  if(ret != Qfalse)
    return pass_callback(ruby_quit_hook, client, reason);
  else
    return NULL;
}

static void *
rb_nick_hdlr(va_list args)
{
  struct Client *source = va_arg(args, struct Client *);
  char *oldnick = va_arg(args, char *);
  VALUE ret;
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_NICK);

  ret = do_hook(hooks, 2, client_to_value(source), rb_str_new2(oldnick));

  if(ret != Qfalse)
    return pass_callback(ruby_nick_hook, source, oldnick);
  else
    return NULL;
}

static void *
rb_notice_hdlr(va_list args)
{
  struct Client *source = va_arg(args, struct Client *);
  struct Channel *channel = va_arg(args, struct Channel *);
  char *message = va_arg(args, char *);
  VALUE ret;
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_NOTICE);

  ret = do_hook(hooks, 3, client_to_value(source), rb_cchannel2rbchannel(channel),
      rb_str_new2(message));

  if(ret != Qfalse)
    return pass_callback(ruby_notice_hook, source, channel, message);
  else
    return NULL;
}

static void *
rb_chan_create_hdlr(va_list args)
{
  struct Channel *channel = va_arg(args, struct Channel *);
  VALUE ret;
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_CHAN_CREATED);

  ret = do_hook(hooks, 1, rb_cchannel2rbchannel(channel));

  if(ret != Qfalse)
    return pass_callback(ruby_chan_create_hook, channel);
  else
    return NULL;
}

static void *
rb_chan_delete_hdlr(va_list args)
{
  struct Channel *channel = va_arg(args, struct Channel *);
  VALUE ret;
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_CHAN_DELETED);

  ret = do_hook(hooks, 1, rb_cchannel2rbchannel(channel));

  if(ret != Qfalse)
    return pass_callback(ruby_chan_delete_hook, channel);
  else
    return NULL;
}

static void *
rb_ctcp_hdlr(va_list args)
{
  struct Service *service = va_arg(args, struct Service *);
  struct Client *client   = va_arg(args, struct Client *);
  char *command           = va_arg(args, char *);
  char *arg               = va_arg(args, char *);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_CTCP);
  VALUE varg, ret;

  if(arg == NULL)
    varg = Qnil;
  else
    varg = rb_str_new2(arg);

  ret = do_hook(hooks, 4, /*TODO*/service,
    client_to_value(client), rb_str_new2(command), varg);

  if(ret != Qfalse)
    return pass_callback(ruby_ctcp_hook, service, client, command, arg);
  else
    return NULL;
}

static void *
rb_chan_reg_hdlr(va_list args)
{
  struct Client *client = va_arg(args, struct Client *);
  struct Channel *channel = va_arg(args, struct Channel *);
  VALUE ret;
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_CHAN_REG);

  ret = do_hook(hooks, 2, client_to_value(client), rb_cchannel2rbchannel(channel));

  if(ret != Qfalse)
    return pass_callback(ruby_chan_reg_hook, client, channel);
  else
    return NULL;
}

static void *
rb_nick_reg_hdlr(va_list args)
{
  struct Client *client = va_arg(args, struct Client *);
  VALUE ret;
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_NICK_REG);

  ret = do_hook(hooks, 1, client_to_value(client));

  if(ret != Qfalse)
    return pass_callback(ruby_nick_reg_hook, client);
  else
    return NULL;
}

static void *
rb_db_init_hdlr(va_list args)
{
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_DB_INIT);
  VALUE ret = do_hook(hooks, 0, Qnil);

  if(ret != Qfalse)
    return pass_callback(ruby_db_init_hook);
  else
    return NULL;
}

static void *
rb_eob_hdlr(va_list args)
{
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_EOB);
  VALUE ret = do_hook(hooks, 0, Qnil);

  if(ret != Qfalse)
    return pass_callback(ruby_eob_hook);
  else
    return NULL;
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

static void
unhook_callbacks(const char* name)
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

enum EVENT_POSITION
{
  EVT_SELF = 0,
  EVT_METHOD,
  EVT_TIMER,
  EVT_LAST,
  EVT_COUNT,
};

void
rb_add_event(VALUE self, VALUE method, VALUE time)
{
  VALUE sn = rb_iv_get(self, "@ServiceName");
  VALUE events = rb_hash_aref(ruby_server_events, sn);
  VALUE event = rb_ary_new2(EVT_COUNT);
  rb_ary_store(event, EVT_SELF, self);
  rb_ary_store(event, EVT_METHOD, method);
  rb_ary_store(event, EVT_TIMER, time);
  rb_ary_store(event, EVT_LAST, LONG2NUM(CurrentTime));

  if(events == Qnil)
  {
    events = rb_ary_new();
    rb_hash_aset(ruby_server_events, sn, events);
  }

  ilog(L_DEBUG, "{%s} Adding Event: %s Every %d", StringValueCStr(sn), StringValueCStr(method), NUM2INT(time));
  rb_ary_push(events, event);
}

static void
unhook_events(VALUE self)
{
  VALUE sn = rb_iv_get(self, "@ServiceName");
  rb_hash_delete(ruby_server_events, sn);
}

static void
m_generic_event(void *param)
{
  int i, j;
  int delta;
  VALUE keys = do_ruby_ret(ruby_server_events, rb_intern("keys"), 0);
  VALUE key, events, event, self, method, timer, ltime;

  for(i = 0; i < RARRAY(keys)->len; ++i)
  {
    key = rb_ary_entry(keys, i);
    events = rb_hash_aref(ruby_server_events, key);

    for(j = 0; j < RARRAY(events)->len; ++j)
    {
      event = rb_ary_entry(events, j);
      self = rb_ary_entry(event, EVT_SELF);
      method = rb_ary_entry(event, EVT_METHOD);
      timer = rb_ary_entry(event, EVT_TIMER);
      ltime = rb_ary_entry(event, EVT_LAST);

      delta = CurrentTime - NUM2LONG(ltime);

      if(delta >= NUM2LONG(timer))
      {
        do_ruby(self, rb_intern(StringValueCStr(method)), 0);
        rb_ary_store(event, EVT_LAST, LONG2NUM(CurrentTime));
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
  VALUE self;

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

  self = (VALUE)service->data;

  unhook_events(self);

  if(!do_ruby(self, rb_intern("unload"), 0))
    ilog(L_DEBUG, "Failed to call %s's unload method", namet);

  unhook_callbacks(namet);
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
  int i;//, status;
  //char path[PATH_MAX];

  ruby_init();
  ruby_show_version();
  ruby_init_loadpath();

  Init_ServiceModule();
  Init_Client();
  Init_ChannelStruct();
  Init_RegChannel();
  Init_NickStruct();

  /* Place holder for hooks */
  ruby_server_hooks = rb_ary_new();
  for(i=0; i < RB_HOOKS_COUNT; ++i)
    rb_ary_push(ruby_server_hooks, rb_hash_new());

  ruby_server_events = rb_hash_new();

  ruby_cmode_hook = install_hook(on_cmode_change_cb, rb_cmode_hdlr);
  ruby_umode_hook = install_hook(on_umode_change_cb, rb_umode_hdlr);
  ruby_newusr_hook = install_hook(on_newuser_cb, rb_newusr_hdlr);
  ruby_privmsg_hook = install_hook(on_privmsg_cb, rb_privmsg_hdlr);
  ruby_join_hook = install_hook(on_join_cb, rb_join_hdlr);
  ruby_part_hook = install_hook(on_part_cb, rb_part_hdlr);
  ruby_quit_hook = install_hook(on_quit_cb, rb_quit_hdlr);
  ruby_nick_hook = install_hook(on_nick_change_cb, rb_nick_hdlr);
  ruby_notice_hook = install_hook(on_notice_cb, rb_notice_hdlr);
  ruby_chan_create_hook = install_hook(on_channel_created_cb, rb_chan_create_hdlr);
  ruby_chan_delete_hook = install_hook(on_channel_destroy_cb, rb_chan_delete_hdlr);
  ruby_ctcp_hook = install_hook(on_ctcp_cb, rb_ctcp_hdlr);
  ruby_chan_reg_hook = install_hook(on_chan_reg_cb, rb_chan_reg_hdlr);
  ruby_nick_reg_hook = install_hook(on_nick_reg_cb, rb_nick_reg_hdlr);
  ruby_db_init_hook = install_hook(on_db_init_cb, rb_db_init_hdlr);
  ruby_eob_hook = install_hook(on_burst_done_cb, rb_eob_hdlr);

  /* pin any ruby address we keep on the C side */
  rb_gc_register_address(&ruby_server_hooks);
  rb_gc_register_address(&ruby_server_events);

  eventAdd("Generic Ruby Event Handler", m_generic_event, NULL, 10);
}

void
cleanup_ruby(void)
{
  eventDelete(m_generic_event, NULL);
  ruby_finalize();
}
