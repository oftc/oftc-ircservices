# Copyright (C) 2006 Luca Filipozzi
MAINTAINERCLEANFILES=aclocal.m4 config.h.in config.h.in~ config.guess config.log config.status config.sub configure install-sh missing mkinstalldirs Makefile.in depcomp compile ltmain.sh COPYING
SUBDIRS=libltdl libio modules languages include src etc 

maintainer-clean-local:
	rm -f ${PACKAGE}-${VERSION}.tar.gz
	rm -rf libltdl

install-data-local:
	$(INSTALL) -d ${localstatedir}/log
