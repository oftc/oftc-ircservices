#include <ruby.h>
#include "libruby_module.h"

static VALUE cServiceModule = Qnil;

/* Core Functions */
static VALUE ServiceModule_register(VALUE, VALUE);
static VALUE ServiceModule_reply_user(VALUE, VALUE, VALUE);
static VALUE ServiceModule_service_name(VALUE, VALUE);
static VALUE ServiceModule_add_hook(VALUE, VALUE);
static VALUE ServiceModule_log(VALUE, VALUE, VALUE);
static VALUE ServiceModule_do_help(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_exit_client(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_introduce_server(VALUE, VALUE, VALUE);
static VALUE ServiceModule_unload(VALUE);
static VALUE ServiceModule_join_channel(VALUE, VALUE);
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
/* DB Prototypes */

static void m_generic(struct Service *, struct Client *, int, char**);

static struct Service* get_service(VALUE self);
static void set_service(VALUE, struct Service *); 

static struct Service *
get_service(VALUE self)
{
  struct Service *service;

  VALUE rbservice = rb_iv_get(self, "@service_ptr");
  Data_Get_Struct(rbservice, struct Service, service);

  return service;
}

static void
set_service(VALUE self, struct Service *service)
{
  VALUE rbservice = Data_Wrap_Struct(rb_cObject, 0, 0, service);
  rb_iv_set(self, "@service_ptr", rbservice);
}

static VALUE
ServiceModule_register(VALUE self, VALUE commands)
{
  struct Service *ruby_service;
  struct ServiceMessage *generic_msgtab;
  VALUE command, service_name;
  long i;

  service_name = rb_iv_get(self, "@ServiceName");

  ruby_service = make_service(StringValueCStr(service_name));

  set_service(self, ruby_service);

  if(ircncmp(ruby_service->name, StringValueCStr(service_name), NICKLEN) != 0)
    rb_iv_set(self, "@ServiceName", rb_str_new2(ruby_service->name));

  clear_serv_tree_parse(&ruby_service->msg_tree);
  dlinkAdd(ruby_service, &ruby_service->node, &services_list);
  hash_add_service(ruby_service);
  introduce_client(ruby_service->name);

  Check_Type(commands, T_ARRAY);

  for(i = RARRAY(commands)->len-1; i >= 0; --i)
  {
    VALUE name, param_min, param_max, flags, access, hlp_shrt, hlp_long;
    char *tmp;

    command = rb_ary_shift(commands);
    Check_Type(command, T_ARRAY);

    name = rb_ary_shift(command);
    param_min = rb_ary_shift(command);
    param_max = rb_ary_shift(command);
    flags = rb_ary_shift(command);
    access = rb_ary_shift(command);
    hlp_shrt = rb_ary_shift(command);
    hlp_long = rb_ary_shift(command);

    generic_msgtab = MyMalloc(sizeof(struct ServiceMessage));
    DupString(tmp, StringValueCStr(name));

    generic_msgtab->cmd = tmp;
    generic_msgtab->parameters = NUM2INT(param_min);
    generic_msgtab->maxpara = NUM2INT(param_max);
    generic_msgtab->flags = NUM2INT(flags);
    generic_msgtab->access = NUM2INT(access);
    generic_msgtab->help_short = NUM2INT(hlp_shrt);
    generic_msgtab->help_long = NUM2INT(hlp_long);

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
ServiceModule_reply_user(VALUE self, VALUE rbclient, VALUE message)
{
  struct Client *client = rb_rbclient2cclient(rbclient);
  struct Service *service = get_service(self);

  reply_user(service, service, client, 0, StringValueCStr(message));

  return self;
}

static VALUE
ServiceModule_service_name(VALUE self, VALUE name)
{
  return rb_iv_set(self, "@ServiceName", name);
}

static VALUE
ServiceModule_add_hook(VALUE self, VALUE hooks)
{
  if(RARRAY(hooks)->len > 0)
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
  struct Client *serv;
  VALUE rbserver;
  const char* name = StringValueCStr(server);
  const char* cgecos = StringValueCStr(gecos);

  serv = introduce_server(name, cgecos);

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
ServiceModule_do_help(VALUE self, VALUE client, VALUE value, VALUE parv)
{
  struct Service *service = get_service(self);
  struct Client *cclient = rb_rbclient2cclient(client);
  char *cvalue = StringValueCStr(value);
  int argc = RARRAY(parv)->len;
  char **argv = rb_rbarray2carray(parv);

  do_help(service, cclient, cvalue, argc, argv);

  return self;
}

static VALUE
ServiceModule_unload(VALUE self)
{
  /* place holder, maybe one day we'll have things we need to free here */
  return self;
}

static VALUE
ServiceModule_join_channel(VALUE self, VALUE channame)
{
  struct Service *service = get_service(self);
  struct Client *client = find_client(service->name);
  const char* chname = StringValueCStr(channame);
  struct Channel *channel = join_channel(client, chname);
  return rb_cchannel2rbchannel(channel);
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
  if(ret != NULL)
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
  if(ret == NULL) /* FIXME Exception Needed */
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
  if(ret == NULL) /* FIXME Exception Needed */
    return Qnil;
  else
  {
    VALUE channel = rb_cregchan2rbregchan(ret);
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
  VALUE cServiceBase = rb_path2class("ServiceBase");
  cServiceModule = rb_define_class("ServiceModule", cServiceBase);

  rb_define_class_variable(cServiceModule, "@@ServiceName", rb_str_new2(""));

  rb_define_const(cServiceModule, "UMODE_HOOK",  INT2NUM(RB_HOOKS_UMODE));
  rb_define_const(cServiceModule, "CMODE_HOOK",  INT2NUM(RB_HOOKS_CMODE));
  rb_define_const(cServiceModule, "NEWUSR_HOOK", INT2NUM(RB_HOOKS_NEWUSR));
  rb_define_const(cServiceModule, "PRIVMSG_HOOK", INT2NUM(RB_HOOKS_PRIVMSG));
  rb_define_const(cServiceModule, "JOIN_HOOK", INT2NUM(RB_HOOKS_JOIN));
  rb_define_const(cServiceModule, "NICK_HOOK", INT2NUM(RB_HOOKS_NICK));
  rb_define_const(cServiceModule, "NOTICE_HOOK", INT2NUM(RB_HOOKS_NOTICE));

  rb_define_const(cServiceModule, "LOG_CRIT",   INT2NUM(L_CRIT));
  rb_define_const(cServiceModule, "LOG_ERROR",  INT2NUM(L_ERROR));
  rb_define_const(cServiceModule, "LOG_WARN",   INT2NUM(L_WARN));
  rb_define_const(cServiceModule, "LOG_NOTICE", INT2NUM(L_NOTICE));
  rb_define_const(cServiceModule, "LOG_TRACE",  INT2NUM(L_TRACE));
  rb_define_const(cServiceModule, "LOG_INFO",   INT2NUM(L_INFO));
  rb_define_const(cServiceModule, "LOG_DEBUG",  INT2NUM(L_DEBUG));

  rb_define_const(cServiceModule, "MFLG_SLOW", INT2NUM(MFLG_SLOW));
  rb_define_const(cServiceModule, "MFLG_UNREG", INT2NUM(MFLG_UNREG));
  rb_define_const(cServiceModule, "SFLG_UNREGOK", INT2NUM(SFLG_UNREGOK));
  rb_define_const(cServiceModule, "SFLG_ALIAS", INT2NUM(SFLG_ALIAS));
  rb_define_const(cServiceModule, "SFLG_KEEPARG", INT2NUM(SFLG_KEEPARG));
  rb_define_const(cServiceModule, "SFLG_CHANARG", INT2NUM(SFLG_CHANARG));
  rb_define_const(cServiceModule, "SFLG_NICKARG", INT2NUM(SFLG_NICKARG));
  rb_define_const(cServiceModule, "SFLG_NOMAXPARAM", INT2NUM(SFLG_NOMAXPARAM));

  rb_define_const(cServiceModule, "USER_FLAG", INT2NUM(USER_FLAG));
  rb_define_const(cServiceModule, "IDNETIFIED_FLAG", INT2NUM(IDENTIFIED_FLAG));
  rb_define_const(cServiceModule, "OPER_FLAG", INT2NUM(OPER_FLAG));
  rb_define_const(cServiceModule, "ADMIN_FLAG", INT2NUM(ADMIN_FLAG));
  rb_define_const(cServiceModule, "SUDO_FLAG", INT2NUM(SUDO_FLAG));

  rb_define_method(cServiceModule, "register", ServiceModule_register, 1);
  rb_define_method(cServiceModule, "reply_user", ServiceModule_reply_user, 2);
  rb_define_method(cServiceModule, "service_name", ServiceModule_service_name, 1);
  rb_define_method(cServiceModule, "add_hook", ServiceModule_add_hook, 1);
  rb_define_method(cServiceModule, "log", ServiceModule_log, 2);
  rb_define_method(cServiceModule, "introduce_server", ServiceModule_introduce_server, 2);
  rb_define_method(cServiceModule, "exit_client", ServiceModule_exit_client, 3);
  rb_define_method(cServiceModule, "do_help", ServiceModule_do_help, 3);
  rb_define_method(cServiceModule, "unload", ServiceModule_unload, 0);
  rb_define_method(cServiceModule, "join_channel", ServiceModule_join_channel, 1);

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
  VALUE real_client, self;
  ID class_command;

  strupr(command);
  class_command = rb_intern(command);
 
  rbparams = rb_ary_new();

  real_client = rb_cclient2rbclient(client);
  rbparv = rb_carray2rbarray(parc, parv);

  rb_ary_push(rbparams, real_client);
  rb_ary_push(rbparams, rbparv);

  self = (VALUE)service->data;

  ilog(L_TRACE, "RUBY INFO: Calling Command: %s From %s", command, client->name);

  if(!do_ruby(self, class_command, 2, real_client, rbparv))
  {
    reply_user(service, service, client, 0, 
        "An error has occurred, please be patient and report this bug");
    ilog(L_NOTICE, "Ruby Failed to Execute Command: %s by %s", command, client->name);
  }
}
