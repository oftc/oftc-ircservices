#include <ruby.h>
#include "libruby_module.h"

static VALUE NickStruct_Nick(VALUE);
static VALUE NickStruct_Pass(VALUE);
static VALUE NickStruct_Status(VALUE);
static VALUE NickStruct_Flags(VALUE);
static VALUE NickStruct_Language(VALUE);
static VALUE NickStruct_RegTime(VALUE);
static VALUE NickStruct_LastSeen(VALUE);
static VALUE NickStruct_LastQuit(VALUE);

static VALUE
NickStruct_Initialize(VALUE self, VALUE nick)
{
  rb_iv_set(self, "@realptr", nick);
  return self;
}

static VALUE
NickStruct_Nick(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  if(nick)
    return rb_str_new2(nick->nick);
  else
    return rb_str_new2("");
}

static VALUE
NickStruct_Pass(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_str_new2(nick->pass);
}

static VALUE
NickStruct_Status(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return INT2NUM(nick->status);
}

static VALUE
NickStruct_Flags(VALUE self)
{
  //struct Nick *nick = rb_rbnick2cnick(self);
  // XXX Broken needs updating return INT2NUM(nick->flags);
  return Qnil;
}

static VALUE
NickStruct_Language(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return INT2NUM(nick->language);
}

static VALUE
NickStruct_RegTime(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return INT2NUM(nick->reg_time);
}

static VALUE
NickStruct_LastSeen(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return INT2NUM(nick->last_seen);
}

static VALUE
NickStruct_LastQuit(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_str_new2(nick->last_quit);
}

 void
Init_NickStruct(void)
{
  cNickStruct = rb_define_class("NickStruct", rb_cObject);

  rb_define_class_variable(cNickStruct, "@@realptr", Qnil);

  rb_define_method(cNickStruct, "initialize", NickStruct_Initialize, 1);
  rb_define_method(cNickStruct, "nick", NickStruct_Nick, 0);
  rb_define_method(cNickStruct, "pass", NickStruct_Pass, 0);
  rb_define_method(cNickStruct, "status", NickStruct_Status, 0);
  rb_define_method(cNickStruct, "flags", NickStruct_Flags, 0);
  rb_define_method(cNickStruct, "language", NickStruct_Language, 0);
  rb_define_method(cNickStruct, "reg_time", NickStruct_RegTime, 0);
  rb_define_method(cNickStruct, "last_seen", NickStruct_LastSeen, 0);
  rb_define_method(cNickStruct, "last_quit", NickStruct_LastQuit, 0);
}

struct Nick* rb_rbnick2cnick(VALUE self)
{
  struct Nick* out;
  VALUE nick = rb_iv_get(self, "@realptr");
  Data_Get_Struct(nick, struct Nick, out);
  return out;
}

VALUE
rb_cnick2rbnick(struct Nick *nick)
{
  VALUE fc2params, rbnick, real_nick;
  int status;

  rbnick = Data_Wrap_Struct(rb_cObject, 0, 0, nick);

  fc2params = rb_ary_new();
  rb_ary_push(fc2params, cNickStruct);
  rb_ary_push(fc2params, rb_intern("new"));
  rb_ary_push(fc2params, 1);
  rb_ary_push(fc2params, (VALUE)&rbnick);

  real_nick = rb_protect(RB_CALLBACK(rb_singleton_call), fc2params, &status);

  if(ruby_handle_error(status))
  {
    printf("RUBY ERROR: Ruby Failed To Create NickStruct\n");
    return Qnil;
  }

  return real_nick;
}
