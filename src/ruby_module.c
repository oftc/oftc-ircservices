#include <ruby.h>
#include "stdinc.h"

VALUE mOftc;
VALUE cModule;

VALUE Module_register(int argc, VALUE *argv, VALUE class)
{
	struct Service *service;

	service = make_service(STR2CSTR(argv[0]));
	clear_serv_tree_parse(&service->msg_tree);
	dlinkAdd(service, &service->node, &services_list);
	hash_add_service(service);
	introduce_service(service);

	/*load_language(chanserv, "chanserv.en");
	load_language(chanserv, "chanserv.rude");
	load_language(chanserv, "chanserv.de");
	
	mod_add_servcmd(&chanserv->msg_tree, &register_msgtab);
	mod_add_servcmd(&chanserv->msg_tree, &help_msgtab);*/

	return Qnil;
}

void Init_Module(void)
{
	mOftc = rb_define_module("Oftc");
	cModule = rb_define_class_under(mOftc,"ModuleServer", rb_cObject);
	rb_define_singleton_method(cModule, "register", Module_register, -1);
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
	klass = rb_const_get(rb_cObject, rb_intern(classname));
	self = rb_class_new_instance(0, NULL, klass);
	rb_funcall(self, rb_intern("moo"), 0, NULL);
	
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

void
init_ruby(void)
{
	ruby_init();
	ruby_show_version();
	ruby_init_loadpath();
	Init_Module();
}
