/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
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

#include <ruby.h>
/* Umm, it sucks but ruby defines these and so do we */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef EXTERN
#include <ctype.h>
#include <signal.h>
#include "stdinc.h"

#define RB_CALLBACK(x) (VALUE (*)())(x)

VALUE cServiceModule = Qnil;
VALUE cClientStruct = Qnil;
VALUE cChannelStruct = Qnil;

VALUE ruby_server_hooks = Qnil;

#define RB_HOOKS_CMODE 0
#define RB_HOOKS_UMODE 1

static dlink_node *ruby_cmode_hook;
static dlink_node *ruby_umode_hook;

static void ruby_script_error();
static int ruby_handle_error(int);
static int do_ruby(VALUE(*)(), VALUE);

static struct Client* rb_rbclient2cclient(VALUE);
static VALUE rb_cclient2rbclient(struct Client*);
static VALUE rb_carray2rbarray(int, char **);
static struct Channel* rb_rbchannel2cchannel(VALUE);
static VALUE rb_cchannel2rbchannel(struct Channel*);

static VALUE rb_singleton_call(VALUE);

static VALUE ServiceModule_register(VALUE, VALUE);
static VALUE ServiceModule_reply_user(VALUE, VALUE, VALUE);
static VALUE ServiceModule_service_name(VALUE, VALUE);
static VALUE ServiceModule_cmode_hook(VALUE, VALUE);
static VALUE ServiceModule_umode_hook(VALUE, VALUE);
/* DB Prototypes */
static VALUE ServiceModule_find_nick(VALUE, VALUE);
static VALUE ServiceModule_register_nick(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_set_language(VALUE, VALUE, VALUE);
static VALUE ServiceModule_set_password(VALUE, VALUE, VALUE);
static VALUE ServiceModule_find_chan(VALUE, VALUE);
static VALUE ServiceModule_register_chan(VALUE, VALUE, VALUE);
static VALUE ServiceModule_delete_nick(VALUE, VALUE);
static VALUE ServiceModule_set_url(VALUE, VALUE, VALUE);
static VALUE ServiceModule_set_email(VALUE, VALUE, VALUE);
static VALUE ServiceModule_set_cloak(VALUE, VALUE, VALUE);
/* DB Prototypes */
static void Init_ServiceModule(void);

static VALUE ClientStruct_Initialize(VALUE, VALUE);
static VALUE ClientStruct_Name(VALUE);
static VALUE ClientStruct_Host(VALUE);
static VALUE ClientStruct_ID(VALUE);
static VALUE ClientStruct_Info(VALUE);
static VALUE ClientStruct_Username(VALUE);
static VALUE ClientStruct_Umodes(VALUE);
static void Init_ClientStruct(void);

static VALUE ChannelStruct_Initialize(VALUE, VALUE);
static VALUE ChannelStruct_Name(VALUE);
static VALUE ChannelStruct_Topic(VALUE);
static VALUE ChannelStruct_TopicInfo(VALUE);
static VALUE ChannelStruct_Mode(VALUE);
static void Init_ChannelStruct(void);

static void m_generic(struct Service *, struct Client *, int, char**);

static char *strupr(char *s)
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

    printf("RUBY ERROR: Error while executing Ruby Script: %s\n", err);
    array = rb_funcall(ruby_errinfo, rb_intern("backtrace"), 0);
    printf("RUBY ERROR: BACKTRACE\n");
    for (i = 0; i < RARRAY(array)->len; ++i)
      printf("RUBY ERROR:   %s\n", RSTRING(RARRAY(array)->ptr[i])->ptr);
  }
}

static int
ruby_handle_error(int status)
{
  if(status)
  {
    ruby_script_error();
    //ruby_cleanup(status);
  }
  return status;
}

static int
do_ruby(VALUE (*func)(), VALUE args)
{
  int status;

  rb_protect(func, args, &status);

  if(ruby_handle_error(status))
    return 0;

  return 1;
}

static VALUE
rb_singleton_call(VALUE data)
{
  VALUE recv, id, argc, argv;
  recv = rb_ary_entry(data, 0);
  id = rb_ary_entry(data, 1);
  argc = rb_ary_entry(data, 2);
  argv = rb_ary_entry(data, 3);
  return rb_funcall2(recv, id, argc, (VALUE *)argv);
}

static struct Client* rb_rbclient2cclient(VALUE client)
{
  struct Client* out;
  Data_Get_Struct(client, struct Client, out);
  return out;
}

static VALUE
rb_cclient2rbclient(struct Client *client)
{
  VALUE fc2params, rbclient, real_client;
  int status;

  rbclient = Data_Wrap_Struct(rb_cObject, 0, free, client);

  fc2params = rb_ary_new();
  rb_ary_push(fc2params, cClientStruct);
  rb_ary_push(fc2params, rb_intern("new"));
  rb_ary_push(fc2params, 1);
  rb_ary_push(fc2params, (VALUE)&rbclient);

  real_client = rb_protect(RB_CALLBACK(rb_singleton_call), fc2params, &status);

  if(ruby_handle_error(status))
  {
    printf("RUBY ERROR: Ruby Failed To Create ClientStruct\n");
    return Qnil;
  }

  return real_client;
}

static struct Channel* rb_rbchannel2cchannel(VALUE channel)
{
  struct Channel* out;
  Data_Get_Struct(channel, struct Channel, out);
  return out;
}

static VALUE
rb_cchannel2rbchannel(struct Channel *channel)
{
  VALUE fc2params, rbchannel, real_channel;
  int status;

  rbchannel = Data_Wrap_Struct(rb_cObject, 0, free, channel);

  fc2params = rb_ary_new();
  rb_ary_push(fc2params, cChannelStruct);
  rb_ary_push(fc2params, rb_intern("new"));
  rb_ary_push(fc2params, 1);
  rb_ary_push(fc2params, (VALUE)&rbchannel);

  real_channel = rb_protect(RB_CALLBACK(rb_singleton_call), fc2params, &status);

  if(ruby_handle_error(status))
  {
    printf("RUBY ERROR: Ruby Failed To Create ChannelStruct\n");
    return Qnil;
  }

  return real_channel;
}

static VALUE
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

static void
m_generic(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  char *command = strdup(service->last_command);
  VALUE rbparams, rbparv;
  VALUE class, real_client, self;
  VALUE fc2params;
  ID class_command;

  strupr(command);
  class_command = rb_intern(command);

  class = rb_path2class(service->name);

  rbparams = rb_ary_new();

  real_client = rb_cclient2rbclient(client);
  rbparv = rb_carray2rbarray(parc, parv);

  rb_ary_push(rbparams, real_client);
  rb_ary_push(rbparams, rbparv);

  self = (VALUE)service->data;

  fc2params = rb_ary_new();
  rb_ary_push(fc2params, self);
  rb_ary_push(fc2params, class_command);
  rb_ary_push(fc2params, RARRAY(rbparams)->len);
  rb_ary_push(fc2params, (VALUE)RARRAY(rbparams)->ptr);

  printf("RUBY INFO: Calling Command: %s From %s\n", command, client->name);

  if(!do_ruby(RB_CALLBACK(rb_singleton_call), fc2params))
  {
    reply_user(service, client, "An error has occurred, please be patient and report this bug");
    global_notice(service, "Ruby Failed to Execute Command: %s by %s", command, client->name);
    printf("RUBY ERROR: Ruby Failed to Execute Command: %s by %s\n", command, client->name);
  }
}

static VALUE
ServiceModule_register(VALUE self, VALUE commands)
{
  struct Service *ruby_service;
  struct ServiceMessage *generic_msgtab;
  VALUE command, service_name;
  long i;
  int n;

  service_name = rb_iv_get(self, "@ServiceName");

  ruby_service = make_service(StringValueCStr(service_name));

  clear_serv_tree_parse(&ruby_service->msg_tree);
  dlinkAdd(ruby_service, &ruby_service->node, &services_list);
  hash_add_service(ruby_service);
  introduce_service(ruby_service);

  Check_Type(commands, T_ARRAY);

  for(i = RARRAY(commands)->len-1; i >= 0; --i)
  {
    generic_msgtab = MyMalloc(sizeof(struct ServiceMessage));
    command = rb_ary_shift(commands);
    generic_msgtab->cmd = StringValueCStr(command);
    rb_ary_push(commands, command);

    for(n = 0; n < SERVICES_LAST_HANDLER_TYPE; n++)
      generic_msgtab->handlers[n] = m_generic;

    mod_add_servcmd(&ruby_service->msg_tree, generic_msgtab);
  }


  return Qnil;
}

static VALUE
ServiceModule_reply_user(VALUE self, VALUE rbclient, VALUE rbmessage)
{
  struct Client *client;
  struct Service *service;
  VALUE inner_client, service_name;

  service_name = rb_iv_get(self, "@ServiceName");
  service = find_service(StringValueCStr(service_name));

  inner_client = rb_iv_get(rbclient, "@realptr");
  client = rb_rbclient2cclient(inner_client);

  char *message = StringValueCStr(rbmessage);

  reply_user(service, client, message);
  return Qnil;
}

static VALUE
ServiceModule_service_name(VALUE self, VALUE name)
{
  return rb_iv_set(self, "@ServiceName", name);
}

static VALUE
ServiceModule_cmode_hook(VALUE self, VALUE hook)
{
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_CMODE);
  VALUE newhook = rb_ary_new();
  rb_ary_push(newhook, rb_iv_get(self, "@ServiceName"));
  rb_ary_push(newhook, self);
  rb_ary_push(newhook, hook);
  rb_ary_push(hooks, newhook);
  return Qnil;
}

static VALUE
ServiceModule_umode_hook(VALUE self, VALUE hook)
{
  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_UMODE);
  VALUE newhook = rb_ary_new();
  rb_ary_push(newhook, rb_iv_get(self, "@ServiceName"));
  rb_ary_push(newhook, self);
  rb_ary_push(newhook, hook);
  rb_ary_push(hooks, newhook);
  return Qnil;
}

static VALUE ServiceModule_find_nick(VALUE self, VALUE nick)
{
  return Qnil;
}

static VALUE ServiceModule_register_nick(VALUE self, VALUE nick, VALUE password, VALUE email)
{
  return Qnil;
}

static VALUE ServiceModule_set_language(VALUE self, VALUE client, VALUE language)
{
  return Qnil;
}

static VALUE ServiceModule_set_password(VALUE self, VALUE client, VALUE password)
{
  return Qnil;
}

static VALUE ServiceModule_find_chan(VALUE self, VALUE channel)
{
  return Qnil;
}

static VALUE ServiceModule_register_chan(VALUE self, VALUE client, VALUE chan_name)
{
  return Qnil;
}

static VALUE ServiceModule_delete_nick(VALUE self, VALUE client)
{
  return Qnil;
}

static VALUE ServiceModule_set_url(VALUE self, VALUE client, VALUE url)
{
  return Qnil;
}

static VALUE ServiceModule_set_email(VALUE self, VALUE client, VALUE email)
{
  return Qnil;
}

static VALUE ServiceModule_set_cloak(VALUE self, VALUE nick, VALUE cloakstring)
{
  return Qnil;
}

static void
Init_ServiceModule(void)
{
  cServiceModule = rb_define_class("ServiceModule", rb_cObject);
  rb_define_class_variable(cServiceModule, "@@ServiceName", rb_str_new2(""));
  rb_define_method(cServiceModule, "register", ServiceModule_register, 1);
  rb_define_method(cServiceModule, "reply_user", ServiceModule_reply_user, 2);
  rb_define_method(cServiceModule, "service_name", ServiceModule_service_name, 1);
  rb_define_method(cServiceModule, "add_cmode_hook", ServiceModule_cmode_hook, 1);
  rb_define_method(cServiceModule, "add_umode_hook", ServiceModule_umode_hook, 1);
  rb_define_method(cServiceModule, "find_nick", ServiceModule_find_nick, 1);
  rb_define_method(cServiceModule, "register_nick", ServiceModule_register_nick, 3);
  rb_define_method(cServiceModule, "set_language", ServiceModule_set_language, 2);
  rb_define_method(cServiceModule, "set_password", ServiceModule_set_password, 2);
  rb_define_method(cServiceModule, "find_chan", ServiceModule_find_chan, 1);
  rb_define_method(cServiceModule, "register_chan", ServiceModule_register_chan, 2);
  rb_define_method(cServiceModule, "delete_nick", ServiceModule_delete_nick, 1);
  rb_define_method(cServiceModule, "set_url", ServiceModule_set_url, 2);
  rb_define_method(cServiceModule, "set_email", ServiceModule_set_email, 2);
  rb_define_method(cServiceModule, "set_cloak", ServiceModule_set_cloak, 2);
}

static VALUE
ClientStruct_Initialize(VALUE self, VALUE client)
{
  rb_iv_set(self, "@realptr", client);
  return self;
}

static VALUE
ClientStruct_Name(VALUE self)
{
  VALUE clientstruct = rb_iv_get(self, "@realptr");
  return rb_str_new2(rb_rbclient2cclient(clientstruct)->name);
}

static VALUE
ClientStruct_Host(VALUE self)
{
  VALUE clientstruct = rb_iv_get(self, "@realptr");
  return rb_str_new2(rb_rbclient2cclient(clientstruct)->host);
}

static VALUE
ClientStruct_ID(VALUE self)
{
  VALUE clientstruct = rb_iv_get(self, "@realptr");
  return rb_str_new2(rb_rbclient2cclient(clientstruct)->id);
}

static VALUE
ClientStruct_Info(VALUE self)
{
  VALUE clientstruct = rb_iv_get(self, "@realptr");
  return rb_str_new2(rb_rbclient2cclient(clientstruct)->info);
}

static VALUE
ClientStruct_Username(VALUE self)
{
  VALUE clientstruct = rb_iv_get(self, "@realptr");
  return rb_str_new2(rb_rbclient2cclient(clientstruct)->username);
}

static VALUE
ClientStruct_Umodes(VALUE self)
{
  VALUE clientstruct = rb_iv_get(self, "@realptr");
  return INT2NUM(rb_rbclient2cclient(clientstruct)->umodes);
}

static void
Init_ClientStruct(void)
{
  cClientStruct = rb_define_class("ClientStruct", rb_cObject);

  rb_define_class_variable(cClientStruct, "@@realptr", Qnil);

  rb_define_method(cClientStruct, "initialize", ClientStruct_Initialize, 1);
  rb_define_method(cClientStruct, "name", ClientStruct_Name, 0);
  rb_define_method(cClientStruct, "host", ClientStruct_Host, 0);
  rb_define_method(cClientStruct, "id", ClientStruct_ID, 0);
  rb_define_method(cClientStruct, "info", ClientStruct_Info, 0);
  rb_define_method(cClientStruct, "username", ClientStruct_Username, 0);
  rb_define_method(cClientStruct, "umodes", ClientStruct_Umodes, 0);
}

static VALUE
ChannelStruct_Initialize(VALUE self, VALUE channel)
{
  rb_iv_set(self, "@realptr", channel);
  return self;
}

static VALUE ChannelStruct_Name(VALUE self)
{
  VALUE channelstruct = rb_iv_get(self, "@realptr");
  return rb_str_new2(rb_rbchannel2cchannel(channelstruct)->chname);
}

static VALUE ChannelStruct_Topic(VALUE self)
{
  VALUE channelstruct = rb_iv_get(self, "@realptr");
  return rb_str_new2(rb_rbchannel2cchannel(channelstruct)->topic);
}

static VALUE ChannelStruct_TopicInfo(VALUE self)
{
  VALUE channelstruct = rb_iv_get(self, "@realptr");
  return rb_str_new2(rb_rbchannel2cchannel(channelstruct)->topic_info);
}

static VALUE ChannelStruct_Mode(VALUE self)
{
  VALUE modeary = rb_ary_new();
  VALUE channelstruct = rb_iv_get(self, "@realptr");

  struct Channel *channel = rb_rbchannel2cchannel(channelstruct);
  rb_ary_push(modeary, INT2NUM(channel->mode.mode));
  rb_ary_push(modeary, INT2NUM(channel->mode.limit));
  rb_ary_push(modeary, rb_str_new2(channel->mode.key));

  return modeary;
}

static void
Init_ChannelStruct(void)
{
  cChannelStruct = rb_define_class("ChannelStruct", rb_cObject);

  rb_define_class_variable(cChannelStruct, "@@realptr", Qnil);

  rb_define_method(cChannelStruct, "initialize", ChannelStruct_Initialize, 1);
  rb_define_method(cChannelStruct, "name", ChannelStruct_Name, 0);
  rb_define_method(cChannelStruct, "topic", ChannelStruct_Topic, 0);
  rb_define_method(cChannelStruct, "topic_info", ChannelStruct_TopicInfo, 0);
  rb_define_method(cChannelStruct, "mode", ChannelStruct_Mode, 0);
}

static void *
rb_cmode_hdlr(va_list args)
{
  struct Client  *client_p = va_arg(args, struct Client*);
  struct Client  *source_p = va_arg(args, struct Client*);
  struct Channel *chptr    = va_arg(args, struct Channel*);
  int             parc     = va_arg(args, int);
  char           **parv    = va_arg(args, char **);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_CMODE);

  if(RARRAY(hooks)->len)
  {
    VALUE params, current, service_name, self, command;
    VALUE fc2params;

    ID command_id;
    int i;

    params = rb_ary_new();

    rb_ary_push(params, rb_cclient2rbclient(client_p));
    rb_ary_push(params, rb_cclient2rbclient(source_p));
    rb_ary_push(params, rb_cchannel2rbchannel(chptr));
    rb_ary_push(params, rb_carray2rbarray(parc, parv));

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

      if(!do_ruby(RB_CALLBACK(rb_singleton_call), fc2params))
      {
        printf("RUBY ERROR: Failed to call: %s\n", StringValueCStr(command));
      }
    }
  }

  return pass_callback(ruby_cmode_hook, client_p, source_p, chptr, parc, parv);
}

static void*
rb_umode_hdlr(va_list args)
{
  struct Client *user = va_arg(args, struct Client *);
  int what = va_arg(args, int);
  int mode = va_arg(args, int);
#if 0
  XXX NEEDS UPDATING: NEW FORMAT: Client WHAT MODE
  struct Client *client_p = va_arg(args, struct Client*);
  struct Client *source_p = va_arg(args, struct Client*);
  int            parc     = va_arg(args, int);
  char         **parv     = va_arg(args, char**);

  VALUE hooks = rb_ary_entry(ruby_server_hooks, RB_HOOKS_UMODE);

  if(RARRAY(hooks)->len)
  {
    VALUE params, current, service_name, self, command;
    VALUE fc2params;
    ID command_id;
    int i;

    params = rb_ary_new();
    rb_ary_push(params, rb_cclient2rbclient(client_p));
    rb_ary_push(params, rb_cclient2rbclient(source_p));
    rb_ary_push(params, rb_carray2rbarray(parc, parv));

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
        printf("RUBY ERROR: Failed to call: %s\n", StringValueCStr(command));
      }
    }
  }
  
#endif
  return pass_callback(ruby_umode_hook, user, what, mode);
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

  snprintf(path, sizeof(path), "%s/%s", dir, fname);

  printf("RUBY INFO: Loading ruby module: %s\n", path);

  if(!do_ruby(RB_CALLBACK(rb_load_file), (VALUE)path))
    return 0;

  ruby_exec();

  strncpy(classname, fname, strlen(fname)-3);

  klass = rb_protect(RB_CALLBACK(rb_path2class), (VALUE)(classname), &status);

  if(ruby_handle_error(status))
    return 0;

  printf("RUBY INFO: Loaded Class %s\n", classname);

  params = rb_ary_new();
  rb_ary_push(params, klass);
  rb_ary_push(params, rb_intern("new"));
  rb_ary_push(params, 0);
  rb_ary_push(params, (VALUE)NULL);

  self = rb_protect(RB_CALLBACK(rb_singleton_call), params, &status);

  if(ruby_handle_error(status))
    return 0;
  printf("RUBY INFO: Initialized Class %s\n", classname);

  service = find_service(classname);
  service->data = (void *)self;

  ruby_cmode_hook = install_hook(on_cmode_change_cb, rb_cmode_hdlr);
  ruby_umode_hook = install_hook(on_umode_change_cb, rb_umode_hdlr);

  return 1;
}

void
sigabrt()
{
  signal(SIGABRT, sigabrt);
  printf("We've encountered a SIGABRT\n");
  ruby_script_error();
}

void
init_ruby(void)
{
  ruby_init();
  ruby_show_version();
  ruby_init_loadpath();
  Init_ServiceModule();
  Init_ClientStruct();
  Init_ChannelStruct();
  signal(SIGINT, SIG_DFL);
  signal(SIGHUP, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);
  signal(SIGABRT, sigabrt);

  ruby_server_hooks = rb_ary_new();
  rb_ary_push(ruby_server_hooks, rb_ary_new());
  rb_ary_push(ruby_server_hooks, rb_ary_new());
}
