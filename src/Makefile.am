# Copyright (C) 2006 Luca Filipozzi
MAINTAINERCLEANFILES=Makefile.in
SUBDIRS=conf ruby_module
sbin_PROGRAMS=services
services_SOURCES=channel.c channel_mode.c conf.c client.c crypt.c connection.c dbm.c hash.c hostmask.c interface.c language.c m_error.c packet.c parse.c  services.c send.c lua_module.c
#services_LDADD=$(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.$(OBJEXT))) $(top_srcdir)/libio/libio.a
services_LDADD=conf/libconf.a ruby_module/libruby_module.a $(top_srcdir)/libio/libio.a
services_LDFLAGS=@LIBLTDL@ @RUBY_LDFLAGS@ -export-dynamic
services_CFLAGS=-I$(top_srcdir)/libio -I$(top_srcdir)/include
INCLUDES = @INCLTDL@
if USE_SHARED_MODULES
services_LDADD+=
else
services_LDADD+=$(top_srcdir)/modules/libmodules.la
endif
AM_YFLAGS=-d
