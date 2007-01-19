#include <ruby.h>
#include "libruby_module.h"

static VALUE ChannelStruct_Initialize(VALUE, VALUE);
static VALUE ChannelStruct_Name(VALUE);
static VALUE ChannelStruct_Topic(VALUE);
static VALUE ChannelStruct_TopicInfo(VALUE);
static VALUE ChannelStruct_Mode(VALUE);

static VALUE
ChannelStruct_Initialize(VALUE self, VALUE channel)
{
  rb_iv_set(self, "@realptr", channel);
  return self;
}

static VALUE ChannelStruct_Name(VALUE self)
{
  struct RegChannel *channel = rb_rbchannel2cchannel(self);
  return rb_str_new2(channel->channel);
}

static VALUE ChannelStruct_Topic(VALUE self)
{
  struct RegChannel *channel = rb_rbchannel2cchannel(self);
  return rb_str_new2(channel->topic);
}

static VALUE ChannelStruct_TopicInfo(VALUE self)
{
  /*struct RegChannel *channel = rb_rbchannel2cchannel(self);
  return rb_str_new2(channel->topic_info);*/
  return Qnil;
}

static VALUE ChannelStruct_Mode(VALUE self)
{
  /*VALUE modeary = rb_ary_new();
  struct RegChannel *channel = rb_rbchannel2cchannel(self);

  rb_ary_push(modeary, INT2NUM(channel->mode.mode));
  rb_ary_push(modeary, INT2NUM(channel->mode.limit));
  rb_ary_push(modeary, rb_str_new2(channel->mode.key));

  return modeary;*/
  return Qnil;
}

void
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

struct RegChannel* rb_rbchannel2cchannel(VALUE self)
{
  struct RegChannel* out;
  VALUE channel = rb_iv_get(self, "@realptr");
  Data_Get_Struct(channel, struct RegChannel, out);
  return out;
}

VALUE
rb_cchannel2rbchannel(struct RegChannel *channel)
{
  VALUE fc2params, rbchannel, real_channel;
  int status;

  rbchannel = Data_Wrap_Struct(rb_cObject, 0, free_regchan, channel);

  fc2params = rb_ary_new();
  rb_ary_push(fc2params, cChannelStruct);
  rb_ary_push(fc2params, rb_intern("new"));
  rb_ary_push(fc2params, 1);
  rb_ary_push(fc2params, (VALUE)&rbchannel);

  real_channel = rb_protect(RB_CALLBACK(rb_singleton_call), fc2params, &status);

  if(ruby_handle_error(status))
  {
    ilog(L_DEBUG, "RUBY ERROR: Ruby Failed To Create ChannelStruct");
    return Qnil;
  }

  return real_channel;
}

