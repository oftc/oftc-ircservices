# Copyright (C) 2006 Luca Filipozzi
MAINTAINERCLEANFILES=Makefile.in
dist_pkgdata_DATA=chanserv.en.lang chanserv.fr.lang nickserv.de.lang nickserv.en.lang operserv.en.lang services.en.lang
BUILT_SOURCES=chanserv-lang.h nickserv-lang.h operserv-lang.h services-lang.h
CLEANFILES=$(BUILT_SOURCES)

%-lang.h: %.en.lang
	$(TAIL) -n +2 $< | $(EGREP) -v "^[[:space:]]" | $(AWK) '{print "#define " $$1 " " FNR}' > $@
