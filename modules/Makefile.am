# Copyright (C) 2006 Luca Filipozzi
MAINTAINERCLEANFILES=Makefile.in
INCLUDES=-I$(top_srcdir)/libio -I$(top_srcdir)/include -I$(top_srcdir)/languages
if USE_SHARED_MODULES
AM_LDFLAGS=-no-undefined -module -avoid-version
pkglib_LTLIBRARIES=chanserv.la operserv.la nickserv.la irc.la hybrid.la hybridts6.la oftc.la
dist_pkglib_DATA=luaserv.lua RubyServ.rb nickserv.lua JupeServ.rb XmlRpc.rb PerlServ.pl
else
AM_LDFLAGS=-static
noinst_LTLIBRARIES=chanserv.la operserv.la nickserv.la
endif
chanserv_la_SOURCES=chanserv.c
operserv_la_SOURCES=operserv.c
nickserv_la_SOURCES=nickserv.c
irc_la_SOURCES=irc.c
hybrid_la_SOURCES=hybrid.c
hybridts6_la_SOURCES=hybridts6.c
oftc_la_SOURCES=oftc.c

install-exec-local:
	rm -f $(DESTDIR)/$(pkglibdir)/*.la $(DESTDIR)/$(pkblibdir)/*.a
