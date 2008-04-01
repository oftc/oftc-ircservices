# Copyright (C) Luca Filipozzi
MAINTAINERCLEANFILES=Makefile.in
noinst_LIBRARIES=libruby_module.a
libruby_module_a_SOURCES= channel.c \
	client.c \
	db.c \
	dbchannel.c \
	dbresult.c \
	dbrow.c \
	libruby_module.h \
	nickname.c \
	servicemodule.c \
	ruby_module.c
libruby_module_a_CFLAGS=-I$(top_srcdir)/libio -I$(top_srcdir)/include -I$(top_srcdir)/languages @RUBY_CFLAGS@
