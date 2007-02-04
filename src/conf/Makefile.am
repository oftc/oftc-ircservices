# Copyright (C) Luca Filipozzi
MAINTAINERCLEANFILES=Makefile.in
noinst_LIBRARIES=libconf.a
libconf_a_SOURCES=parser.y lexer.l conf.c connect.c database.c logging.c modules.c servicesinfo.c service.c
libconf_a_CFLAGS=-I$(top_srcdir)/libio -I$(top_srcdir)/include
AM_YFLAGS=-d
