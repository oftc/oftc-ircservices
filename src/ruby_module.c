#include <ruby.h>
#include <ctype.h>
#include <signal.h>
#include "stdinc.h"

VALUE mOftc;
VALUE cModule;

char *strupr(char *s)
{
	char *c;
	for (c=s; c && *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
	return c;
}

static void
m_generic(struct Service *service, struct Client *client,
		    int parc, char *parv[])
{
	char *command = strdup(service->last_command);
	VALUE *rbservice, *rbclient, *rbparv;
	VALUE cServiceStruct, cClientStruct, class;
	int i;

	strupr(command);

	class = rb_path2class(service->name);
		
	printf("Ruby Generic Handler Call: %s -> %s from %s\n", service->name, command, parv[0]);

	rbparv = rb_ary_new();

	for (i = 1; i <= parc; ++i)
		rb_ary_push(rbparv, rb_str_new2(parv[i]));

	cServiceStruct = rb_path2class("ServiceStruct");
	cClientStruct = rb_path2class("ClientStruct");

	rbservice = Data_Wrap_Struct(cServiceStruct, 0, free, client);
	rbclient = Data_Wrap_Struct(cClientStruct, 0, free, client);
	
	printf("Passing Control to Ruby Handler\n");
	rb_funcall2(class, rb_intern(command), 3, (VALUE *)(&rbservice, &rbclient, &rbparv));
}

VALUE Module_register(int argc, VALUE *argv, VALUE class)
{
	struct Service *service;
	struct ServiceMessage *generic_msgtab;
	VALUE aryCommands, command;
	long i;
	int n;

	service = make_service(StringValueCStr(argv[0]));
	clear_serv_tree_parse(&service->msg_tree);
	dlinkAdd(service, &service->node, &services_list);
	hash_add_service(service);
	introduce_service(service);

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
		
		printf("Adding generic ruby handler for: %s\n", generic_msgtab->cmd);
		
		mod_add_servcmd(&service->msg_tree, generic_msgtab);
	}
		
	return Qnil;
}

VALUE Module_reply_user(int argc, VALUE *argv, VALUE class)
{
	struct Service *service;
	struct Client *client;
	
	Data_Get_Struct(argv[0], struct Service, service);
	Data_Get_Struct(argv[1], struct Client, client);
	char *message = StringValueCStr(argv[2]);
	
	reply_user(service, client, message);
}

void Init_Module(void)
{
	mOftc = rb_define_module("Oftc");
	cModule = rb_define_class_under(mOftc,"ModuleServer", rb_cObject);
	rb_define_singleton_method(cModule, "register", Module_register, -1);
	rb_define_singleton_method(cModule, "repy_user", Module_reply_user, -1);
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
	
	printf("Loading ruby class: %s\n", classname);
	klass = rb_protect((VALUE (*)())rb_path2class, (VALUE)(classname), &status);
	
	if(status != 0)
	{
		rb_p(ruby_errinfo);
		return 0;
	}
	printf("Ruby class loaded\n");
	
	self = rb_funcall2(klass, rb_intern("new"), 0, NULL);
	
	return 1;
}

void sigabrt() { printf("SIGABRT\n"); rb_p(ruby_errinfo); }

void
init_ruby(void)
{
	ruby_init();
	ruby_show_version();
	ruby_init_loadpath();
	Init_Module();
	signal(SIGINT, SIG_DFL);
	signal(SIGHUP, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGABRT, sigabrt);
}
