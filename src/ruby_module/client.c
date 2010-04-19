#include <ruby.h>
#include "libruby_module.h"

VALUE cClient = Qnil;
VALUE cNickname;

static VALUE initialize(VALUE, VALUE);
static VALUE name(VALUE);
static VALUE name_set(VALUE, VALUE);
static VALUE host(VALUE);
static VALUE host_set(VALUE, VALUE);
static VALUE realhost(VALUE);
static VALUE realhost_set(VALUE, VALUE);
static VALUE sockhost(VALUE);
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
static VALUE firsttime(VALUE);
static VALUE from(VALUE);
static VALUE is_oper(VALUE);
static VALUE is_admin(VALUE);
static VALUE is_identified(VALUE);
static VALUE is_server(VALUE);
static VALUE is_client(VALUE);
static VALUE is_me(VALUE);
static VALUE is_services_client(VALUE);
static VALUE join(VALUE, VALUE);
static VALUE part(VALUE, VALUE, VALUE);
static VALUE m_exit(VALUE, VALUE, VALUE);
static VALUE cloak(VALUE, VALUE);
static VALUE find(VALUE, VALUE);
static VALUE to_str(VALUE);
static VALUE ip_or_hostname(VALUE);

void
Init_Client(void)
{
  cClient = rb_define_class("Client", rb_cObject);

  rb_define_method(cClient, "initialize", initialize, 1);
  rb_define_method(cClient, "name", name, 0);
  rb_define_method(cClient, "name=", name_set, 1);
  rb_define_method(cClient, "host", host, 0);
  rb_define_method(cClient, "host=", host_set, 1);
  rb_define_method(cClient, "realhost", realhost, 0);
  rb_define_method(cClient, "realhost=", realhost_set, 1);
  rb_define_method(cClient, "sockhost", sockhost, 0);
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
  rb_define_method(cClient, "firsttime", firsttime, 0);
  rb_define_method(cClient, "from", from, 0);
  rb_define_method(cClient, "is_oper?", is_oper, 0);
  rb_define_method(cClient, "is_admin?", is_admin, 0);
  rb_define_method(cClient, "is_identified?", is_identified, 0);
  rb_define_method(cClient, "is_server?", is_server, 0);
  rb_define_method(cClient, "is_client?", is_client, 0);
  rb_define_method(cClient, "is_me?", is_me, 0);
  rb_define_method(cClient, "is_services_client?", is_services_client, 0);
  rb_define_method(cClient, "join", join, 1);
  rb_define_method(cClient, "part", part, 2);
  rb_define_method(cClient, "exit", m_exit, 2);
  rb_define_method(cClient, "cloak", cloak, 1);
  rb_define_method(cClient, "to_str", to_str, 0);
  rb_define_method(cClient, "ip_or_hostname", ip_or_hostname, 0);

  rb_define_singleton_method(cClient, "find", find, 1);
}

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
  if(EmptyString(client->realhost))
    return Qnil;
  else
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
sockhost(VALUE self)
{
  struct Client *client = value_to_client(self);
  if(EmptyString(client->sockhost))
    return Qnil;
  else
    return rb_str_new2(client->sockhost);
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
  VALUE nick = nickname_to_value(client->nickname);
  return nick;
}

static VALUE
nick_set(VALUE self, VALUE value)
{
  struct Client *client = value_to_client(self);

  Check_OurType(value, cNickname);

  client->nickname = value_to_nickname(value);
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
firsttime(VALUE self)
{
  struct Client *client = value_to_client(self);
  return ULONG2NUM(client->firsttime);
}

static VALUE
from(VALUE self)
{
  struct Client *client = value_to_client(self);
  return client_to_value(client->servptr);
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

static VALUE
join(VALUE self, VALUE channame)
{
  struct Client *client = value_to_client(self);
  const char* chname;
  struct Channel *channel;

  Check_Type(channame, T_STRING);
  chname = StringValueCStr(channame);

  channel = hash_find_channel(chname);

  if(channel == NULL)
    channel = make_channel(chname);

  join_channel(client, channel);

  return channel_to_value(channel);
}

static VALUE
part(VALUE self, VALUE channame, VALUE reason)
{
  struct Client *client = value_to_client(self);
  const char* chname;
  char creason[KICKLEN+1];

  Check_Type(channame, T_STRING);

  chname = StringValueCStr(channame);

  creason[0] = '\0';

  if(!NIL_P(reason))
  {
    Check_Type(reason, T_STRING);
    strlcpy(creason, StringValueCStr(reason), sizeof(creason));
  }

  part_channel(client, chname, creason);

  return self;
}

static VALUE
m_exit(VALUE self, VALUE rbsource, VALUE rbreason)
{
  struct Client *client = value_to_client(self);
  struct Client *source = value_to_client(rbsource);
  char *reason;

  Check_Type(rbreason, T_STRING);

  reason = StringValueCStr(rbreason);

  exit_client(client, source, reason);
  return Qtrue;
}

static VALUE
cloak(VALUE self, VALUE hostname)
{
  struct Client *client = value_to_client(self);
  char *host;

  Check_Type(hostname, T_STRING);
  host = StringValueCStr(hostname);
  cloak_user(client, host);

  return Qtrue;
}

static VALUE
find(VALUE klass, VALUE name)
{
  struct Client *client;
  const char *cname = StringValueCStr(name);

  if(IsDigit(*cname))
    client = hash_find_id(cname);
  else
    client = find_client(cname);

  if(client == NULL || !IsClient(client))
    return Qnil;
  else
    return client_to_value(client);
}

static VALUE
to_str(VALUE self)
{
  struct Client *client = value_to_client(self);
  char buf[IRC_BUFSIZE+1] = {0};

  snprintf(buf, IRC_BUFSIZE, "%s!%s@%s", client->name, client->username,
    EmptyString(client->realhost) ? client->host : client->realhost);

  return rb_str_new2(buf);
}

/* return in the order they're available, prefer ip to names, if ip not available
 * check to see if they're cloaked and return the realhost, otherwise just return
 * the host. The caller is responsible to do forward lookups if it's not ip */
static VALUE
ip_or_hostname(VALUE self)
{
  struct Client *client = value_to_client(self);

  if(!EmptyString(client->sockhost))
    return rb_str_new2(client->sockhost);
  else if(!EmptyString(client->realhost))
    return rb_str_new2(client->realhost);
  else
    return rb_str_new2(client->host);
}

struct Client *
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

