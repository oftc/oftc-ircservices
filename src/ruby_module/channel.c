#include <ruby.h>
#include "libruby_module.h"

VALUE cChannel = Qnil;

static VALUE initialize(VALUE, VALUE);
static VALUE name(VALUE);
static VALUE name_set(VALUE, VALUE);
static VALUE topic(VALUE);
static VALUE topic_set(VALUE, VALUE);
static VALUE topicinfo(VALUE);
static VALUE topicinfo_set(VALUE, VALUE);
static VALUE mode(VALUE);
static VALUE mode_set(VALUE, VALUE);
static VALUE regchan(VALUE);
static VALUE regchan_set(VALUE, VALUE);
static VALUE members_length(VALUE);
static VALUE members_each(VALUE);
static VALUE find(VALUE, VALUE);

void
Init_Channel(void)
{
  cChannel = rb_define_class("Channel", rb_cObject);

  rb_define_method(cChannel, "initialize", initialize, 1);
  rb_define_method(cChannel, "name", name, 0);
  rb_define_method(cChannel, "name=", name_set, 1);
  rb_define_method(cChannel, "topic", topic, 0);
  rb_define_method(cChannel, "topic=", topic_set, 1);
  rb_define_method(cChannel, "topic_info", topicinfo, 0);
  rb_define_method(cChannel, "topic_info=", topicinfo_set, 1);
  rb_define_method(cChannel, "mode", mode, 0);
  rb_define_method(cChannel, "mode=", mode_set, 1);
  rb_define_method(cChannel, "regchan", regchan, 0);
  rb_define_method(cChannel, "regchan=", regchan_set, 1);
  rb_define_method(cChannel, "members_length", members_length, 0);
  rb_define_method(cChannel, "members_each", members_each, 0);

  /*TODO members, invites, bans, excepts, invex */

  rb_define_singleton_method(cChannel, "find", find, 1);
}

static VALUE
initialize(VALUE self, VALUE channel)
{
  rb_iv_set(self, "@realptr", channel);
  return self;
}

static VALUE
name(VALUE self)
{
  struct Channel *channel = value_to_channel(self);
  return rb_str_new2(channel->chname);
}

static VALUE
name_set(VALUE self, VALUE name)
{
  struct Channel *channel = value_to_channel(self);
  const char* cvalue;

  Check_Type(name, T_STRING);

  cvalue = StringValueCStr(name);

  strlcpy(channel->chname, cvalue, sizeof(channel->chname));
  return name;
}

static VALUE
topic(VALUE self)
{
  struct Channel *channel = value_to_channel(self);
  return rb_str_new2(channel->topic);
}

static VALUE
topic_set(VALUE self, VALUE value)
{
  struct Channel *channel = value_to_channel(self);

  Check_Type(value, T_STRING);

  DupString(channel->topic, StringValueCStr(value));
  return value;
}

static VALUE
topicinfo(VALUE self)
{
  struct Channel *channel = value_to_channel(self);
  return rb_str_new2(channel->topic_info);
}

static VALUE
topicinfo_set(VALUE self, VALUE value)
{
  struct Channel *channel = value_to_channel(self);

  Check_Type(value, T_STRING);

  DupString(channel->topic_info, StringValueCStr(value));
  return value;
}

static VALUE
mode(VALUE self)
{
  /* TODO Channel Mode */
  return Qnil;
}

static VALUE
mode_set(VALUE self, VALUE value)
{
  /* TODO Channel Mode */
  return Qnil;
}

static VALUE
regchan(VALUE self)
{
  struct Channel *channel = value_to_channel(self);
  if(channel->regchan)
  {
    VALUE regchan = dbchannel_to_value(channel->regchan);
    return regchan;
  }
  else
  {
    return Qnil;
  }
}

static VALUE
regchan_set(VALUE self, VALUE value)
{
  struct Channel *channel = value_to_channel(self);

  if(!NIL_P(value))
  {
    Check_OurType(value, cDBChannel);
    channel->regchan = value_to_dbchannel(value);
  }
  else
  {
    if(channel->regchan)
      ilog(L_CRIT, "ChannelStruct trying to set regchan to NIL but regchan exists, possible memory leak, ignoring request");
  }
  return value;
}

static VALUE
members_length(VALUE self)
{
  struct Channel *channel = value_to_channel(self);
  return ULONG2NUM(channel->members.length);
}

static VALUE
members_each(VALUE self)
{
  struct Channel *channel = value_to_channel(self);
  dlink_node *ptr = NULL, *next_ptr = NULL;

  if(rb_block_given_p())
  {
    /* TODO Wrap in protect/ensure */
    DLINK_FOREACH_SAFE(ptr, next_ptr, channel->members.head)
    {
      struct Membership *ms = ptr->data;
      rb_yield(client_to_value(ms->client_p));
    }
  }

  return self;
}

static VALUE
find(VALUE self, VALUE name)
{
  struct Channel *channel = hash_find_channel(StringValueCStr(name));
  if(channel == NULL)
    return Qnil;
  else
    return channel_to_value(channel);
}

struct Channel*
value_to_channel(VALUE self)
{
  struct Channel* out;
  VALUE channel = rb_iv_get(self, "@realptr");
  Data_Get_Struct(channel, struct Channel, out);
  return out;
}

VALUE
channel_to_value(struct Channel *channel)
{
  VALUE rbchannel, real_channel;

  if(channel == NULL)
  {
    ilog(L_CRIT, "Evil channel pointer is NULL");
    return Qnil;
  }

  rbchannel = Data_Wrap_Struct(rb_cObject, 0, 0, channel);
  real_channel = do_ruby_ret(cChannel, rb_intern("new"), 1, rbchannel);

  if(real_channel == Qnil)
  {
    ilog(L_DEBUG, "RUBY ERROR: Ruby Failed To Create ChannelStruct");
    return Qnil;
  }

  return real_channel;
}

