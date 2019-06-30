#include <ruby.h>
#include "libruby_module.h"

VALUE cNickname = Qnil;

static VALUE initialize(VALUE, VALUE);
static VALUE account_id(VALUE);
static VALUE nickname_id(VALUE);
static VALUE primary_nickname_id(VALUE);
static VALUE nick(VALUE);
static VALUE nick_set(VALUE, VALUE);
static VALUE pass(VALUE);
static VALUE pass_set(VALUE, VALUE);
static VALUE salt(VALUE);
static VALUE salt_set(VALUE, VALUE);
static VALUE cloak(VALUE);
static VALUE cloak_set(VALUE, VALUE);
static VALUE email(VALUE);
static VALUE email_set(VALUE, VALUE);
static VALUE url(VALUE);
static VALUE url_set(VALUE, VALUE);
static VALUE last_realname(VALUE);
static VALUE last_realname_set(VALUE, VALUE);
static VALUE last_host(VALUE);
static VALUE last_host_set(VALUE, VALUE);
static VALUE last_quit(VALUE);
static VALUE last_quit_set(VALUE, VALUE);
static VALUE status(VALUE);
static VALUE status_set(VALUE, VALUE);
static VALUE enforce(VALUE);
static VALUE enforce_set(VALUE, VALUE);
static VALUE secure(VALUE);
static VALUE secure_set(VALUE, VALUE);
static VALUE verified(VALUE);
static VALUE verified_set(VALUE, VALUE);
static VALUE cloak_on(VALUE);
static VALUE cloak_on_set(VALUE, VALUE);
static VALUE admin(VALUE);
static VALUE admin_set(VALUE, VALUE);
static VALUE emailverified(VALUE);
static VALUE emailverified_set(VALUE, VALUE);
static VALUE priv(VALUE);
static VALUE priv_set(VALUE, VALUE);
static VALUE language(VALUE);
static VALUE language_set(VALUE, VALUE);
static VALUE regtime(VALUE);
static VALUE regtime_set(VALUE, VALUE);
static VALUE last_seen(VALUE);
static VALUE last_seen_set(VALUE, VALUE);
static VALUE last_quit_time(VALUE);
static VALUE last_quit_time_set(VALUE, VALUE);
static VALUE nick_regtime(VALUE);
static VALUE nick_regtime_set(VALUE, VALUE);

static inline VALUE m_get_flag(VALUE, unsigned char(*)(Nickname *));
static inline VALUE m_get_string(VALUE, const char *(*)(Nickname *));

static inline VALUE m_set_flag(VALUE, VALUE, int(*)(Nickname *, unsigned char));
static inline VALUE m_set_string(VALUE, VALUE, int(*)(Nickname *, const char *));

static inline VALUE
m_get_flag(VALUE self, unsigned char(*get_func)(Nickname *))
{
  Nickname *nick = value_to_nickname(self);
  return get_func(nick) == TRUE ? Qtrue : Qfalse;
}

static inline VALUE
m_get_string(VALUE self, const char *(*get_func)(Nickname *))
{
  Nickname *nick = value_to_nickname(self);
  const char *ptr = get_func(nick);
  if(ptr == NULL)
    return Qnil;
  else
    return rb_str_new2(ptr);
}

static inline VALUE
m_set_flag(VALUE self, VALUE value, int(*set_func)(Nickname *, unsigned char))
{
  Nickname *nick = value_to_nickname(self);
  /* TODO XXX FIXME Check FALSE and throw exception */
  set_func(nick, value == Qtrue ? TRUE : FALSE);
  return value;
}

static inline VALUE
m_set_string(VALUE self, VALUE value, int(*set_func)(Nickname *, const char *))
{
  Nickname *nick = value_to_nickname(self);
  Check_Type(value, T_STRING);
  /* TODO XXX FIXME Check FALSE and throw exception */
  set_func(nick, StringValueCStr(value));
  return value;
}

static VALUE
initialize(VALUE self, VALUE nick)
{
  rb_iv_set(self, "@realptr", nick);
  return self;
}

static VALUE
account_id(VALUE self)
{
  Nickname *nick = value_to_nickname(self);
  return UINT2NUM(nickname_get_id(nick));
}

static VALUE
nickname_id(VALUE self)
{
  Nickname *nick = value_to_nickname(self);
  return UINT2NUM(nickname_get_nickid(nick));
}

static VALUE
primary_nickname_id(VALUE self)
{
  Nickname *nick = value_to_nickname(self);
  return UINT2NUM(nickname_get_pri_nickid(nick));
}

static VALUE
nick(VALUE self)
{
  return m_get_string(self, &nickname_get_nick);
}

static VALUE
nick_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &nickname_set_nick);
}

static VALUE
pass(VALUE self)
{
  return m_get_string(self, &nickname_get_pass);
}

static VALUE
pass_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &nickname_set_pass);
}

static VALUE
salt(VALUE self)
{
  return m_get_string(self, &nickname_get_salt);
}

static VALUE
salt_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &nickname_set_salt);
}

static VALUE
cloak(VALUE self)
{
  return m_get_string(self, &nickname_get_cloak);
}

static VALUE
cloak_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &nickname_set_cloak);
}

static VALUE
email(VALUE self)
{
  return m_get_string(self, &nickname_get_email);
}

static VALUE
email_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &nickname_set_email);
}

static VALUE
url(VALUE self)
{
  return m_get_string(self, &nickname_get_url);
}

static VALUE
url_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &nickname_set_url);
}

static VALUE
last_realname(VALUE self)
{
  return m_get_string(self, &nickname_get_last_realname);
}

static VALUE
last_realname_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &nickname_set_last_realname);
}

static VALUE
last_host(VALUE self)
{
  return m_get_string(self, &nickname_get_last_host);
}

static VALUE
last_host_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &nickname_set_last_host);
}

static VALUE
last_quit(VALUE self)
{
  return m_get_string(self, &nickname_get_last_quit);
}

static VALUE
last_quit_set(VALUE self, VALUE value)
{
  return m_set_string(self, value, &nickname_set_last_quit);
}

static VALUE
status(VALUE self)
{
  Nickname *nick = value_to_nickname(self);
  return UINT2NUM(nickname_get_status(nick));
}

static VALUE
status_set(VALUE self, VALUE value)
{
  Nickname *nick = value_to_nickname(self);
  nickname_set_status(nick, NUM2UINT(value));
  return value;
}

static VALUE
enforce(VALUE self)
{
  return m_get_flag(self, &nickname_get_enforce);
}

static VALUE
enforce_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &nickname_set_enforce);
}

static VALUE
secure(VALUE self)
{
  return m_get_flag(self, &nickname_get_secure);
}

static VALUE
secure_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &nickname_set_secure);
}

static VALUE
verified(VALUE self)
{
  return m_get_flag(self, &nickname_get_verified);
}

static VALUE
verified_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &nickname_set_verified);
}

static VALUE
cloak_on(VALUE self)
{
  return m_get_flag(self, &nickname_get_cloak_on);
}

static VALUE
cloak_on_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &nickname_set_cloak_on);
}

static VALUE
admin(VALUE self)
{
  return m_get_flag(self, &nickname_get_admin);
}

static VALUE
admin_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &nickname_set_admin);
}

static VALUE
emailverified(VALUE self)
{
  return m_get_flag(self, &nickname_get_email_verified);
}

static VALUE
emailverified_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &nickname_set_email_verified);
}

static VALUE
priv(VALUE self)
{
  return m_get_flag(self, &nickname_get_priv);
}

static VALUE
priv_set(VALUE self, VALUE value)
{
  return m_set_flag(self, value, &nickname_set_priv);
}

static VALUE
language(VALUE self)
{
  Nickname *nick = value_to_nickname(self);
  return UINT2NUM(nickname_get_language(nick));
}

static VALUE
language_set(VALUE self, VALUE value)
{
  Nickname *nick = value_to_nickname(self);
  nickname_set_language(nick, NUM2INT(value));
  return value;
}

static VALUE
regtime(VALUE self)
{
  Nickname *nick = value_to_nickname(self);
  return rb_time_new(nickname_get_reg_time(nick), (VALUE)NULL);
}

static VALUE
regtime_set(VALUE self, VALUE value)
{
  Nickname *nick = value_to_nickname(self);
  nickname_set_reg_time(nick, NUM2INT(value));
  return value;
}

static VALUE
last_seen(VALUE self)
{
  Nickname *nick = value_to_nickname(self);
  return rb_time_new(nickname_get_last_seen(nick), (VALUE)NULL);
}

static VALUE
last_seen_set(VALUE self, VALUE value)
{
  Nickname *nick = value_to_nickname(self);
  nickname_set_last_seen(nick, NUM2INT(value));
  return value;
}

static VALUE
last_quit_time(VALUE self)
{
  Nickname *nick = value_to_nickname(self);
  return rb_time_new(nickname_get_last_quit_time(nick), (VALUE)NULL);
}

static VALUE
last_quit_time_set(VALUE self, VALUE value)
{
  Nickname *nick = value_to_nickname(self);
  nickname_set_last_quit_time(nick, NUM2INT(value));
  return value;
}

static VALUE
nick_regtime(VALUE self)
{
  Nickname *nick = value_to_nickname(self);
  return rb_time_new(nickname_get_reg_time(nick), (VALUE)NULL);
}

static VALUE
nick_regtime_set(VALUE self, VALUE value)
{
  Nickname *nick = value_to_nickname(self);
  nickname_set_reg_time(nick, NUM2INT(value));
  return value;
}

void
Init_Nickname(void)
{
  cNickname = rb_define_class("Nickname", rb_cObject);

  rb_define_method(cNickname, "initialize", initialize, 1);
  rb_define_method(cNickname, "account_id", account_id, 0);
  rb_define_method(cNickname, "nickname_id", nickname_id, 0);
  rb_define_method(cNickname, "primary_nickname_id", primary_nickname_id, 0);
  rb_define_method(cNickname, "nick", nick, 0);
  rb_define_method(cNickname, "nick=", nick_set, 1);
  rb_define_method(cNickname, "pass", pass, 0);
  rb_define_method(cNickname, "pass=", pass_set, 1);
  rb_define_method(cNickname, "salt", salt, 0);
  rb_define_method(cNickname, "salt=", salt_set, 1);
  rb_define_method(cNickname, "cloak", cloak, 0);
  rb_define_method(cNickname, "cloak=", cloak_set, 1);
  rb_define_method(cNickname, "email", email, 0);
  rb_define_method(cNickname, "email=", email_set, 1);
  rb_define_method(cNickname, "url", url, 0);
  rb_define_method(cNickname, "url=", url_set, 1);
  rb_define_method(cNickname, "last_realname", last_realname, 0);
  rb_define_method(cNickname, "last_realname=", last_realname_set, 1);
  rb_define_method(cNickname, "last_host", last_host, 0);
  rb_define_method(cNickname, "last_host=", last_host_set, 1);
  rb_define_method(cNickname, "last_quit", last_quit, 0);
  rb_define_method(cNickname, "last_quit=", last_quit_set, 1);
  rb_define_method(cNickname, "status", status, 0);
  rb_define_method(cNickname, "status=", status_set, 1);
  rb_define_method(cNickname, "enforce?", enforce, 0);
  rb_define_method(cNickname, "enforce=", enforce_set, 1);
  rb_define_method(cNickname, "secure?", secure, 0);
  rb_define_method(cNickname, "secure=", secure_set, 1);
  rb_define_method(cNickname, "verified?", verified, 0);
  rb_define_method(cNickname, "verified=", verified_set, 1);
  rb_define_method(cNickname, "cloaked?", cloak_on, 0);
  rb_define_method(cNickname, "cloaked=", cloak_on_set, 1);
  rb_define_method(cNickname, "admin?", admin, 0);
  rb_define_method(cNickname, "admin=", admin_set, 1);
  rb_define_method(cNickname, "email_verified?", emailverified, 0);
  rb_define_method(cNickname, "email_verified=", emailverified_set, 1);
  rb_define_method(cNickname, "private?", priv, 0);
  rb_define_method(cNickname, "private=", priv_set, 1);
  rb_define_method(cNickname, "language", language, 0);
  rb_define_method(cNickname, "language=", language_set, 1);
  rb_define_method(cNickname, "reg_time", regtime, 0);
  rb_define_method(cNickname, "reg_time=", regtime_set, 1);
  rb_define_method(cNickname, "last_seen", last_seen, 0);
  rb_define_method(cNickname, "last_seen=", last_seen_set, 1);
  rb_define_method(cNickname, "last_quit_time", last_quit_time, 0);
  rb_define_method(cNickname, "last_quit_time=", last_quit_time_set, 1);
  rb_define_method(cNickname, "nick_reg_time", nick_regtime, 0);
  rb_define_method(cNickname, "nick_reg_time=", nick_regtime_set, 1);
}

Nickname* value_to_nickname(VALUE self)
{
  Nickname* out;
  VALUE nick = rb_iv_get(self, "@realptr");
  Data_Get_Struct(nick, Nickname, out);
  return out;
}

VALUE
nickname_to_value(Nickname *nick)
{
  VALUE rbnick, real_nick;

  rbnick = Data_Wrap_Struct(rb_cObject, 0, 0, nick);
  real_nick = do_ruby_ret(cNickname, rb_intern("new"), 1, rbnick);

  if(real_nick == Qnil)
  {
    printf("RUBY ERROR: Ruby Failed To Create Nickname\n");
    return Qnil;
  }

  return real_nick;
}
