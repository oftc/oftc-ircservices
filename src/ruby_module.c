/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  ruby_module.c: An interface to run ruby scripts
 *
 *  Copyright (C) 2006 TJ Fontaine and the OFTC Coding department
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id: $
 */

#include <ruby.h>
#include <ctype.h>
#include <signal.h>
#include "stdinc.h"

#define RB_CALLBACK(x) (VALUE (*)())(x)

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

void
ruby_script_error() {
	VALUE lasterr, array;
	char *err;
	int i;

	if(!NIL_P(ruby_errinfo))
	{
		lasterr = rb_gv_get("$!");
		err = RSTRING(rb_obj_as_string(lasterr))->ptr;
		
		printf("RUBY ERROR: Error while executing Ruby Script: %s\n", err);
		array = rb_funcall(ruby_errinfo, rb_intern("backtrace"), 0);
		printf("RUBY ERROR: BACKTRACE\n");
		for (i = 0; i < RARRAY(array)->len; ++i)
			printf("RUBY ERROR:   %s\n", RSTRING(RARRAY(array)->ptr[i])->ptr);
	}
}

int
ruby_handle_error(int status)
{
	if(status)
	{
		ruby_script_error();
		ruby_cleanup(status);
	}
	return status;
}

int
do_ruby(VALUE (*func)(), VALUE args)
{
	int status;

	rb_protect(func, args, &status);

	if(ruby_handle_error(status))
		return 0;

	return 1;
}

VALUE
rb_singleton_call(VALUE data)
{
	VALUE *args = (VALUE *)data;
	return rb_funcall2(args[0], args[1], args[2], args[3]);
}

static void
m_generic(struct Service *service, struct Client *client,
		    int parc, char *parv[])
{
	char *command = strdup(service->last_command);
	VALUE rbparams, rbclient, rbparv;
	VALUE class, real_client;
	VALUE fc2params[4];
	ID class_command;
	int i, status;

	strupr(command);
	class_command = rb_intern(command);

	class = rb_path2class(service->name);
		
	rbparams = rb_ary_new();
	rbparv = rb_ary_new();

	for (i = 1; i <= parc; ++i)
		rb_ary_push(rbparv, rb_str_new2(parv[i]));

	rbclient = Data_Wrap_Struct(rb_cObject, 0, free, client);

	fc2params[0] = cClientStruct;
	fc2params[1] = rb_intern("new");
	fc2params[2] = 1;
	fc2params[3] = &rbclient;

	real_client = rb_protect(RB_CALLBACK(rb_singleton_call), fc2params, &status);

	if(ruby_handle_error(status))
	{
		reply_user(service, client, "An error has occurred, please be patient and report this bug");
		global_notice(service, "Ruby Failed To Create ClientStruct");
		printf("RUBY ERROR: Ruby Failed To Create ClientStruct\n");
	}

	rb_ary_push(rbparams, real_client);
	rb_ary_push(rbparams, rbparv);

	fc2params[0] = class;
	fc2params[1] = class_command;
	fc2params[2] = RARRAY(rbparams)->len;
	fc2params[3] = RARRAY(rbparams)->ptr;
	
	if(!do_ruby(RB_CALLBACK(rb_singleton_call), (VALUE)fc2params))
	{
		reply_user(service, client, "An error has occurred, please be patient and report this bug");
		global_notice(service, "Ruby Failed to Execute Command: %s by %s", command, client->name);
		printf("RUBY ERROR: Ruby Failed to Execute Command: %s by %s\n", command, client->name);
	}
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
	VALUE params[4];
	
	snprintf(path, sizeof(path), "%s/%s", dir, fname);
	
	printf("RUBY INFO: Loading ruby module: %s\n", path);
	
	if(!do_ruby(RB_CALLBACK(rb_load_file), (VALUE)path))
		return 0;
	
	ruby_exec();
	
	strncpy(classname, fname, strlen(fname)-3);
	
	klass = rb_protect(RB_CALLBACK(rb_path2class), (VALUE)(classname), &status);
	
	if(ruby_handle_error(status))
		return 0;

	printf("RUBY INFO: Loaded Class %s\n", classname);
	
	params[0] = klass;
	params[1] = rb_intern("new");
	params[2] = 0;
	params[3] = NULL;
	
	self = rb_protect(RB_CALLBACK(rb_singleton_call), (VALUE)params, &status);

	if(ruby_handle_error(status))
		return 0;

	printf("RUBY INFO: Initialized Class %s\n", classname);
	
	return 1;
}

void 
sigabrt() 
{
	signal(SIGABRT, sigabrt);
	printf("We've encountered a SIGABRT\n");
	ruby_script_error();
}

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
