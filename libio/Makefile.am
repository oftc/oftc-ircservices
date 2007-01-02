# Copyright (C) 2006 Luca Filipozzi
MAINTAINERCLEANFILES=Makefile.in
SUBDIRS=comm mem misc net string
noinst_LIBRARIES=libio.a
libio_a_SOURCES=irc_libio.h libioinc.h
libio_a_LIBADD=$(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.$(OBJEXT)))
libio_a_CFLAGS=-DIN_LIBIO
