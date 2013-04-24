# Copyright (C) 2006 Luca Filipozzi
MAINTAINERCLEANFILES=Makefile.in
SUBDIRS=conf 
if USE_RUBY
SUBDIRS+=ruby_module
endif
if USE_PYTHON
SUBDIRS+=python_module
endif
bin_PROGRAMS=services
services_SOURCES= akick.c					    \
									akill.c				      \
									chanaccess.c		    \
									channel.c			      \
									channel_mode.c	    \
									conf.c				      \
									client.c			      \
									crypt.c				      \
									connection.c		    \
									dbchannel.c			    \
									dbm.c				        \
									dbmail.c			      \
									event.c							\
                  group.c             \
                  groupaccess.c       \
									hash.c				      \
									hostmask.c			    \
									interface.c			    \
									jupe.c				      \
									language.c			    \
                  kill.c              \
									m_error.c			      \
									mqueue.c			      \
									nickname.c			    \
									packet.c			      \
									parse.c				      \
									servicemask.c		    \
									services.c			    \
									send.c              \
									tor.c

services_LDADD=conf/libconf.a $(top_srcdir)/libio/libio.a @LIBLTDL@
services_LDFLAGS=-levent
if USE_RUBY
services_LDFLAGS+=@RUBY_LDFLAGS@
services_LDADD+=ruby_module/libruby_module.a
endif
if USE_PYTHON
services_LDFLAGS+=@PYTHON_LDFLAGS@
services_LDADD+=python_module/libpython_module.a
endif
services_LDFLAGS+=-export-dynamic
services_CFLAGS=-I$(top_srcdir)/libio -I$(top_srcdir)/include -I$(top_srcdir)/languages
AM_CPPFLAGS=@INCLTDL@
if USE_SHARED_MODULES
services_LDADD+=
else
services_LDADD+=$(top_srcdir)/modules/libmodules.la
endif
AM_YFLAGS=-d
top_build_prefix=$(top_srcdir)/
