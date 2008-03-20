# Copyright (C) Luca Filipozzi
MAINTAINERCLEANFILES=Makefile.in
noinst_LIBRARIES=libruby_module.a
libruby_module_a_SOURCES=ruby_module.c client.c channel.c dbchannel.c nickname.c servicemodule.c libruby_module.h
libruby_module_a_CFLAGS=-I$(top_srcdir)/libio -I$(top_srcdir)/include -I$(top_srcdir)/languages @RUBY_CFLAGS@
