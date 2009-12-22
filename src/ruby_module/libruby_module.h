#ifndef INCLUDED_libruby_module_h
#define INCLUDED_libruby_module_h

/* Umm, it sucks but ruby defines these and so do we */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef EXTERN

#include "stdinc.h"
#include "ruby_module.h"
#include "nickname.h"
#include "chanserv.h"
#include "nickserv.h"
#include "channel_mode.h"
#include "channel.h"
#include "dbm.h"
#include "language.h"
#include "hash.h"
#include "parse.h"
#include "client.h"
#include "interface.h"
#include "msg.h"

VALUE rb_carray2rbarray(int, char **);

struct Service* get_service(VALUE);

struct Client* value_to_client(VALUE);
VALUE client_to_value(struct Client*);

struct Channel* value_to_channel(VALUE);
VALUE channel_to_value(struct Channel*);

DBChannel* value_to_dbchannel(VALUE);
VALUE dbchannel_to_value(DBChannel*);

Nickname* value_to_nickname(VALUE);
VALUE nickname_to_value(Nickname*);

result_set_t* value_to_dbresult(VALUE);
VALUE dbresult_to_value(result_set_t*);

row_t* value_to_dbrow(VALUE);
VALUE dbrow_to_value(row_t*);

void Init_Channel(void);
void Init_DBChannel(void);
void Init_Client(void);
void Init_Nickname(void);
void Init_ServiceModule(void);

void Init_DB(void);
void Init_DBResult(void);
void Init_DBRow(void);

enum Ruby_Hooks
{
  RB_HOOKS_CMODE,
  RB_HOOKS_UMODE,
  RB_HOOKS_NEWUSR,
  RB_HOOKS_PRIVMSG,
  RB_HOOKS_JOIN,
  RB_HOOKS_PART,
  RB_HOOKS_QUIT,
  RB_HOOKS_NICK,
  RB_HOOKS_NOTICE,
  RB_HOOKS_CHAN_CREATED,
  RB_HOOKS_CHAN_DELETED,
  RB_HOOKS_CTCP,
  RB_HOOKS_NICK_REG,
  RB_HOOKS_CHAN_REG,
  RB_HOOKS_DB_INIT,
  RB_HOOKS_EOB,
  RB_HOOKS_COUNT
};

#define RB_CALLBACK(x) (VALUE (*)())(x)

VALUE rb_singleton_call(VALUE);

void rb_do_hook_cb(VALUE, VALUE);
void rb_add_hook(VALUE, VALUE, int);
VALUE rb_add_event(VALUE, VALUE, VALUE);
VALUE rb_delete_event(VALUE, VALUE);

int ruby_handle_error(int);
VALUE do_ruby(VALUE, ID, int, ...);
VALUE do_ruby_ret(VALUE, ID, int, ...);

struct ruby_args
{
  VALUE recv;
  ID id;
  int parc;
  VALUE *parv;
};

struct rhook_args
{
  int parc;
  VALUE *parv;
};

void check_our_type(VALUE obj, VALUE type);

extern VALUE cServiceModule;
extern VALUE cNickname;
extern VALUE cClient;
extern VALUE cDBChannel;
extern VALUE cChannel;

extern VALUE cDB;
extern VALUE cDBResult;
extern VALUE cDBRow;

#define Check_OurType(x, v) check_our_type((VALUE)(x), (VALUE)(v))

#endif /* INCLUDED_ruby_module_h */
