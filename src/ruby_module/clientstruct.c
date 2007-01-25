#include <ruby.h>
#include "libruby_module.h"

static VALUE ClientStruct_Initialize(VALUE, VALUE);
static VALUE ClientStruct_Name(VALUE);
static VALUE ClientStruct_Host(VALUE);
static VALUE ClientStruct_ID(VALUE);
static VALUE ClientStruct_Info(VALUE);
static VALUE ClientStruct_Username(VALUE);
static VALUE ClientStruct_Umodes(VALUE);
static VALUE ClientStruct_Nick(VALUE);

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
ClientStruct_Host(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return rb_str_new2(client->host);
}

static VALUE
ClientStruct_ID(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return rb_str_new2(client->id);
}

static VALUE
ClientStruct_Info(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return rb_str_new2(client->info);
}

static VALUE
ClientStruct_Username(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return rb_str_new2(client->username);
}

static VALUE
ClientStruct_Umodes(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  return INT2NUM(client->umodes);
}

static VALUE
ClientStruct_Nick(VALUE self)
{
  struct Client *client = rb_rbclient2cclient(self);
  VALUE nick = rb_cnick2rbnick(client->nickname);
  return nick;
}

void
Init_ClientStruct(void)
{
  cClientStruct = rb_define_class("ClientStruct", rb_cObject);

  rb_define_class_variable(cClientStruct, "@@realptr", Qnil);

  rb_define_method(cClientStruct, "initialize", ClientStruct_Initialize, 1);
  rb_define_method(cClientStruct, "name", ClientStruct_Name, 0);
  rb_define_method(cClientStruct, "host", ClientStruct_Host, 0);
  rb_define_method(cClientStruct, "id", ClientStruct_ID, 0);
  rb_define_method(cClientStruct, "info", ClientStruct_Info, 0);
  rb_define_method(cClientStruct, "username", ClientStruct_Username, 0);
  rb_define_method(cClientStruct, "umodes", ClientStruct_Umodes, 0);
  rb_define_method(cClientStruct, "nick", ClientStruct_Nick, 0);
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

