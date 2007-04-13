#include <ruby.h>
#include "libruby_module.h"

VALUE cClientStruct = Qnil;
VALUE cNickStruct;

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
  const char* cvalue;

  Check_Type(value, T_STRING);

  if(strlen(cvalue) > HOSTLEN)
    rb_raise(rb_eArgError, "Failed setting Client.name %s too long", cvalue);

  strlcpy(client->name, cvalue, sizeof(client->name));
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
  const char* cvalue;

  Check_Type(value, T_STRING);

  if(strlen(cvalue) > HOSTLEN)
    rb_raise(rb_eArgError, "Failed setting Client.host %s too long", cvalue);

  strlcpy(client->host, cvalue, sizeof(client->host));
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
  const char* cvalue;

  Check_Type(value, T_STRING);

  if(strlen(cvalue) > IDLEN)
    rb_raise(rb_eArgError, "Failed setting Client.id %s too long", cvalue);

  strlcpy(client->id, cvalue, sizeof(client->id));
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
  const char* cvalue;

  Check_Type(value, T_STRING);

  if(strlen(cvalue) > REALLEN)
    rb_raise(rb_eArgError, "Failed setting Client.info %s too long", cvalue);

  strlcpy(client->info, cvalue, sizeof(client->info));
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
  const char* cvalue;

  Check_Type(value, T_STRING);

  if(strlen(cvalue) > USERLEN)
    rb_raise(rb_eArgError, "Failed setting Client.username %s too long", cvalue);

  strlcpy(client->username, cvalue, sizeof(client->username));
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

  Check_OurType(value, cNickStruct);

  client->nickname = rb_rbnick2cnick(value);
  return value;
}

static VALUE
ClientStruct_IsOper(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return IsOper(client) ? Qtrue : Qfalse;
}

static VALUE
ClientStruct_IsAdmin(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return IsAdmin(client) ? Qtrue : Qfalse;
}

static VALUE
ClientStruct_IsIdentified(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return IsIdentified(client) ? Qtrue : Qfalse;
}

static VALUE
ClientStruct_IsServer(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return IsServer(client) ? Qtrue : Qfalse;
}

static VALUE
ClientStruct_IsClient(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return IsClient(client) ? Qtrue : Qfalse;
}

static VALUE ClientStruct_IsMe(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return IsMe(client) ? Qtrue : Qfalse;
}

void
Init_ClientStruct(void)
{
  cClientStruct = rb_define_class("ClientStruct", rb_cObject);

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
  VALUE rbclient, real_client;

  rbclient = Data_Wrap_Struct(rb_cObject, 0, 0, client);
  real_client = do_ruby_ret(cClientStruct, rb_intern("new"), 1, rbclient);;

  if(real_client == Qnil)
  {
    ilog(L_DEBUG, "RUBY ERROR: Ruby Failed To Create ClientStruct");
    return Qnil;
  }

  return real_client;
}

