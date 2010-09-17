# Copyright (C) 2006 Luca Filipozzi
MAINTAINERCLEANFILES=Makefile.in
INCLUDES=-I$(top_srcdir)/libio -I$(top_srcdir)/include -I$(top_srcdir)/languages
if USE_SHARED_MODULES
AM_LDFLAGS=-no-undefined -module -avoid-version
pkglib_LTLIBRARIES=chanserv.la operserv.la nickserv.la groupserv.la floodserv.la irc.la oftc.la pgsql.la nulldb.la
dist_pkglib_DATA=PythonServ.py RubyServ.rb JupeServ.rb XmlRpc.rb ServiceBase.rb GanneffServ.rb Bopm.rb MoranServ.rb
else
AM_LDFLAGS=-static
noinst_LTLIBRARIES=chanserv.la operserv.la nickserv.la groupserv.la floodserv.la irc.la oftc.la pgsql.la nulldb.la
dist_pkglib_DATA=PythonServ.py RubyServ.rb JupeServ.rb XmlRpc.rb ServiceBase.rb GanneffServ.rb Bopm.rb MoranServ.rb
endif
chanserv_la_SOURCES=chanserv.c
operserv_la_SOURCES=operserv.c
nickserv_la_SOURCES=nickserv.c
floodserv_la_SOURCES=floodserv.c
groupserv_la_SOURCES=groupserv.c
irc_la_SOURCES=irc.c
oftc_la_SOURCES=oftc.c
pgsql_la_SOURCES=pgsql.c
pgsql_la_CFLAGS=@POSTGRESQL_CFLAGS@
pgsql_la_LIBADD=@POSTGRESQL_LDFLAGS@
nulldb_la_SOURCES=nulldb.c

install-exec-local:
	rm -f $(DESTDIR)/$(pkglibdir)/*.la $(DESTDIR)/$(pkblibdir)/*.a
