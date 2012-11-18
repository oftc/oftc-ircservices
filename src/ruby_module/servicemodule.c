#include <ruby.h>
#include <evdns.h>
#include <arpa/inet.h>
#include "libruby_module.h"
#include "servicemask.h"
#include "akill.h"
#include "send.h"
#include "kill.h"

VALUE cServiceModule = Qnil;
VALUE cClient;

/* Core Functions */
static VALUE ServiceModule_register(VALUE, VALUE);
static VALUE ServiceModule_service_name(VALUE, VALUE);
static VALUE ServiceModule_add_hook(VALUE, VALUE);
static VALUE ServiceModule_log(VALUE, VALUE, VALUE);
static VALUE ServiceModule_do_help(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_introduce_server(VALUE, VALUE, VALUE);
static VALUE ServiceModule_unload(VALUE);
static VALUE ServiceModule_chain_language(VALUE, VALUE);
static VALUE ServiceModule_akill_add(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_kill_user(VALUE, VALUE, VALUE);
static VALUE ServiceModule_load_language(VALUE, VALUE);
static VALUE ServiceModule_lm(VALUE, VALUE);
static VALUE ServiceModule_drop_nick(VALUE, VALUE);
static VALUE ServiceModule_add_event(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_send_cmode(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_dns_lookup(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_dns_lookup_reverse(VALUE, VALUE, VALUE, VALUE);
static VALUE reply(VALUE, VALUE, VALUE);
/* Core Functions */

static void m_generic(struct Service *, struct Client *, int, char**);

struct Service* get_service(VALUE self);
static void set_service(VALUE, struct Service *); 

struct Service *
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
  struct Service *ruby_service = get_service(self);
  struct ServiceMessage *generic_msgtab;
  VALUE command;
  long i;

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
ServiceModule_service_name(VALUE self, VALUE name)
{
  struct Service *ruby_service;
  struct Client *ruby_client;

  Check_Type(name, T_STRING);

  rb_iv_set(self, "@ServiceName", name);

  ruby_service = make_service(StringValueCStr(name));

  set_service(self, ruby_service);

  if(ircncmp(ruby_service->name, StringValueCStr(name), NICKLEN) != 0)
    rb_iv_set(self, "@ServiceName", rb_str_new2(ruby_service->name));

  clear_serv_tree_parse(&ruby_service->msg_tree);
  dlinkAdd(ruby_service, &ruby_service->node, &services_list);
  hash_add_service(ruby_service);
  ruby_client = introduce_client(ruby_service->name, ruby_service->name, TRUE);

  rb_iv_set(self, "@client", client_to_value(ruby_client));

  rb_iv_set(self, "@langpath", rb_str_new2(LANGPATH));

  return name;
}

static VALUE
ServiceModule_add_hook(VALUE self, VALUE hooks)
{
  Check_Type(hooks, T_ARRAY);

  if(RARRAY(hooks)->len > 0)
  {
    int i;
    VALUE current, hook, type;
    for(i=0; i < RARRAY(hooks)->len; ++i)
    {
      current = rb_ary_entry(hooks, i);

      Check_Type(current, T_ARRAY);

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
  const char* name;
  const char* cgecos;

  Check_Type(server, T_STRING);
  Check_Type(gecos, T_STRING);

  name = StringValueCStr(server);
  cgecos = StringValueCStr(gecos);

  serv = introduce_server(name, cgecos);

  rbserver = client_to_value(serv);
  return rbserver;
}

static VALUE
ServiceModule_log(VALUE self, VALUE level, VALUE message)
{
  VALUE service_name = rb_iv_get(self, "@ServiceName");
  Check_Type(message, T_STRING);

  ilog(NUM2INT(level), "{%s} %s", StringValueCStr(service_name), StringValueCStr(message));

  return self;
}

static VALUE
ServiceModule_do_help(VALUE self, VALUE client, VALUE value, VALUE parv)
{
  struct Service *service = get_service(self);
  struct Client *cclient;
  int argc = 0;
  int i;
  char *cvalue = 0;
  char **argv = 0;
  VALUE tmp;

  Check_OurType(client, cClient);
  cclient = value_to_client(client);

  if(!NIL_P(value))
  {
    Check_Type(value, T_STRING);
    Check_Type(parv, T_ARRAY);

    DupString(cvalue, StringValueCStr(value));

    argc = RARRAY(parv)->len - 1;
    argv = ALLOCA_N(char *, argc);

    for(i = 0; i < argc; ++i)
    {
      tmp = rb_ary_entry(parv, i);
      DupString(argv[i], StringValueCStr(tmp));
    }
  }

  do_help(service, cclient, cvalue, argc, argv);

  if(argc > 0)
  {
    for(i = 0; i < argc; ++i)
      MyFree(argv[i]);
  }

  MyFree(cvalue);

  return self;
}

static VALUE
ServiceModule_unload(VALUE self)
{
  /* place holder, maybe one day we'll have things we need to free here */
  return self;
}

static VALUE
ServiceModule_chain_language(VALUE self, VALUE langfile)
{
  struct Service *service = get_service(self);

  Check_Type(langfile, T_STRING);

  load_language(service->languages, StringValueCStr(langfile));

  return self;
}

static VALUE
ServiceModule_akill_add(VALUE self, VALUE mask, VALUE reason, VALUE duration)
{
  struct Service *service = get_service(self);
  struct Client *client = value_to_client(rb_iv_get(self, "@client"));
  const char *creason;
  const char *cmask;
  struct ServiceMask *akill;

  Check_Type(mask, T_STRING);
  cmask = StringValueCStr(mask);

  if((akill = akill_find(cmask)) != NULL)
  {
    free_servicemask(akill);
    return Qfalse;
  }

  if(!valid_wild_card(cmask))
    return Qfalse;

  Check_Type(reason, T_STRING);
  creason = StringValueCStr(reason);

  akill = MyMalloc(sizeof(struct ServiceMask));

  akill->time_set = CurrentTime;
  akill->duration = NUM2INT(duration);
  DupString(akill->mask, cmask);
  DupString(akill->reason, creason);

  if(!akill_add(akill))
  {
    ilog(L_NOTICE, "%s Failed to add akill on %s", client->name, cmask);
    free_servicemask(akill);
    return Qfalse;
  }
  else
  {
    ilog(L_NOTICE, "%s Added akill on %s because %s for %ld seconds",
      client->name, cmask, creason, akill->duration);
    send_akill(service, client->name, akill);
    free_servicemask(akill);
    return Qtrue;
  }
}

static VALUE
ServiceModule_kill_user(VALUE self, VALUE who, VALUE reason)
{
  struct Service *service = get_service(self);
  struct Client *client = value_to_client(rb_iv_get(self, "@client"));
  const char *creason;

  Check_Type(reason, T_STRING);
  creason = StringValueCStr(reason);

  Check_OurType(who, cClient);
  client = value_to_client(who);

  kill_user(service, client, creason);

  return Qtrue;
}

static VALUE
ServiceModule_load_language(VALUE self, VALUE file)
{
  VALUE lang_map = rb_iv_get(self, "@lang_map");
  VALUE sv_langs = rb_iv_get(self, "@sv_langs");
  VALUE curr = Qnil;
  VALUE last = Qnil;
  size_t count = 1;
  int langid;
  FBFILE *cfile;
  char buffer[256];

  if(lang_map == Qnil)
  {
    lang_map = rb_hash_new();
    rb_iv_set(self, "@lang_map", lang_map);
  }

  if(sv_langs == Qnil)
  {
    sv_langs = rb_hash_new();
    rb_iv_set(self, "@sv_langs", sv_langs);
  }

  snprintf(buffer, sizeof(buffer), "%s/%s.lang", LANGPATH, StringValueCStr(file));

  if((cfile = fbopen(buffer, "r")) == NULL)
  {
    rb_raise(rb_eIOError, "Failed to open file %s", buffer);
    return Qfalse;
  }

  fbgets(buffer, sizeof(buffer), cfile);
  langid = atoi(buffer);

  curr = rb_hash_aref(sv_langs, langid);

  if(curr == Qnil)
  {
    curr = rb_hash_new();
    rb_hash_aset(sv_langs, langid, curr);
  }

  while(fbgets(buffer, sizeof(buffer), cfile) != NULL)
  {
    if(buffer[0] == '\t')
    {
      VALUE line = rb_str_new2(buffer);
      VALUE this = rb_hash_aref(curr, last);
      rb_str_append(this, rb_str_substr(line, 1, RSTRING_LEN(line)-1));
      rb_hash_aset(curr, last, this);
    }
    else
    {
      VALUE line = rb_funcall2(rb_str_new2(buffer), rb_intern("chomp"), 0, 0);
      rb_hash_aset(lang_map, line, INT2NUM(count));
      rb_hash_aset(curr, line, rb_str_new2(""));
      last = line;
      ++count;
    }
  }

  fbclose(cfile);

  ServiceModule_chain_language(self, file);
  return Qtrue;
}

static VALUE
ServiceModule_lm(VALUE self, VALUE mid)
{
  VALUE lang_map = rb_iv_get(self, "@lang_map");
  VALUE entry = rb_hash_aref(lang_map, mid);

  if(entry == Qnil)
  {
    VALUE sn = rb_iv_get(self, "@ServiceName");
    ilog(L_DEBUG, "{%s} Failed to find help string id: %s", StringValueCStr(sn), StringValueCStr(mid));
    return INT2NUM(0);
  }
  else
    return entry;
}

static VALUE
ServiceModule_ctcp_user(VALUE self, VALUE user, VALUE message)
{
  struct Client *client = value_to_client(user);
  if(me.uplink != NULL && !IsConnecting(me.uplink))
    ctcp_user(get_service(self), client, StringValueCStr(message));
  return self;
}

static VALUE
ServiceModule_sendto_channel(VALUE self, VALUE channel, VALUE message)
{
  sendto_channel(get_service(self), value_to_channel(channel), StringValueCStr(message));
  return self;
}

static VALUE
ServiceModule_drop_nick(VALUE self, VALUE nickname)
{
  if(drop_nickname(get_service(self), NULL, StringValueCStr(nickname)))
    return Qtrue;
  else
    return Qfalse;
}

static VALUE
ServiceModule_add_event(VALUE self, VALUE method, VALUE time, VALUE arg)
{
  return rb_add_event(self, method, time, arg);
}

static VALUE
ServiceModule_delete_event(VALUE self, VALUE event)
{
  return rb_delete_event(self, event);
}

static VALUE
ServiceModule_send_cmode(VALUE self, VALUE channel, VALUE mode, VALUE param)
{
  struct Service *service = get_service(self);
  struct Channel *target = value_to_channel(channel);
  char *cmode = NULL, *cparam = NULL;

  if(!NIL_P(param))
    cmode = StringValueCStr(mode);

  if(!NIL_P(param))
    cparam = StringValueCStr(param);

  send_cmode(service, target, cmode, cparam);
  return self;
}

static VALUE
ServiceModule_send_raw(VALUE self, VALUE msg)
{
  Check_Type(msg, T_STRING);
  sendto_server(me.uplink, StringValueCStr(msg));
  return self;
}

static VALUE
ServiceModule_client(VALUE self)
{
  return rb_iv_get(self, "@client");
}

static VALUE
reply(VALUE self, VALUE target, VALUE message)
{
  struct Service *service = get_service(self);
  struct Client *client = value_to_client(target);

  reply_user(service, service, client, 0, StringValueCStr(message));

  return self;
}

static void
lookup_callback(int result, char type, int count, int ttl, void *addresses,
  void *arg)
{
  VALUE *values = (VALUE *)arg;
  VALUE cb = rb_ary_entry(*values, 0);
  VALUE args = rb_ary_entry(*values, 1);
  VALUE results = rb_ary_new();
  VALUE *parv;
  int status;
  struct ruby_args rargs;

  rb_gc_unregister_address(values);
  MyFree(values);

  ilog(L_DEBUG, "lookup_callback hit, result: %d, type: %d, count: %d", result, (int)type, count);

  if(result == DNS_ERR_NONE && count > 0)
  {
    int i;

    for(i = 0; i < count; i++)
    {
      VALUE r;

      if(type == DNS_PTR)
      {
        r = rb_str_new2(((char **)addresses)[i]);
      }
      else if(type == DNS_IPv4_A)
      {
        struct in_addr *addrs = addresses;
        r = rb_str_new2(inet_ntoa(addrs[i]));
      }
      else if(type == DNS_IPv6_AAAA)
      {
        struct in6_addr *addrs = addresses;
        char buf[INET6_ADDRSTRLEN+1];
        const char *a = inet_ntop(AF_INET6, &addrs[i], buf, sizeof(buf));
        r = rb_str_new2(a);
      }

      rb_ary_push(results, r);
    }
  }

  parv = ALLOCA_N(VALUE, 2);
  parv[0] = results;
  parv[1] = args;

  ilog(L_DEBUG, "lookup Preparing to dispatch ruby cb");

  rargs.recv = cb;
  rargs.id = rb_intern("call");
  rargs.parc = 2;
  rargs.parv = parv;

  rb_protect(rb_singleton_call, (VALUE)&rargs, &status);
  ruby_handle_error(status);
}

static VALUE
ServiceModule_dns_lookup(VALUE self, VALUE host, VALUE cb, VALUE arg)
{
  VALUE *params = ALLOC(VALUE);
  *params = rb_ary_new();
  rb_ary_push(*params, cb);
  rb_ary_push(*params, arg);
  rb_gc_register_address(params);
  return INT2NUM(dns_resolve_host(StringValueCStr(host), &lookup_callback, (void *)params, 0));
}

static VALUE
ServiceModule_dns_lookup_reverse(VALUE self, VALUE host, VALUE cb, VALUE arg)
{
  VALUE *params = ALLOC(VALUE);
  *params = rb_ary_new();
  rb_ary_push(*params, cb);
  rb_ary_push(*params, arg);
  rb_gc_register_address(params);
  return INT2NUM(dns_resolve_ip(StringValueCStr(host), &lookup_callback, (void *)params));
}

void
Init_ServiceModule(void)
{
  //VALUE cServiceBase = rb_path2class("ServiceBase");
  cServiceModule = rb_define_class("ServiceModule", rb_cObject);

  rb_define_const(cServiceModule, "UMODE_HOOK",  INT2NUM(RB_HOOKS_UMODE));
  rb_define_const(cServiceModule, "CMODE_HOOK",  INT2NUM(RB_HOOKS_CMODE));
  rb_define_const(cServiceModule, "NEWUSR_HOOK", INT2NUM(RB_HOOKS_NEWUSR));
  rb_define_const(cServiceModule, "PRIVMSG_HOOK", INT2NUM(RB_HOOKS_PRIVMSG));
  rb_define_const(cServiceModule, "JOIN_HOOK", INT2NUM(RB_HOOKS_JOIN));
  rb_define_const(cServiceModule, "PART_HOOK", INT2NUM(RB_HOOKS_PART));
  rb_define_const(cServiceModule, "QUIT_HOOK", INT2NUM(RB_HOOKS_QUIT));
  rb_define_const(cServiceModule, "NICK_HOOK", INT2NUM(RB_HOOKS_NICK));
  rb_define_const(cServiceModule, "NOTICE_HOOK", INT2NUM(RB_HOOKS_NOTICE));
  rb_define_const(cServiceModule, "CHAN_CREATED_HOOK", INT2NUM(RB_HOOKS_CHAN_CREATED));
  rb_define_const(cServiceModule, "CHAN_DELETED_HOOK", INT2NUM(RB_HOOKS_CHAN_DELETED));
  rb_define_const(cServiceModule, "CTCP_HOOK", INT2NUM(RB_HOOKS_CTCP));
  rb_define_const(cServiceModule, "CHAN_REG_HOOK", INT2NUM(RB_HOOKS_CHAN_REG));
  rb_define_const(cServiceModule, "NICK_REG_HOOK", INT2NUM(RB_HOOKS_NICK_REG));
  rb_define_const(cServiceModule, "DB_INIT_HOOK", INT2NUM(RB_HOOKS_DB_INIT));
  rb_define_const(cServiceModule, "EOB_HOOK", INT2NUM(RB_HOOKS_EOB));

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

  rb_define_const(cServiceModule, "CONFIG_PATH", rb_str_new2(SYSCONFDIR));

  rb_define_method(cServiceModule, "register", ServiceModule_register, 1);
  rb_define_method(cServiceModule, "service_name", ServiceModule_service_name, 1);
  rb_define_method(cServiceModule, "add_hook", ServiceModule_add_hook, 1);
  rb_define_method(cServiceModule, "log", ServiceModule_log, 2);
  rb_define_method(cServiceModule, "introduce_server", ServiceModule_introduce_server, 2);
  rb_define_method(cServiceModule, "do_help", ServiceModule_do_help, 3);
  rb_define_method(cServiceModule, "unload", ServiceModule_unload, 0);
  rb_define_method(cServiceModule, "chain_language", ServiceModule_chain_language, 1);
  rb_define_method(cServiceModule, "akill_add", ServiceModule_akill_add, 3);
  rb_define_method(cServiceModule, "ctcp_user", ServiceModule_ctcp_user, 2);
  rb_define_method(cServiceModule, "sendto_channel", ServiceModule_sendto_channel, 2);
  rb_define_method(cServiceModule, "kill_user", ServiceModule_kill_user, 2);

  rb_define_method(cServiceModule, "load_language", ServiceModule_load_language, 1);
  rb_define_method(cServiceModule, "lm", ServiceModule_lm, 1);

  rb_define_method(cServiceModule, "drop_nick", ServiceModule_drop_nick, 1);
  rb_define_method(cServiceModule, "add_event", ServiceModule_add_event, 3);
  rb_define_method(cServiceModule, "delete_event", ServiceModule_delete_event, 1);
  rb_define_method(cServiceModule, "send_cmode", ServiceModule_send_cmode, 3);

  rb_define_method(cServiceModule, "send_raw", ServiceModule_send_raw, 1);

  rb_define_method(cServiceModule, "client", ServiceModule_client, 0);
  rb_define_method(cServiceModule, "reply", reply, 2);

  rb_define_method(cServiceModule, "dns_lookup", ServiceModule_dns_lookup, 3);
  rb_define_method(cServiceModule, "dns_lookup_reverse", ServiceModule_dns_lookup_reverse, 3);
}

static void
m_generic(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  char *command = service->last_command;
  VALUE rbparams, rbparv;
  VALUE real_client, self;
  ID class_command;

  strupper(command);
  class_command = rb_intern(command);

  rbparams = rb_ary_new();

  real_client = client_to_value(client);
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
