#include <ruby.h>
#include <ctype.h>
#include <signal.h>
#include "stdinc.h"

VALUE mOftc;
VALUE cModule;
VALUE cClientStruct;

char *strupr(char *s)
{
	char *c;
	for (c=s; c && *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
	return c;
}

struct Client* rbclient2client(VALUE client)
{
	struct Client* out;
	Data_Get_Struct(client, struct Client, out);
	return out;
}


static void
m_generic(struct Service *service, struct Client *client,
		    int parc, char *parv[])
{
	char *command = strdup(service->last_command);
	VALUE rbparams, rbclient, rbparv;
	VALUE class, real_client;
	int i;

	strupr(command);

	class = rb_path2class(service->name);
		
	rbparams = rb_ary_new();
	rbparv = rb_ary_new();

	for (i = 1; i <= parc; ++i)
		rb_ary_push(rbparv, rb_str_new2(parv[i]));

	rbclient = Data_Wrap_Struct(rb_cObject, 0, free, client);

	real_client = rb_funcall2(cClientStruct, rb_intern("new"), 1, &rbclient);

	rb_ary_push(rbparams, real_client);
	rb_ary_push(rbparams, rbparv);
	
	rb_funcall2(class, rb_intern(command), RARRAY(rbparams)->len, RARRAY(rbparams)->ptr);
}

VALUE Module_register(int argc, VALUE *argv, VALUE class)
{
	struct Service *ruby_service;
	struct ServiceMessage *generic_msgtab;
	VALUE aryCommands, command;
	long i;
	int n;

	ruby_service = make_service(StringValueCStr(argv[0]));
	clear_serv_tree_parse(&ruby_service->msg_tree);
	dlinkAdd(ruby_service, &ruby_service->node, &services_list);
	hash_add_service(ruby_service);
	introduce_service(ruby_service);

	/*load_language(chanserv, "chanserv.en");
	load_language(chanserv, "chanserv.rude");
	load_language(chanserv, "chanserv.de");*/
	
	Check_Type(argv[1], T_ARRAY);
	
	aryCommands = argv[1];
	
	for(i = RARRAY(aryCommands)->len-1; i >= 0; --i)
	{
		generic_msgtab = MyMalloc(sizeof(struct ServiceMessage));
		command = rb_ary_shift(aryCommands);
		generic_msgtab->cmd = StringValueCStr(command);
		rb_ary_push(aryCommands, command);

		for(n = 0; n < SERVICES_LAST_HANDLER_TYPE; n++)
			generic_msgtab->handlers[n] = m_generic;
		
		mod_add_servcmd(&ruby_service->msg_tree, generic_msgtab);
	}
		
	return Qnil;
}

VALUE Module_reply_user(int argc, VALUE *argv, VALUE class)
{
	struct Client *client;
	struct Service *service;
	VALUE rbclient;
	
	service = find_service(StringValueCStr(argv[0]));
	
	rbclient = rb_iv_get(argv[1], "@realptr");
	
	client = rbclient2client(rbclient);
	char *message = StringValueCStr(argv[2]);

	reply_user(service, client, message);
	return Qnil;
}

void Init_Module(void)
{
	mOftc = rb_define_module("Oftc");
	cModule = rb_define_class_under(mOftc,"ModuleServer", rb_cObject);
	rb_define_singleton_method(cModule, "register", Module_register, -1);
	rb_define_singleton_method(cModule, "reply_user", Module_reply_user, -1);
}

VALUE
ClientStruct_Initialize(VALUE self, VALUE client)
{
	rb_iv_set(self, "@realptr", client);
	return self;
}

VALUE
ClientStruct_Name(VALUE self)
{
	VALUE clientstruct = rb_iv_get(self, "@realptr");
	return rb_str_new2(rbclient2client(clientstruct)->name);
}

VALUE
ClientStruct_Host(VALUE self)
{
	VALUE clientstruct = rb_iv_get(self, "@realptr");
	return rb_str_new2(rbclient2client(clientstruct)->host);
}

VALUE
ClientStruct_ID(VALUE self)
{
	VALUE clientstruct = rb_iv_get(self, "@realptr");
	return rb_str_new2(rbclient2client(clientstruct)->id);
}

VALUE
ClientStruct_Info(VALUE self)
{
	VALUE clientstruct = rb_iv_get(self, "@realptr");
	return rb_str_new2(rbclient2client(clientstruct)->info);
}

VALUE
ClientStruct_Username(VALUE self)
{
	VALUE clientstruct = rb_iv_get(self, "@realptr");
	return rb_str_new2(rbclient2client(clientstruct)->username);
}

VALUE
ClientStruct_Umodes(VALUE self)
{
	VALUE clientstruct = rb_iv_get(self, "@realptr");
	return INT2NUM(rbclient2client(clientstruct)->umodes);
}

void
Init_ClientStruct(void)
{
	cClientStruct = rb_define_class_under(mOftc, "ClientStruct", rb_cObject);
	
	rb_define_class_variable(cClientStruct, "@@realptr", Qnil);

	rb_define_method(cClientStruct, "initialize", ClientStruct_Initialize, 1);
	rb_define_method(cClientStruct, "name", ClientStruct_Name, 0);
	rb_define_method(cClientStruct, "host", ClientStruct_Host, 0);
	rb_define_method(cClientStruct, "id", ClientStruct_ID, 0);
	rb_define_method(cClientStruct, "info", ClientStruct_Info, 0);
	rb_define_method(cClientStruct, "username", ClientStruct_Username, 0);
	rb_define_method(cClientStruct, "umodes", ClientStruct_Umodes, 0);
}

int
load_ruby_module(const char *name, const char *dir, const char *fname)
{
	int status;
	char path[PATH_MAX];
	char classname[PATH_MAX];
	VALUE klass, self;
	
	snprintf(path, sizeof(path), "%s/%s", dir, fname);
	
	printf("Loading ruby module: %s\n", path);
	rb_protect((VALUE (*)())rb_load_file, (VALUE)path, &status);
	
	if(status != 0)
	{
		rb_p(ruby_errinfo);
		return 0;
	}
	
	status = ruby_exec();
	
	strncpy(classname, fname, strlen(fname)-3);
	
	klass = rb_protect((VALUE (*)())rb_path2class, (VALUE)(classname), &status);
	
	if(status != 0)
	{
		rb_p(ruby_errinfo);
		return 0;
	}
	
	self = rb_funcall2(klass, rb_intern("new"), 0, NULL);
	
	return 1;
}

void sigabrt() { printf("We've encountered a SIGABRT\n"); rb_p(ruby_errinfo); }

void
init_ruby(void)
{
	ruby_init();
	ruby_show_version();
	ruby_init_loadpath();
	Init_Module();
	Init_ClientStruct();
	signal(SIGINT, SIG_DFL);
	signal(SIGHUP, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGABRT, sigabrt);
}
