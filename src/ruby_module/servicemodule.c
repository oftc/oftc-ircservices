#include <ruby.h>
#include "libruby_module.h"

/* Core Functions */
static VALUE ServiceModule_register(VALUE, VALUE);
static VALUE ServiceModule_reply_user(VALUE, VALUE, VALUE);
static VALUE ServiceModule_service_name(VALUE, VALUE);
static VALUE ServiceModule_add_hook(VALUE, VALUE);
static VALUE ServiceModule_log(VALUE, VALUE, VALUE);
/* Core Functions */
/* DB Prototypes */
static VALUE ServiceModule_db_set_string(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_db_get_string(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_db_set_number(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_db_set_bool(VALUE, VALUE, VALUE, VALUE);

static VALUE ServiceModule_db_get_nickname_from_id(VALUE, VALUE);
static VALUE ServiceModule_db_get_id_from_name(VALUE, VALUE, VALUE);

static VALUE ServiceModule_db_find_nick(VALUE, VALUE);
static VALUE ServiceModule_db_find_chan(VALUE, VALUE);
static VALUE ServiceModule_db_find_chanaccess(VALUE, VALUE, VALUE);

static VALUE ServiceModule_db_list_add(VALUE, VALUE, VALUE);
static VALUE ServiceModule_db_list_first(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_db_list_next(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_db_list_done(VALUE, VALUE);
static VALUE ServiceModule_db_list_del(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_db_list_del_index(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_exit_client(VALUE, VALUE, VALUE, VALUE);
/* DB Prototypes */

static void m_generic(struct Service *, struct Client *, int, char**);

static VALUE
ServiceModule_register(VALUE self, VALUE commands)
{
  struct Service *ruby_service;
  struct ServiceMessage *generic_msgtab;
  VALUE command, service_name;
  long i;

  service_name = rb_iv_get(self, "@ServiceName");

  ruby_service = make_service(StringValueCStr(service_name));

  clear_serv_tree_parse(&ruby_service->msg_tree);
  dlinkAdd(ruby_service, &ruby_service->node, &services_list);
  hash_add_service(ruby_service);
  introduce_client(ruby_service->name);

  Check_Type(commands, T_ARRAY);

  for(i = RARRAY(commands)->len-1; i >= 0; --i)
  {
    generic_msgtab = MyMalloc(sizeof(struct ServiceMessage));
    command = rb_ary_shift(commands);
    generic_msgtab->cmd = StringValueCStr(command);
    rb_ary_push(commands, command);

    generic_msgtab->handler = m_generic;

    mod_add_servcmd(&ruby_service->msg_tree, generic_msgtab);
  }

  return Qnil;
}

static VALUE
ServiceModule_exit_client(VALUE self, VALUE rbclient, VALUE rbsource, 
    VALUE rbreason)
{
  struct Client *client, *source;
  char *reason;

  client = rb_rbclient2cclient(rbclient);
  source = rb_rbclient2cclient(rbsource);

  reason = StringValueCStr(rbreason);

  exit_client(client, source, reason);
  return Qnil;
}

static VALUE
ServiceModule_reply_user(VALUE self, VALUE rbclient, VALUE rbmessage)
{
  struct Client *client;
  struct Service *service;
  VALUE service_name;

  service_name = rb_iv_get(self, "@ServiceName");
  service = find_service(StringValueCStr(service_name));

  client = rb_rbclient2cclient(rbclient);

  char *message = StringValueCStr(rbmessage);

  reply_user(service, service, client, 0, message);
  return Qnil;
}

static VALUE
ServiceModule_service_name(VALUE self, VALUE name)
{
  return rb_iv_set(self, "@ServiceName", name);
}

static VALUE
ServiceModule_add_hook(VALUE self, VALUE hooks)
{
  if(RARRAY(hooks)->len)
  {
    int i;
    VALUE current, hook, type;
    for(i=0; i < RARRAY(hooks)->len; ++i)
    {
      current = rb_ary_entry(hooks, i);
      type = rb_ary_entry(current, 0);
      hook = rb_ary_entry(current, 1);
      rb_add_hook(self, hook, NUM2INT(type));
    }
  }
  return self;
}

static VALUE
ServiceModule_introduce_server(VALUE self, VALUE server, VALUE gecos)
{
  struct Client *serv = introduce_server(StringValueCStr(server), StringValueCStr(gecos));
  VALUE rbserver;

  rbserver = rb_cclient2rbclient(serv);
  return rbserver;
}

static VALUE
ServiceModule_log(VALUE self, VALUE level, VALUE message)
{
  ilog(NUM2INT(level), StringValueCStr(message));
  return self;
}

static VALUE
ServiceModule_db_set_string(VALUE self, VALUE key, VALUE id, VALUE value)
{
  int ret = db_set_string(NUM2INT(key), NUM2INT(id), StringValueCStr(value));

  if(ret != -1)
    return value;
  else /* FIXME We need a way to raise exceptions */
    return Qnil;
}

static VALUE
ServiceModule_db_get_string(VALUE self, VALUE key, VALUE id, VALUE value)
{
  return rb_str_new2("");
}

static VALUE
ServiceModule_db_set_number(VALUE self, VALUE key, VALUE id, VALUE value)
{
  int ret = db_set_number(NUM2INT(key), NUM2INT(id), NUM2INT(value));
  if(ret != -1)
    return value;
  else /* FIXME Exception needed */
    return Qnil;
}

static VALUE
ServiceModule_db_set_bool(VALUE self, VALUE key, VALUE id, VALUE value)
{
  int ret = db_set_bool(NUM2INT(key), NUM2INT(id), value == Qtrue ? TRUE : FALSE);
  if(ret != -1)
    return value;
  else /* FIXME Exception Needed */
    return Qnil;
}

static VALUE
ServiceModule_db_get_nickname_from_id(VALUE self, VALUE id)
{
  char *ret = db_get_nickname_from_id(NUM2INT(id));
  if(ret)
  {
    VALUE nick = rb_str_new2(ret);
    MyFree(ret);
    return nick;
  }
  else
    return Qnil;
}

static VALUE
ServiceModule_db_get_id_from_name(VALUE self, VALUE name, VALUE type)
{
  int ret = db_get_id_from_name(StringValueCStr(name), NUM2INT(type));

  return INT2NUM(ret);
}

static VALUE
ServiceModule_db_find_nick(VALUE self, VALUE name)
{
  struct Nick *ret = db_find_nick(StringValueCStr(name));
  if(!ret) /* FIXME Exception Needed */
    return Qnil;
  else
  {
    VALUE nick = rb_cnick2rbnick(ret);
    return nick;
  }
}

static VALUE
ServiceModule_db_find_chan(VALUE self, VALUE name)
{
  struct RegChannel *ret = db_find_chan(StringValueCStr(name));
  if(!ret) /* FIXME Exception Needed */
    return Qnil;
  else
  {
    VALUE channel = rb_cchannel2rbchannel(ret);
    return channel;
  }
}

static VALUE
ServiceModule_db_find_chanaccess(VALUE self, VALUE channel, VALUE account)
{
  return self;
}

static VALUE
ServiceModule_db_list_add(VALUE self, VALUE type, VALUE value)
{
  return self;
}

static VALUE
ServiceModule_db_list_first(VALUE self, VALUE type, VALUE param, VALUE entry)
{
  return self;
}

static VALUE
ServiceModule_db_list_next(VALUE self, VALUE result, VALUE type, VALUE entry)
{
  return self;
}

static VALUE
ServiceModule_db_list_done(VALUE self, VALUE result)
{
  return self;
}

static VALUE
ServiceModule_db_list_del(VALUE self, VALUE type, VALUE id, VALUE param)
{
  return self;
}

static VALUE
ServiceModule_db_list_del_index(VALUE self, VALUE type, VALUE id, VALUE index)
{
  return self;
}

void
Init_ServiceModule(void)
{
  cServiceModule = rb_define_class("ServiceModule", rb_cObject);

  rb_define_class_variable(cServiceModule, "@@ServiceName", rb_str_new2(""));

  rb_define_const(cServiceModule, "UMODE_HOOK",  INT2NUM(RB_HOOKS_UMODE));
  rb_define_const(cServiceModule, "CMODE_HOOK",  INT2NUM(RB_HOOKS_CMODE));
  rb_define_const(cServiceModule, "NEWUSR_HOOK", INT2NUM(RB_HOOKS_NEWUSR));

  rb_define_const(cServiceModule, "LOG_CRIT",   INT2NUM(L_CRIT));
  rb_define_const(cServiceModule, "LOG_ERROR",  INT2NUM(L_ERROR));
  rb_define_const(cServiceModule, "LOG_WARN",   INT2NUM(L_WARN));
  rb_define_const(cServiceModule, "LOG_NOTICE", INT2NUM(L_NOTICE));
  rb_define_const(cServiceModule, "LOG_TRACE",  INT2NUM(L_TRACE));
  rb_define_const(cServiceModule, "LOG_INFO",   INT2NUM(L_INFO));
  rb_define_const(cServiceModule, "LOG_DEBUG",  INT2NUM(L_DEBUG));

  rb_define_method(cServiceModule, "register", ServiceModule_register, 1);
  rb_define_method(cServiceModule, "reply_user", ServiceModule_reply_user, 2);
  rb_define_method(cServiceModule, "service_name", ServiceModule_service_name, 1);
  rb_define_method(cServiceModule, "add_hook", ServiceModule_add_hook, 1);
  rb_define_method(cServiceModule, "log", ServiceModule_log, 2);
  rb_define_method(cServiceModule, "introduce_server", ServiceModule_introduce_server, 1);
  rb_define_method(cServiceModule, "exit_client", ServiceModule_exit_client, 3);

  rb_define_method(cServiceModule, "string", ServiceModule_db_set_string, 3);
  rb_define_method(cServiceModule, "string?", ServiceModule_db_get_string, 3);
  rb_define_method(cServiceModule, "number", ServiceModule_db_set_number, 3);
  rb_define_method(cServiceModule, "bool", ServiceModule_db_set_bool, 3);

  rb_define_method(cServiceModule, "nickname_from_id?", ServiceModule_db_get_nickname_from_id, 1);
  rb_define_method(cServiceModule, "id_from_nickname?", ServiceModule_db_get_id_from_name, 2);

  rb_define_method(cServiceModule, "find_nick?", ServiceModule_db_find_nick, 1);
  rb_define_method(cServiceModule, "find_chan?", ServiceModule_db_find_chan, 1);
  rb_define_method(cServiceModule, "find_chanaccess?", ServiceModule_db_find_chanaccess, 2);

  rb_define_method(cServiceModule, "list_add", ServiceModule_db_list_add, 2);
  rb_define_method(cServiceModule, "list_first", ServiceModule_db_list_first, 3);
  rb_define_method(cServiceModule, "list_next", ServiceModule_db_list_next, 3);
  rb_define_method(cServiceModule, "list_done", ServiceModule_db_list_done, 1);
  rb_define_method(cServiceModule, "list_del", ServiceModule_db_list_del, 3);
  rb_define_method(cServiceModule, "list_del_index", ServiceModule_db_list_del_index, 3);
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

  ilog(L_TRACE, "RUBY INFO: Calling Command: %s From %s", command, client->name);

  if(!do_ruby(RB_CALLBACK(rb_singleton_call), fc2params))
  {
    reply_user(service, service, client, 0, 
        "An error has occurred, please be patient and report this bug");
    ilog(L_NOTICE, "Ruby Failed to Execute Command: %s by %s", command, client->name);
  }
}
