# Copyright (C) 2006 Luca Filipozzi
MAINTAINERCLEANFILES=Makefile.in
SUBDIRS=conf
sbin_PROGRAMS=services
services_SOURCES=conf.cc client.cc crypt.cc connection.cc dbm.cc hostmask.cc interface.cc irc_protocol.cc language.cc module.cc packet.cc parse.cc services.cc 
#services_LDADD=$(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.$(OBJEXT))) $(top_srcdir)/libio/libio.a
services_LDADD=conf/libconf.a $(top_srcdir)/libio/libio.a
services_LDFLAGS=@RUBY_LDFLAGS@ @LIBLTDL@ -Wl,-export-dynamic
INCLUDES = @INCLTDL@
services_CXXFLAGS=-I$(top_srcdir)/libio -I$(top_srcdir)/include @RUBY_CFLAGS@ 
if USE_SHARED_MODULES
services_LDADD+=
else
services_LDADD+=$(top_srcdir)/modules/libmodules.la
endif
AM_YFLAGS=-d
