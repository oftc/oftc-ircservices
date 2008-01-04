#include <ruby.h>
#include "libruby_module.h"

VALUE cServiceModule = Qnil;
VALUE cClient;

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
static VALUE ServiceModule_part_channel(VALUE, VALUE, VALUE);
static VALUE ServiceModule_chain_language(VALUE, VALUE);
static VALUE ServiceModule_channels_each(VALUE);
static VALUE ServiceModule_regchan_by_name(VALUE, VALUE);
static VALUE ServiceModule_akill_add(VALUE, VALUE, VALUE, VALUE);
static VALUE ServiceModule_load_language(VALUE, VALUE);
static VALUE ServiceModule_lm(VALUE, VALUE);
static VALUE ServiceModule_nickname_delete(VALUE, VALUE);
/* Core Functions */

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
ServiceModule_exit_client(VALUE self, VALUE rbclient, VALUE rbsource,
    VALUE rbreason)
{
  struct Client *client, *source;
  char *reason;

  Check_OurType(rbclient, cClient);
  Check_OurType(rbsource, cClient);
  Check_Type(rbreason, T_STRING);

  client = value_to_client(rbclient);
  source = value_to_client(rbsource);

  reason = StringValueCStr(rbreason);

  exit_client(client, source, reason);
  return Qnil;
}

static VALUE
ServiceModule_reply_user(VALUE self, VALUE rbclient, VALUE message)
{
  struct Client *client;
  struct Service *service = get_service(self);

  Check_OurType(rbclient, cClient);
  Check_Type(message, T_STRING);

  client = value_to_client(rbclient);

  reply_user(service, service, client, 0, StringValueCStr(message));

  return self;
}

static VALUE
ServiceModule_service_name(VALUE self, VALUE name)
{
  struct Service *ruby_service;

  Check_Type(name, T_STRING);

  rb_iv_set(self, "@ServiceName", name);

  ruby_service = make_service(StringValueCStr(name));

  set_service(self, ruby_service);

  if(ircncmp(ruby_service->name, StringValueCStr(name), NICKLEN) != 0)
    rb_iv_set(self, "@ServiceName", rb_str_new2(ruby_service->name));

  clear_serv_tree_parse(&ruby_service->msg_tree);
  dlinkAdd(ruby_service, &ruby_service->node, &services_list);
  hash_add_service(ruby_service);
  introduce_client(ruby_service->name, ruby_service->name, TRUE);

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
  VALUE service_name = rb_class_name(RBASIC(self)->klass);
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
  const char* chname;
  struct Channel *channel;

  Check_Type(channame, T_STRING);
  chname = StringValueCStr(channame);

  channel = hash_find_channel(chname);

  if(channel == NULL)
    channel = make_channel(chname);

  join_channel(client, channel);

  return rb_cchannel2rbchannel(channel);
}

static VALUE
ServiceModule_part_channel(VALUE self, VALUE channame, VALUE reason)
{
  struct Service *service = get_service(self);
  struct Client *client = find_client(service->name);
  const char* chname;
  char creason[KICKLEN+1];

  Check_Type(channame, T_STRING);

  chname = StringValueCStr(channame);

  creason[0] = '\0';

  if(!NIL_P(reason))
  {
    Check_Type(reason, T_STRING);
    strlcpy(creason, StringValueCStr(reason), sizeof(creason));
  }

  part_channel(client, chname, creason);

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
ServiceModule_channels_each(VALUE self)
{
  dlink_node *ptr = NULL, *next_ptr = NULL;

  if(rb_block_given_p())
  {
    /* TODO wrap in protect/ensure */
    DLINK_FOREACH_SAFE(ptr, next_ptr, global_channel_list.head)
      rb_yield(rb_cchannel2rbchannel(ptr->data));
  }

  return self;
}

static VALUE
ServiceModule_akill_add(VALUE self, VALUE mask, VALUE reason, VALUE duration)
{
  struct Service *service = get_service(self);
  struct Client *client = find_client(service->name);
  const char *creason;
  const char *cmask;
  struct ServiceBan *akill;

  Check_Type(mask, T_STRING);
  cmask = StringValueCStr(mask);

  if((akill = db_find_akill(cmask)) != NULL)
  {
    free_serviceban(akill);
    return Qfalse;
  }

  if(!valid_wild_card(cmask))
    return Qfalse;

  Check_Type(reason, T_STRING);
  creason = StringValueCStr(reason);

  if(client->nickname == NULL)
    client->nickname = db_find_nick(client->name);

  if(client->nickname == NULL)
  {
    ilog(L_CRIT, "%s Trying to akill but doesn't have a NickStruct", client->name);
    return Qfalse;
  }

  akill = akill_add(service, client, cmask, creason, NUM2INT(duration));

  if(akill == NULL)
    return Qfalse;
  else
  {
    free_serviceban(akill);
    return Qtrue;
  }
}

static VALUE
ServiceModule_regchan_by_name(VALUE self, VALUE name)
{
  struct RegChannel *channel = NULL;
  Check_Type(name, T_STRING);

  channel = db_find_chan(StringValueCStr(name));

  if(channel)
    return rb_cregchan2rbregchan(channel);
  else
    return Qnil;
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
  return rb_hash_aref(lang_map, mid);
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
  sendto_channel(get_service(self), rb_rbchannel2cchannel(channel), StringValueCStr(message));
  return self;
}

static VALUE
ServiceModule_nickname_delete(VALUE self, VALUE user)
{
  struct Client *client = value_to_client(user);
  if(client->nickname &&
    db_delete_nick(client->nickname->id, client->nickname->nickid,
      client->nickname->pri_nickid))
  {
    ClearIdentified(client);
    client->nickname = NULL;
    client->access = USER_FLAG;
    send_umode(get_service(self), client, "-R");
    return Qtrue;
  }
  else
  {
    return Qfalse;
  }
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
  rb_define_method(cServiceModule, "part_channel", ServiceModule_part_channel, 2);
  rb_define_method(cServiceModule, "chain_language", ServiceModule_chain_language, 1);
  rb_define_method(cServiceModule, "channels_each", ServiceModule_channels_each, 0);
  rb_define_method(cServiceModule, "akill_add", ServiceModule_akill_add, 3);
  rb_define_method(cServiceModule, "ctcp_user", ServiceModule_ctcp_user, 2);
  rb_define_method(cServiceModule, "sendto_channel", ServiceModule_sendto_channel, 2);

  rb_define_method(cServiceModule, "regchan_by_name?", ServiceModule_regchan_by_name, 1);
  rb_define_method(cServiceModule, "load_language", ServiceModule_load_language, 1);
  rb_define_method(cServiceModule, "lm", ServiceModule_lm, 1);

  rb_define_method(cServiceModule, "nickname_delete", ServiceModule_nickname_delete, 1);
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
