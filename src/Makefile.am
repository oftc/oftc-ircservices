# Copyright (C) 2006 Luca Filipozzi
MAINTAINERCLEANFILES=Makefile.in
SUBDIRS=conf 
if USE_RUBY
SUBDIRS+=ruby_module
endif
bin_PROGRAMS=services
services_SOURCES=channel.c channel_mode.c conf.c client.c crypt.c connection.c dbm.c hash.c hostmask.c interface.c language.c m_error.c packet.c parse.c  services.c send.c mqueue.c 
services_LDADD=conf/libconf.a $(top_srcdir)/libio/libio.a
services_LDFLAGS=@LIBLTDL@ @RUBY_LDFLAGS@ -export-dynamic
if USE_RUBY
services_LDADD+=ruby_module/libruby_module.a
endif
services_LDFLAGS+=-export-dynamic
services_CFLAGS=-I$(top_srcdir)/libio -I$(top_srcdir)/include -I$(top_srcdir)/languages
INCLUDES=@INCLTDL@
if USE_SHARED_MODULES
services_LDADD+=
else
services_LDADD+=$(top_srcdir)/modules/libmodules.la
endif
AM_YFLAGS=-d
