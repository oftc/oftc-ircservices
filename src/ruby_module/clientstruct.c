#include <ruby.h>
#include "libruby_module.h"

static VALUE cClientStruct = Qnil;

static VALUE ClientStruct_Initialize(VALUE, VALUE);
static VALUE ClientStruct_Name(VALUE);
static VALUE ClientStruct_NameSet(VALUE, VALUE);
static VALUE ClientStruct_Host(VALUE);
static VALUE ClientStruct_HostSet(VALUE, VALUE);
static VALUE ClientStruct_ID(VALUE);
static VALUE ClientStruct_IDSet(VALUE, VALUE);
static VALUE ClientStruct_Info(VALUE);
static VALUE ClientStruct_InfoSet(VALUE, VALUE);
static VALUE ClientStruct_Username(VALUE);
static VALUE ClientStruct_UsernameSet(VALUE, VALUE);
static VALUE ClientStruct_Nick(VALUE);
static VALUE ClientStruct_NickSet(VALUE, VALUE);
static VALUE ClientStruct_IsOper(VALUE);
static VALUE ClientStruct_IsAdmin(VALUE);
static VALUE ClientStruct_IsIdentified(VALUE);
static VALUE ClientStruct_IsServer(VALUE);
static VALUE ClientStruct_IsClient(VALUE);
static VALUE ClientStruct_IsMe(VALUE);

static VALUE
ClientStruct_Initialize(VALUE self, VALUE client)
{
  rb_iv_set(self, "@realptr", client);
  return self;
}

static VALUE
ClientStruct_Name(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return rb_str_new2(client->name);
}

static VALUE
ClientStruct_NameSet(VALUE self, VALUE value)
{
  struct Client *client = rb_rbclient2cclient(self);
  /*TODO check length < HOSTLEN */
  strlcpy(client->name, StringValueCStr(value), sizeof(client->name));
  return value;
}

static VALUE
ClientStruct_Host(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return rb_str_new2(client->host);
}

static VALUE
ClientStruct_HostSet(VALUE self, VALUE value)
{
  struct Client *client = rb_rbclient2cclient(self);
  /* TODO check length < HOSTLEN */
  strlcpy(client->host, StringValueCStr(value), sizeof(client->host));
  return value;
}

static VALUE
ClientStruct_ID(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return rb_str_new2(client->id);
}

static VALUE
ClientStruct_IDSet(VALUE self, VALUE value)
{
  struct Client *client = rb_rbclient2cclient(self);
  /* TODO check length < IDLEN */
  strlcpy(client->id, StringValueCStr(value), sizeof(client->id));
  return value;
}

static VALUE
ClientStruct_Info(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return rb_str_new2(client->info);
}

static VALUE
ClientStruct_InfoSet(VALUE self, VALUE value)
{
  struct Client *client = rb_rbclient2cclient(self);
  /* TODO check length < REALLEN */
  strlcpy(client->info, StringValueCStr(value), sizeof(client->info));
  return value;
}

static VALUE
ClientStruct_Username(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return rb_str_new2(client->username);
}

static VALUE
ClientStruct_UsernameSet(VALUE self, VALUE value)
{
  struct Client *client = rb_rbclient2cclient(self);
  /* TODO check length < USERLEN */
  strlcpy(client->username, StringValueCStr(value), sizeof(client->username));
  return value;
}

static VALUE
ClientStruct_Nick(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  VALUE nick = rb_cnick2rbnick(client->nickname);
  return nick;
}

static VALUE
ClientStruct_NickSet(VALUE self, VALUE value)
{
  struct Client *client = rb_rbclient2cclient(self);
  client->nickname = rb_rbnick2cnick(value);
  return value;
}

static VALUE
ClientStruct_IsOper(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return (IsOper(client));
}

static VALUE
ClientStruct_IsAdmin(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return (IsAdmin(client));
}

static VALUE
ClientStruct_IsIdentified(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return (IsIdentified(client));
}

static VALUE
ClientStruct_IsServer(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return (IsServer(client));
}

static VALUE
ClientStruct_IsClient(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return (IsClient(client));
}

static VALUE ClientStruct_IsMe(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return (IsMe(client));
}

void
Init_ClientStruct(void)
{
  cClientStruct = rb_define_class("ClientStruct", rb_cObject);

  rb_define_class_variable(cClientStruct, "@@realptr", Qnil);

  rb_define_method(cClientStruct, "initialize", ClientStruct_Initialize, 1);
  rb_define_method(cClientStruct, "name", ClientStruct_Name, 0);
  rb_define_method(cClientStruct, "name=", ClientStruct_NameSet, 1);
  rb_define_method(cClientStruct, "host", ClientStruct_Host, 0);
  rb_define_method(cClientStruct, "host=", ClientStruct_HostSet, 1);
  rb_define_method(cClientStruct, "id", ClientStruct_ID, 0);
  rb_define_method(cClientStruct, "id=", ClientStruct_IDSet, 1);
  rb_define_method(cClientStruct, "info", ClientStruct_Info, 0);
  rb_define_method(cClientStruct, "info=", ClientStruct_InfoSet, 1);
  rb_define_method(cClientStruct, "username", ClientStruct_Username, 0);
  rb_define_method(cClientStruct, "username=", ClientStruct_UsernameSet, 1);
  rb_define_method(cClientStruct, "nick", ClientStruct_Nick, 0);
  rb_define_method(cClientStruct, "nick=", ClientStruct_NickSet, 1);
  rb_define_method(cClientStruct, "is_oper?", ClientStruct_IsOper, 0);
  rb_define_method(cClientStruct, "is_admin?", ClientStruct_IsAdmin, 0);
  rb_define_method(cClientStruct, "is_identified?", ClientStruct_IsIdentified, 0);
  rb_define_method(cClientStruct, "is_server?", ClientStruct_IsServer, 0);
  rb_define_method(cClientStruct, "is_client?", ClientStruct_IsClient, 0);
  rb_define_method(cClientStruct, "is_me?", ClientStruct_IsMe, 0);
}

struct Client*
rb_rbclient2cclient(VALUE self)
{
  struct Client* out;
  VALUE rbclient = rb_iv_get(self, "@realptr");
  Data_Get_Struct(rbclient, struct Client, out);
  return out;
}

VALUE
rb_cclient2rbclient(struct Client *client)
{
  VALUE fc2params, rbclient, real_client;
  int status;

  rbclient = Data_Wrap_Struct(rb_cObject, 0, 0, client);

  fc2params = rb_ary_new();
  rb_ary_push(fc2params, cClientStruct);
  rb_ary_push(fc2params, rb_intern("new"));
  rb_ary_push(fc2params, 1);
  rb_ary_push(fc2params, (VALUE)&rbclient);

  real_client = rb_protect(RB_CALLBACK(rb_singleton_call), fc2params, &status);

  if(ruby_handle_error(status))
  {
    ilog(L_DEBUG, "RUBY ERROR: Ruby Failed To Create ClientStruct");
    return Qnil;
  }

  return real_client;
}

