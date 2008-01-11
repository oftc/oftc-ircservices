#include <ruby.h>
#include "libruby_module.h"

VALUE cClient = Qnil;
VALUE cNickStruct;

static VALUE initialize(VALUE, VALUE);
static VALUE name(VALUE);
static VALUE name_set(VALUE, VALUE);
static VALUE host(VALUE);
static VALUE host_set(VALUE, VALUE);
static VALUE realhost(VALUE);
static VALUE realhost_set(VALUE, VALUE);
static VALUE id(VALUE);
static VALUE id_set(VALUE, VALUE);
static VALUE info(VALUE);
static VALUE info_set(VALUE, VALUE);
static VALUE username(VALUE);
static VALUE username_set(VALUE, VALUE);
static VALUE nick(VALUE);
static VALUE nick_set(VALUE, VALUE);
static VALUE ctcp(VALUE);
static VALUE ctcp_set(VALUE, VALUE);
static VALUE ts(VALUE);
static VALUE from(VALUE);
static VALUE is_oper(VALUE);
static VALUE is_admin(VALUE);
static VALUE is_identified(VALUE);
static VALUE is_server(VALUE);
static VALUE is_client(VALUE);
static VALUE is_me(VALUE);
static VALUE is_services_client(VALUE);

static VALUE
initialize(VALUE self, VALUE client)
{
  rb_iv_set(self, "@realptr", client);
  return self;
}

static VALUE
name(VALUE self)
{
  struct Client *client = value_to_client(self);
  return rb_str_new2(client->name);
}

static VALUE
name_set(VALUE self, VALUE value)
{
  struct Client *client = value_to_client(self);
  const char* cvalue;

  Check_Type(value, T_STRING);

  strlcpy(client->name, cvalue, sizeof(client->name));
  return value;
}

static VALUE
host(VALUE self)
{
  struct Client *client = value_to_client(self);
  return rb_str_new2(client->host);
}

static VALUE
host_set(VALUE self, VALUE value)
{
  struct Client *client = value_to_client(self);
  const char* cvalue;

  Check_Type(value, T_STRING);

  strlcpy(client->host, cvalue, sizeof(client->host));
  return value;
}

static VALUE
realhost(VALUE self)
{
  struct Client *client = value_to_client(self);
  return rb_str_new2(client->realhost);
}

static VALUE
realhost_set(VALUE self, VALUE value)
{
  struct Client *client = value_to_client(self);
  const char* cvalue;

  Check_Type(value, T_STRING);

  strlcpy(client->realhost, cvalue, sizeof(client->host));
  return value;
}

static VALUE
id(VALUE self)
{
  struct Client *client = value_to_client(self);
  return rb_str_new2(client->id);
}

static VALUE
id_set(VALUE self, VALUE value)
{
  struct Client *client = value_to_client(self);
  const char* cvalue;

  Check_Type(value, T_STRING);

  strlcpy(client->id, cvalue, sizeof(client->id));
  return value;
}

static VALUE
info(VALUE self)
{
  struct Client *client = value_to_client(self);
  return rb_str_new2(client->info);
}

static VALUE
info_set(VALUE self, VALUE value)
{
  struct Client *client = value_to_client(self);
  const char* cvalue;

  Check_Type(value, T_STRING);

  strlcpy(client->info, cvalue, sizeof(client->info));
  return value;
}

static VALUE
username(VALUE self)
{
  struct Client *client = value_to_client(self);
  return rb_str_new2(client->username);
}

static VALUE
username_set(VALUE self, VALUE value)
{
  struct Client *client = value_to_client(self);
  const char* cvalue;

  Check_Type(value, T_STRING);

  strlcpy(client->username, cvalue, sizeof(client->username));
  return value;
}

static VALUE
nick(VALUE self)
{
  struct Client *client = value_to_client(self);
  VALUE nick = rb_cnick2rbnick(client->nickname);
  return nick;
}

static VALUE
nick_set(VALUE self, VALUE value)
{
  struct Client *client = value_to_client(self);

  Check_OurType(value, cNickStruct);

  client->nickname = rb_rbnick2cnick(value);
  return value;
}

static VALUE
ctcp(VALUE self)
{
  struct Client *client = value_to_client(self);

  return rb_str_new2(client->ctcp_version);
}

static VALUE
ctcp_set(VALUE self, VALUE value)
{
  struct Client *client = value_to_client(self);

  strncpy(client->ctcp_version, StringValueCStr(value), IRC_BUFSIZE);

  return self;
}

static VALUE
ts(VALUE self)
{
  struct Client *client = value_to_client(self);
  return ULONG2NUM(client->tsinfo);
}

static VALUE
from(VALUE self)
{
  struct Client *client = value_to_client(self);
  return client_to_value(client->from);
}

static VALUE
is_oper(VALUE self)
{
  struct Client *client = value_to_client(self);
  return IsOper(client) ? Qtrue : Qfalse;
}

static VALUE
is_admin(VALUE self)
{
  struct Client *client = value_to_client(self);
  return IsAdmin(client) ? Qtrue : Qfalse;
}

static VALUE
is_identified(VALUE self)
{
  struct Client *client = value_to_client(self);
  return IsIdentified(client) ? Qtrue : Qfalse;
}

static VALUE
is_server(VALUE self)
{
  struct Client *client = value_to_client(self);
  return IsServer(client) ? Qtrue : Qfalse;
}

static VALUE
is_client(VALUE self)
{
  struct Client *client = value_to_client(self);
  return IsClient(client) ? Qtrue : Qfalse;
}

static VALUE is_me(VALUE self)
{
  struct Client *client = value_to_client(self);
  return IsMe(client) ? Qtrue : Qfalse;
}

static VALUE is_services_client(VALUE self)
{
  struct Client *client = value_to_client(self);
  return IsMe(client->from) ? Qtrue : Qfalse;
}

void
Init_Client(void)
{
  cClient = rb_define_class("Client", rb_cObject);

  rb_define_method(cClient, "initialize", initialize, 1);
  rb_define_method(cClient, "name", name, 0);
  rb_define_method(cClient, "name=", name_set, 1);
  rb_define_method(cClient, "host", host, 0);
  rb_define_method(cClient, "host=", host_set, 1);
  rb_define_method(cClient, "realhost", host, 0);
  rb_define_method(cClient, "realhost=", host_set, 1);
  rb_define_method(cClient, "id", id, 0);
  rb_define_method(cClient, "id=", id_set, 1);
  rb_define_method(cClient, "info", info, 0);
  rb_define_method(cClient, "info=", info_set, 1);
  rb_define_method(cClient, "username", username, 0);
  rb_define_method(cClient, "username=", username_set, 1);
  rb_define_method(cClient, "nick", nick, 0);
  rb_define_method(cClient, "nick=", nick_set, 1);
  rb_define_method(cClient, "ctcp_version", ctcp, 0);
  rb_define_method(cClient, "ctcp_version=", ctcp_set, 1);
  rb_define_method(cClient, "ts", ts, 0);
  rb_define_method(cClient, "from", from, 0);
  rb_define_method(cClient, "is_oper?", is_oper, 0);
  rb_define_method(cClient, "is_admin?", is_admin, 0);
  rb_define_method(cClient, "is_identified?", is_identified, 0);
  rb_define_method(cClient, "is_server?", is_server, 0);
  rb_define_method(cClient, "is_client?", is_client, 0);
  rb_define_method(cClient, "is_me?", is_me, 0);
  rb_define_method(cClient, "is_services_client?", is_services_client, 0);
}

struct Client*
value_to_client(VALUE self)
{
  struct Client* out;
  VALUE rbclient = rb_iv_get(self, "@realptr");
  Data_Get_Struct(rbclient, struct Client, out);
  return out;
}

VALUE
client_to_value(struct Client *client)
{
  VALUE rbclient, real_client;

  rbclient = Data_Wrap_Struct(rb_cObject, 0, 0, client);
  real_client = do_ruby_ret(cClient, rb_intern("new"), 1, rbclient);;

  if(real_client == Qnil)
  {
    ilog(L_DEBUG, "RUBY ERROR: Ruby Failed To Create ClientStruct");
    return Qnil;
  }

  return real_client;
}

