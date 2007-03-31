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

VALUE rb_carray2rbarray(int, char **);
char** rb_rbarray2carray(VALUE);

struct Client* rb_rbclient2cclient(VALUE);
VALUE rb_cclient2rbclient(struct Client*);

struct Channel* rb_rbchannel2cchannel(VALUE);
VALUE rb_cchannel2rbchannel(struct Channel*);

struct RegChannel* rb_rbregchan2cregchan(VALUE);
VALUE rb_cregchan2rbregchan(struct RegChannel*);

struct Nick* rb_rbnick2cnick(VALUE);
VALUE rb_cnick2rbnick(struct Nick*);

void Init_ChannelStruct(void);
void Init_RegChannel(void);
void Init_ClientStruct(void);
void Init_NickStruct(void);
void Init_ServiceModule(void);

enum Ruby_Hooks
{
  RB_HOOKS_CMODE,
  RB_HOOKS_UMODE,
  RB_HOOKS_NEWUSR,
  RB_HOOKS_PRIVMSG,
  RB_HOOKS_JOIN,
  RB_HOOKS_NICK,
};
/* Update this when new hooks are added */
#define RB_HOOKS_COUNT 6

#define RB_CALLBACK(x) (VALUE (*)())(x)

VALUE rb_singleton_call(VALUE);

void rb_do_hook_cb(VALUE, VALUE);
void rb_add_hook(VALUE, VALUE, int);

int ruby_handle_error(int);
int do_ruby(VALUE(*)(), VALUE);

char *strupr(char *);

#endif /* INCLUDED_ruby_module_h */
