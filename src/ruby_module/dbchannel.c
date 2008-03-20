#include <ruby.h>
#include "libruby_module.h"
#include "dbm.h"

VALUE cDBChannel = Qnil;

static VALUE initialize(VALUE, VALUE);
static VALUE id(VALUE);
static VALUE id_set(VALUE, VALUE);
static VALUE name(VALUE);
static VALUE name_set(VALUE, VALUE);
static VALUE description(VALUE);
static VALUE description_set(VALUE, VALUE);
static VALUE entrymsg(VALUE);
static VALUE entrymsg_set(VALUE, VALUE);
static VALUE url(VALUE);
static VALUE url_set(VALUE, VALUE);
static VALUE email(VALUE);
static VALUE email_set(VALUE, VALUE);
static VALUE topic(VALUE);
static VALUE topic_set(VALUE, VALUE);
static VALUE mlock(VALUE);
static VALUE mlock_set(VALUE, VALUE);
static VALUE priv(VALUE);
static VALUE priv_set(VALUE, VALUE);
static VALUE restricted(VALUE);
static VALUE restricted_set(VALUE, VALUE);
static VALUE topiclock(VALUE);
static VALUE topiclock_set(VALUE, VALUE);
static VALUE verbose(VALUE);
static VALUE verbose_set(VALUE, VALUE);
static VALUE autolimit(VALUE);
static VALUE autolimit_set(VALUE, VALUE);
static VALUE expirebans(VALUE);
static VALUE expirebans_set(VALUE, VALUE);
static VALUE floodserv(VALUE);
static VALUE floodserv_set(VALUE, VALUE);

static inline VALUE m_get_flag(VALUE, char(*)(DBChannel *));
static inline VALUE m_get_string(VALUE, const char *(*)(DBChannel *));

static inline VALUE m_set_flag(VALUE, VALUE, int(*)(DBChannel *, char));
static inline VALUE m_set_string(VALUE, VALUE, int(*)(DBChannel *, const char *));

static inline VALUE
m_get_flag(VALUE self, char(*get_func)(DBChannel *))
{
  DBChannel *channel = value_to_dbchannel(self);
  return get_func(channel) == TRUE ? Qtrue : Qfalse;
}

static inline VALUE
m_get_string(VALUE self, const char *(*get_func)(DBChannel *))
{
  DBChannel *channel = value_to_dbchannel(self);
  const char *ptr = get_func(channel);
  if(ptr == NULL)
    return Qnil;
  else
    return rb_str_new2(ptr);
}

static inline VALUE
m_set_flag(VALUE self, VALUE value, int (*set_func)(DBChannel *, char))
{
  DBChannel *channel = value_to_dbchannel(self);
  /* TODO XXX FIXME check for FALSE and throw exception */
  set_func(channel, value == Qtrue ? 1 : 0) == TRUE ? Qtrue : Qfalse;
  return value;
}

static inline VALUE
m_set_string(VALUE self, VALUE value, int (*set_func)(DBChannel *, const char *))
{
  DBChannel *channel = value_to_dbchannel(self);
  Check_Type(value, T_STRING);
  /* TODO XXX FIXME check for FALSE and throw exception */
  set_func(channel, StringValueCStr(value));
  return value;
}

static VALUE
initialize(VALUE self, VALUE channel)
{
  rb_iv_set(self, "@realptr", channel);
  return self;
}

static VALUE
id(VALUE self)
{
  DBChannel *channel = value_to_dbchannel(self);
  return INT2FIX(dbchannel_get_id(channel));
}

static VALUE
id_set(VALUE self, VALUE value)
{
  DBChannel *channel = value_to_dbchannel(self);
  dbchannel_set_id(channel, FIX2INT(value));
  return value;
}

static VALUE
name(VALUE self)
{
  return m_get_string(self, &dbchannel_get_channel);
}

static VALUE
name_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &dbchannel_set_channel);
}

static VALUE
description(VALUE self)
{
  return m_get_string(self, &dbchannel_get_description);
}

static VALUE
description_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &dbchannel_set_description);
}

static VALUE
entrymsg(VALUE self)
{
  return m_get_string(self, &dbchannel_get_entrymsg);
}

static VALUE
entrymsg_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &dbchannel_set_entrymsg);
}

static VALUE
url(VALUE self)
{
  return m_get_string(self, &dbchannel_get_url);
}

static VALUE
url_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &dbchannel_set_url);
}

static VALUE
email(VALUE self)
{
  return m_get_string(self, &dbchannel_get_email);
}

static VALUE
email_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &dbchannel_set_email);
}

static VALUE
topic(VALUE self)
{
  return m_get_string(self, &dbchannel_get_topic);
}

static VALUE
topic_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &dbchannel_set_topic);
}

static VALUE
mlock(VALUE self)
{
  return m_get_string(self, &dbchannel_get_mlock);
}

static VALUE
mlock_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &dbchannel_set_mlock);
}

static VALUE
priv(VALUE self)
{
  return m_get_flag(self, &dbchannel_get_priv);
}

static VALUE
priv_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &dbchannel_set_priv);
}

static VALUE
restricted(VALUE self)
{
  return m_get_flag(self, &dbchannel_get_restricted);
}

static VALUE
restricted_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &dbchannel_set_restricted);
}

static VALUE
topiclock(VALUE self)
{
  return m_get_flag(self, &dbchannel_get_topic_lock);
}

static VALUE
topiclock_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &dbchannel_set_topic_lock);
}

static VALUE
verbose(VALUE self)
{
  return m_get_flag(self, &dbchannel_get_verbose);
}

static VALUE
verbose_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &dbchannel_set_verbose);
}

static VALUE
autolimit(VALUE self)
{
  return m_get_flag(self, &dbchannel_get_autolimit);
}

static VALUE
autolimit_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &dbchannel_set_autolimit);
}

static VALUE
expirebans(VALUE self)
{
  return m_get_flag(self, &dbchannel_get_expirebans);
}

static VALUE
expirebans_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &dbchannel_set_expirebans);
}

static VALUE
floodserv(VALUE self)
{
  return m_get_flag(self, &dbchannel_get_floodserv);
}

static VALUE
floodserv_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &dbchannel_set_floodserv);
}

void
Init_DBChannel(void)
{
  cDBChannel = rb_define_class("RegChannel", rb_cObject);

  rb_gc_register_address(&cDBChannel);

  rb_define_method(cDBChannel, "initialize", initialize, 1);
  rb_define_method(cDBChannel, "id", id, 0);
  rb_define_method(cDBChannel, "id=", id_set, 1);
  rb_define_method(cDBChannel, "name", name, 0);
  rb_define_method(cDBChannel, "name=", name_set, 1);
  rb_define_method(cDBChannel, "description", description, 0);
  rb_define_method(cDBChannel, "description=", description_set, 1);
  rb_define_method(cDBChannel, "entrymsg", entrymsg, 0);
  rb_define_method(cDBChannel, "entrymsg=", entrymsg_set, 1);
  rb_define_method(cDBChannel, "url", url, 0);
  rb_define_method(cDBChannel, "url=", url_set, 1);
  rb_define_method(cDBChannel, "email", email, 0);
  rb_define_method(cDBChannel, "email=", email_set, 1);
  rb_define_method(cDBChannel, "topic", topic, 0);
  rb_define_method(cDBChannel, "topic=", topic_set, 1);
  rb_define_method(cDBChannel, "mlock", mlock, 0);
  rb_define_method(cDBChannel, "mlock=", mlock_set, 1);
  rb_define_method(cDBChannel, "private?", priv, 0);
  rb_define_method(cDBChannel, "private=", priv_set, 1);
  rb_define_method(cDBChannel, "restricted?", restricted, 0);
  rb_define_method(cDBChannel, "restricted=", restricted_set, 1);
  rb_define_method(cDBChannel, "topic_lock?", topiclock, 0);
  rb_define_method(cDBChannel, "topic_lock=", topiclock_set, 1);
  rb_define_method(cDBChannel, "verbose?", verbose, 0);
  rb_define_method(cDBChannel, "verbose=", verbose_set, 1);
  rb_define_method(cDBChannel, "autolimit?", autolimit, 0);
  rb_define_method(cDBChannel, "autolimit=", autolimit_set, 1);
  rb_define_method(cDBChannel, "expirebans?", expirebans, 0);
  rb_define_method(cDBChannel, "expirebans=", expirebans_set, 1);
  rb_define_method(cDBChannel, "floodserv?", floodserv, 0);
  rb_define_method(cDBChannel, "floodserv=", floodserv_set, 1);
}

DBChannel*
value_to_dbchannel(VALUE self)
{
  DBChannel* out;
  VALUE channel = rb_iv_get(self, "@realptr");
  Data_Get_Struct(channel, DBChannel, out);
  return out;
}

VALUE
dbchannel_to_value(DBChannel *channel)
{
  VALUE rbchannel, real_channel;

  rbchannel = Data_Wrap_Struct(rb_cObject, 0, 0, channel);
  real_channel = do_ruby_ret(cDBChannel, rb_intern("new"), 1, rbchannel);

  if(real_channel == Qnil)
  {
    ilog(L_DEBUG, "RUBY ERROR: Ruby Failed To Create RegChannelStruct");
    return Qnil;
  }

  rb_gc_register_address(&real_channel);
  rb_gc_register_address(&rbchannel);

  return real_channel;
}

