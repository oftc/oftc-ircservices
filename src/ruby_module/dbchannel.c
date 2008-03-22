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
static VALUE autoop(VALUE);
static VALUE autoop_set(VALUE, VALUE);
static VALUE autovoice(VALUE);
static VALUE autovoice_set(VALUE, VALUE);
static VALUE leaveops(VALUE);
static VALUE leaveops_set(VALUE, VALUE);
static VALUE expirebans_lifetime(VALUE);
static VALUE expirebans_lifetime_set(VALUE, VALUE);

/* Actions */
static VALUE m_register(VALUE, VALUE);
static VALUE delete(VALUE);

/* Lists */
static VALUE masters_each(VALUE);
static VALUE masters_count(VALUE);

/* Static Methods */
static VALUE find(VALUE, VALUE);
static VALUE forbid(VALUE, VALUE);
static VALUE unforbid(VALUE, VALUE);
static VALUE is_forbid(VALUE, VALUE);
static VALUE list_forbid_each(VALUE);
static VALUE list_all_each(VALUE);
static VALUE list_regular_each(VALUE);

static inline VALUE m_get_flag(VALUE, char(*)(DBChannel *));
static inline VALUE m_get_string(VALUE, const char *(*)(DBChannel *));

static inline VALUE m_set_flag(VALUE, VALUE, int(*)(DBChannel *, char));
static inline VALUE m_set_string(VALUE, VALUE, int(*)(DBChannel *, const char *));

static void m_string_list_each(int(*)(dlink_list *), void(*)(dlink_list *));

void
Init_DBChannel(void)
{
  cDBChannel = rb_define_class("DBChannel", rb_cObject);

  rb_gc_register_address(&cDBChannel);

  /* Get/Setters */
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
  rb_define_method(cDBChannel, "autoop?", autoop, 0);
  rb_define_method(cDBChannel, "autoop=", autoop_set, 1);
  rb_define_method(cDBChannel, "autovoice?", autovoice, 0);
  rb_define_method(cDBChannel, "autovoice=", autovoice_set, 1);
  rb_define_method(cDBChannel, "leaveops?", leaveops, 0);
  rb_define_method(cDBChannel, "leaveops=", leaveops_set, 1);
  rb_define_method(cDBChannel, "expirebans_lifetime", expirebans_lifetime, 0);
  rb_define_method(cDBChannel, "expirebans_lifetime=", expirebans_lifetime_set, 1);

  /* Actions */
  rb_define_method(cDBChannel, "register", m_register, 1);
  rb_define_method(cDBChannel, "delete", delete, 0);

  /* Lists */
  rb_define_method(cDBChannel, "masters_each", masters_each, 0);
  rb_define_method(cDBChannel, "masters_count", masters_count, 0);

  /* Static Methods */
  rb_define_singleton_method(cDBChannel, "find", find, 1);
  rb_define_singleton_method(cDBChannel, "forbid", forbid, 1);
  rb_define_singleton_method(cDBChannel, "unforbid", unforbid, 1);
  rb_define_singleton_method(cDBChannel, "is_forbid?", is_forbid, 1);
  rb_define_singleton_method(cDBChannel, "list_forbid_each", list_forbid_each, 0);
  rb_define_singleton_method(cDBChannel, "list_all_each", list_all_each, 0);
  rb_define_singleton_method(cDBChannel, "list_regular_each", list_regular_each, 0);
}

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

static VALUE
autoop(VALUE self)
{
  return m_get_flag(self, &dbchannel_get_autoop);
}

static VALUE
autoop_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &dbchannel_set_autoop);
}

static VALUE
autovoice(VALUE self)
{
  return m_get_flag(self, &dbchannel_get_autovoice);
}

static VALUE
autovoice_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &dbchannel_set_autovoice);
}

static VALUE
leaveops(VALUE self)
{
  return m_get_flag(self, &dbchannel_get_leaveops);
}

static VALUE
leaveops_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &dbchannel_set_leaveops);
}

static VALUE
expirebans_lifetime(VALUE self)
{
  DBChannel *chan = value_to_dbchannel(self);
  return UINT2NUM(dbchannel_get_expirebans_lifetime(chan));
}

static VALUE
expirebans_lifetime_set(VALUE self, VALUE value)
{
  DBChannel *chan = value_to_dbchannel(self);
  return dbchannel_set_expirebans_lifetime(chan, NUM2UINT(value)) == TRUE ? Qtrue : Qfalse;
}

static VALUE
m_register(VALUE self, VALUE nick)
{
  /* TODO XXX FIXME not implemented */
  return Qfalse;
}

static VALUE
delete(VALUE self)
{
  DBChannel *chan = value_to_dbchannel(self);
  return dbchannel_delete(chan) == TRUE ? Qtrue : Qfalse;
}

static VALUE
masters_each(VALUE self)
{
  if(rb_block_given_p())
  {
    dlink_node *ptr = NULL, *next_ptr = NULL;
    dlink_list list = { 0 };
    char *master = NULL;
    DBChannel *chan = value_to_dbchannel(self);

    dbchannel_masters_list(dbchannel_get_id(chan), &list);

    DLINK_FOREACH_SAFE(ptr, next_ptr, list.head)
    {
      master = (char *)ptr->data;
      rb_yield(rb_str_new2(master));
    }

    dbchannel_masters_list_free(&list);
  }

  return self;
}

static VALUE
masters_count(VALUE self)
{
  DBChannel *chan = value_to_dbchannel(self);
  int count = 0;
  if(dbchannel_masters_count(dbchannel_get_id(chan), &count))
    return INT2NUM(count);
  else /* TODO XXX FIXME exception? */
    return INT2NUM(0);
}

static VALUE
find(VALUE self, VALUE name)
{
  DBChannel *chan = dbchannel_find(StringValueCStr(name));
  if(chan == NULL)
    return Qnil;
  else
    return dbchannel_to_value(chan);
}

static VALUE
forbid(VALUE self, VALUE name)
{
  return dbchannel_forbid(StringValueCStr(name)) == TRUE ? Qtrue : Qfalse;
}

static VALUE
unforbid(VALUE self, VALUE name)
{
  return dbchannel_delete_forbid(StringValueCStr(name)) == TRUE ? Qtrue : Qfalse;
}

static VALUE
is_forbid(VALUE self, VALUE name)
{
  return dbchannel_is_forbid(StringValueCStr(name)) == TRUE ? Qtrue : Qfalse;
}

static VALUE
list_forbid_each(VALUE self)
{
  if(rb_block_given_p())
  {
    m_string_list_each(&dbchannel_list_forbid, &dbchannel_list_forbid_free);
  }
  return self;
}

static VALUE
list_all_each(VALUE self)
{
  if(rb_block_given_p())
  {
    m_string_list_each(&dbchannel_list_all, &dbchannel_list_all_free);
  }

  return self;
}

static VALUE
list_regular_each(VALUE self)
{
  if(rb_block_given_p())
  {
    m_string_list_each(&dbchannel_list_regular, &dbchannel_list_regular_free);
  }

  return self;
}

static void
m_string_list_each(int(*list_func)(dlink_list *), void(*free_func)(dlink_list*))
{
  dlink_node *ptr = NULL, *next_ptr = NULL;
  dlink_list list = { 0 };
  char *str = NULL;

  list_func(&list);

  DLINK_FOREACH_SAFE(ptr, next_ptr, list.head)
  {
    str = (char *)ptr->data;
    rb_yield(rb_str_new2(str));
  }

  free_func(&list);
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

