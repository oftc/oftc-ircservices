# Copyright (C) Luca Filipozzi
MAINTAINERCLEANFILES=Makefile.in
noinst_LIBRARIES=libpython_module.a
libpython_module_a_SOURCES= python_module.c \
	servicemodule.c \
	client.c \
	libpython_module.h
	
libpython_module_a_CFLAGS=-I$(top_srcdir)/libio -I$(top_srcdir)/include -I$(top_srcdir)/languages @PYTHON_CFLAGS@
