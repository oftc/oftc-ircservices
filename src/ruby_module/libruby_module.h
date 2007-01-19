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

static VALUE cServiceModule = Qnil;
static VALUE cClientStruct = Qnil;
static VALUE cChannelStruct = Qnil;
static VALUE cNickStruct = Qnil;

struct Client* rb_rbclient2cclient(VALUE);
VALUE rb_cclient2rbclient(struct Client*);
VALUE rb_carray2rbarray(int, char **);
struct RegChannel* rb_rbchannel2cchannel(VALUE);
VALUE rb_cchannel2rbchannel(struct RegChannel*);
struct Nick* rb_rbnick2cnick(VALUE);
VALUE rb_cnick2rbnick(struct Nick*);

void Init_ChannelStruct(void);
void Init_ClientStruct(void);
void Init_NickStruct(void);
void Init_ServiceModule(void);

#define RB_HOOKS_CMODE  0
#define RB_HOOKS_UMODE  1
#define RB_HOOKS_NEWUSR 2
/* Update this when new hooks are added */
#define RB_HOOKS_COUNT  3

static dlink_node *ruby_cmode_hook;
static dlink_node *ruby_umode_hook;
static dlink_node *ruby_newusr_hook;

static VALUE ruby_server_hooks = Qnil;

#define RB_CALLBACK(x) (VALUE (*)())(x)

VALUE rb_singleton_call(VALUE);

void rb_do_hook_cb(VALUE, VALUE);
void rb_add_hook(VALUE, VALUE, int);

int ruby_handle_error(int);
int do_ruby(VALUE(*)(), VALUE);

char *strupr(char *);

#endif /* INCLUDED_ruby_module_h */
