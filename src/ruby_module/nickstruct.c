#include <ruby.h>
#include "libruby_module.h"

static VALUE cNickStruct = Qnil;

static VALUE NickStruct_Initialize(VALUE, VALUE);
static VALUE NickStruct_Nick(VALUE);
static VALUE NickStruct_NickSet(VALUE, VALUE);
static VALUE NickStruct_Pass(VALUE);
static VALUE NickStruct_PassSet(VALUE, VALUE);
static VALUE NickStruct_Salt(VALUE);
static VALUE NickStruct_SaltSet(VALUE, VALUE);
static VALUE NickStruct_Cloak(VALUE);
static VALUE NickStruct_CloakSet(VALUE, VALUE);
static VALUE NickStruct_Email(VALUE);
static VALUE NickStruct_EmailSet(VALUE, VALUE);
static VALUE NickStruct_Url(VALUE);
static VALUE NickStruct_UrlSet(VALUE, VALUE);
static VALUE NickStruct_LastRealName(VALUE);
static VALUE NickStruct_LastRealNameSet(VALUE, VALUE);
static VALUE NickStruct_LastHost(VALUE);
static VALUE NickStruct_LastHostSet(VALUE, VALUE);
static VALUE NickStruct_LastQuit(VALUE);
static VALUE NickStruct_LastQuitSet(VALUE, VALUE);
static VALUE NickStruct_Status(VALUE);
static VALUE NickStruct_StatusSet(VALUE, VALUE);
static VALUE NickStruct_Enforce(VALUE);
static VALUE NickStruct_EnforceSet(VALUE, VALUE);
static VALUE NickStruct_Secure(VALUE);
static VALUE NickStruct_SecureSet(VALUE, VALUE);
static VALUE NickStruct_Verified(VALUE);
static VALUE NickStruct_VerifiedSet(VALUE, VALUE);
static VALUE NickStruct_CloakOn(VALUE);
static VALUE NickStruct_CloakOnSet(VALUE, VALUE);
static VALUE NickStruct_Admin(VALUE);
static VALUE NickStruct_AdminSet(VALUE, VALUE);
static VALUE NickStruct_EmailVerified(VALUE);
static VALUE NickStruct_EmailVerifiedSet(VALUE, VALUE);
static VALUE NickStruct_Priv(VALUE);
static VALUE NickStruct_PrivSet(VALUE, VALUE);
static VALUE NickStruct_Language(VALUE);
static VALUE NickStruct_LanguageSet(VALUE, VALUE);
static VALUE NickStruct_RegTime(VALUE);
static VALUE NickStruct_RegTimeSet(VALUE, VALUE);
static VALUE NickStruct_LastSeenTime(VALUE);
static VALUE NickStruct_LastSeenTimeSet(VALUE, VALUE);
static VALUE NickStruct_LastQuitTime(VALUE);
static VALUE NickStruct_LastQuitTimeSet(VALUE, VALUE);
static VALUE NickStruct_NickRegTime(VALUE);
static VALUE NickStruct_NickRegTimeSet(VALUE, VALUE);

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
NickStruct_NickSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  /* TODO check length < NICKLEN */
  strlcpy(nick->nick, StringValueCStr(value), sizeof(nick->nick));
  return value;
}

static VALUE
NickStruct_Pass(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_str_new2(nick->pass);
}

static VALUE
NickStruct_PassSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  /* TODO check length < PASSLEN */
  strlcpy(nick->pass, StringValueCStr(value), sizeof(nick->pass));
  return value;
}

static VALUE
NickStruct_Salt(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_str_new2(nick->salt);
}

static VALUE
NickStruct_SaltSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  /* TODO check < SALTLEN */
  strlcpy(nick->salt, StringValueCStr(value), sizeof(nick->salt));
  return value;
}

static VALUE
NickStruct_Cloak(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_str_new2(nick->cloak);
}

static VALUE
NickStruct_CloakSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  /* TODO check < HOSTLEN */
  strlcpy(nick->cloak, StringValueCStr(value), sizeof(nick->cloak));
  return value;
}

static VALUE
NickStruct_Email(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_str_new2(nick->email);
}

static VALUE
NickStruct_EmailSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  DupString(nick->email, StringValueCStr(value));
  return value;
}

static VALUE
NickStruct_Url(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_str_new2(nick->url);
}

static VALUE
NickStruct_UrlSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  DupString(nick->url, StringValueCStr(value));
  return value;
}

static VALUE
NickStruct_LastRealName(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_str_new2(nick->last_realname);
}

static VALUE
NickStruct_LastRealNameSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  DupString(nick->last_realname, StringValueCStr(value));
  return value;
}

static VALUE
NickStruct_LastHost(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_str_new2(nick->last_host);
}

static VALUE
NickStruct_LastHostSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  DupString(nick->last_host, StringValueCStr(value));
  return value;
}

static VALUE
NickStruct_LastQuit(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_str_new2(nick->last_quit);
}

static VALUE
NickStruct_LastQuitSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  DupString(nick->last_quit, StringValueCStr(value));
  return value;
}

static VALUE
NickStruct_Status(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return UINT2NUM(nick->status);
}

static VALUE
NickStruct_StatusSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  nick->status = NUM2UINT(value);
  return value;
}

static VALUE
NickStruct_Enforce(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return nick->enforce ? Qtrue : Qfalse;
}

static VALUE
NickStruct_EnforceSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  nick->enforce = value == Qtrue ? 1 : 0;
  return value;
}

static VALUE
NickStruct_Secure(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return nick->secure ? Qtrue : Qfalse;
}

static VALUE
NickStruct_SecureSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  nick->secure = value == Qtrue ? 1 : 0;
  return value;
}

static VALUE
NickStruct_Verified(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return nick->verified ? Qtrue : Qfalse;
}

static VALUE
NickStruct_VerifiedSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  nick-> verified = value == Qtrue ? 1 : 0;
  return value;
}

static VALUE
NickStruct_CloakOn(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return nick->cloak_on ? Qtrue : Qfalse;
}

static VALUE
NickStruct_CloakOnSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  nick->cloak_on = value == Qtrue ? 1 : 0;
  return value;
}

static VALUE
NickStruct_Admin(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return nick->admin ? Qtrue : Qfalse;
}

static VALUE
NickStruct_AdminSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  nick->admin = value == Qtrue ? 1 : 0;
  return value;
}

static VALUE
NickStruct_EmailVerified(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return nick->email_verified ? Qtrue : Qfalse;
}

static VALUE
NickStruct_EmailVerifiedSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  nick->email_verified = value == Qtrue ? 1 : 0;
  return value;
}

static VALUE
NickStruct_Priv(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return nick->priv ? Qtrue : Qfalse;
}

static VALUE
NickStruct_PrivSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  nick->priv = value == Qtrue ? 1 : 0;
  return value;
}

static VALUE
NickStruct_Language(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return UINT2NUM(nick->language);
}

static VALUE
NickStruct_LanguageSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  nick->language = NUM2INT(value);
  return value;
}

static VALUE
NickStruct_RegTime(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_time_new(nick->reg_time, (VALUE)NULL);
}

static VALUE
NickStruct_RegTimeSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  nick->reg_time = NUM2INT(value);
  return value;
}

static VALUE
NickStruct_LastSeenTime(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_time_new(nick->last_seen, (VALUE)NULL);
}

static VALUE
NickStruct_LastSeenTimeSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  nick->last_seen = NUM2INT(value);
  return value;
}

static VALUE
NickStruct_LastQuitTime(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_time_new(nick->last_quit_time, (VALUE)NULL);
}

static VALUE
NickStruct_LastQuitTimeSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  nick->last_quit_time = NUM2INT(value);
  return value;
}

static VALUE
NickStruct_NickRegTime(VALUE self)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  return rb_time_new(nick->nick_reg_time, (VALUE)NULL);
}

static VALUE
NickStruct_NickRegTimeSet(VALUE self, VALUE value)
{
  struct Nick *nick = rb_rbnick2cnick(self);
  nick->nick_reg_time = NUM2INT(value);
  return value;
}

 void
Init_NickStruct(void)
{
  cNickStruct = rb_define_class("NickStruct", rb_cObject);

  rb_define_class_variable(cNickStruct, "@@realptr", Qnil);

  rb_define_method(cNickStruct, "initialize", NickStruct_Initialize, 1);
  rb_define_method(cNickStruct, "nick", NickStruct_Nick, 0);
  rb_define_method(cNickStruct, "nick=", NickStruct_NickSet, 1);
  rb_define_method(cNickStruct, "pass", NickStruct_Pass, 0);
  rb_define_method(cNickStruct, "pass=", NickStruct_PassSet, 1);
  rb_define_method(cNickStruct, "salt", NickStruct_Salt, 0);
  rb_define_method(cNickStruct, "salt=", NickStruct_SaltSet, 1);
  rb_define_method(cNickStruct, "cloak", NickStruct_Cloak, 0);
  rb_define_method(cNickStruct, "cloak=", NickStruct_CloakSet, 1);
  rb_define_method(cNickStruct, "email", NickStruct_Email, 0);
  rb_define_method(cNickStruct, "email=", NickStruct_EmailSet, 1);
  rb_define_method(cNickStruct, "url", NickStruct_Url, 0);
  rb_define_method(cNickStruct, "url=", NickStruct_UrlSet, 1);
  rb_define_method(cNickStruct, "last_realname", NickStruct_LastRealName, 0);
  rb_define_method(cNickStruct, "last_realname=", NickStruct_LastRealNameSet, 1);
  rb_define_method(cNickStruct, "last_host", NickStruct_LastHost, 0);
  rb_define_method(cNickStruct, "last_host=", NickStruct_LastHostSet, 1);
  rb_define_method(cNickStruct, "last_quit", NickStruct_LastQuit, 0);
  rb_define_method(cNickStruct, "last_quit=", NickStruct_LastQuitSet, 1);
  rb_define_method(cNickStruct, "status", NickStruct_Status, 0);
  rb_define_method(cNickStruct, "status=", NickStruct_StatusSet, 1);
  rb_define_method(cNickStruct, "enforce?", NickStruct_Enforce, 0);
  rb_define_method(cNickStruct, "enforce=", NickStruct_EnforceSet, 1);
  rb_define_method(cNickStruct, "secure?", NickStruct_Secure, 0);
  rb_define_method(cNickStruct, "secure=", NickStruct_SecureSet, 1);
  rb_define_method(cNickStruct, "verified?", NickStruct_Verified, 0);
  rb_define_method(cNickStruct, "verfied", NickStruct_VerifiedSet, 1);
  rb_define_method(cNickStruct, "cloaked?", NickStruct_CloakOn, 0);
  rb_define_method(cNickStruct, "cloaked=", NickStruct_CloakOnSet, 1);
  rb_define_method(cNickStruct, "admin?", NickStruct_Admin, 0);
  rb_define_method(cNickStruct, "admin=", NickStruct_AdminSet, 1);
  rb_define_method(cNickStruct, "email_verified?", NickStruct_EmailVerified, 0);
  rb_define_method(cNickStruct, "email_verified=", NickStruct_EmailVerifiedSet, 1);
  rb_define_method(cNickStruct, "private?", NickStruct_Priv, 0);
  rb_define_method(cNickStruct, "private=", NickStruct_PrivSet, 1);
  rb_define_method(cNickStruct, "language", NickStruct_Language, 0);
  rb_define_method(cNickStruct, "language=", NickStruct_LanguageSet, 1);
  rb_define_method(cNickStruct, "reg_time", NickStruct_RegTime, 0);
  rb_define_method(cNickStruct, "reg_time=", NickStruct_RegTimeSet, 1);
  rb_define_method(cNickStruct, "last_seen", NickStruct_LastSeenTime, 0);
  rb_define_method(cNickStruct, "last_seen=", NickStruct_LastSeenTimeSet, 1);
  rb_define_method(cNickStruct, "last_quit_time", NickStruct_LastQuitTime, 0);
  rb_define_method(cNickStruct, "last_quit_time=", NickStruct_LastQuitTimeSet, 1);
  rb_define_method(cNickStruct, "nick_reg_time", NickStruct_NickRegTime, 0);
  rb_define_method(cNickStruct, "nick_reg_time=", NickStruct_NickRegTimeSet, 1);
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
