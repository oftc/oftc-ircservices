# Copyright (C) 2006 Luca Filipozzi
# vim: set fdm=marker ts=2 sw=2 et:

AC_PREREQ(2.59)
AC_INIT([oftc-services],[1.2.0-pre13])
AM_INIT_AUTOMAKE([oftc-ircservices],[1.2.0-pre13])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

#ltdl stuff
AC_LIBTOOL_DLOPEN
AC_LIBLTDL_CONVENIENCE
AC_SUBST(INCLTDL)
AC_SUBST(LIBLTDL)

# Checks for programs.
AC_PROG_INSTALL
AC_GNU_SOURCE
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_AWK
AC_PROG_EGREP
AC_PATH_PROG([TAIL],[tail])

# Checks for libraries.
AX_CHECK_LIB_RUBY
AX_CHECK_LIB_OPENSSL
AX_CHECK_LIB_YADA
AX_CHECK_LIB_IPV4
AX_CHECK_LIB_IPV6

# Checks for header files.
AC_CHECK_HEADERS([sys/resource.h]) # ick

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_C_BIGENDIAN

# Checks for library functions.
AC_CHECK_FUNC([snprintf],[AC_DEFINE([HAVE_SNPRINTF],[1],[Define to 1 if you have the snprintf() function.])])
AC_CHECK_FUNC([vsnprintf],[AC_DEFINE([HAVE_VSNPRINTF],[1],[Define to 1 if you have the vsnprintf() function.])])
AC_CHECK_FUNC([lrand48],[AC_DEFINE([HAVE_LRAND48],[1],[Define to 1 if you have the lrand48() function.])])
AC_CHECK_FUNC([srand48],[AC_DEFINE([HAVE_LRAND48],[1],[Define to 1 if you have the srand48() function.])])
AC_CHECK_FUNC([mmap],[AC_DEFINE([HAVE_MMAP],[1],[Define to 1 if you have the mmap() function.])])
AC_CHECK_FUNC([strtok_r],[AC_DEFINE([HAVE_STRTOK_R],[1],[Define to 1 if you have the strtok_r() function.])])
AC_CHECK_FUNC([usleep],[AC_DEFINE([HAVE_USLEEP],[1],[Define to 1 if you have the usleep() function.])])
AC_CHECK_FUNC([strlcat],[AC_DEFINE([HAVE_STRLCAT],[1],[Define to 1 if you have the strlcat() function.])])
AC_CHECK_FUNC([strlcpy],[AC_DEFINE([HAVE_STRLCPY],[1],[Define to 1 if you have the strlcpy() function.])])

# Argument processing.
AX_ARG_ENABLE_IOLOOP_MECHANISM
AX_ARG_DISABLE_BLOCK_ALLOC
AX_ARG_DISABLE_SHARED_MODULES
AX_ARG_WITH_NICKLEN
AX_ARG_WITH_USERLEN
AX_ARG_WITH_HOSTLEN
AX_ARG_WITH_CLIENT_HEAP_SIZE
AX_ARG_WITH_CHANNEL_HEAP_SIZE
AX_ARG_WITH_DBUF_HEAP_SIZE
AX_ARG_WITH_DNODE_HEAP_SIZE
AX_ARG_WITH_BAN_HEAP_SIZE
AX_ARG_WITH_TOPIC_HEAP_SIZE
AX_ARG_WITH_SERVICES_HEAP_SIZE
AX_ARG_WITH_MQUEUE_HEAP_SIZE
AX_ARG_WITH_FMSG_HEAP_SIZE
AX_ARG_WITH_SYSLOG
AX_ARG_ENABLE_HALFOPS
AX_ARG_ENABLE_DEBUGGING
AX_ARG_ENABLE_WARNINGS
AX_ARG_ENABLE_EFENCE
AX_ARG_ENABLE_SYSLOG

AC_DEFINE_DIR([PREFIX],[prefix],[Set to prefix.])
AC_DEFINE_DIR([SYSCONFDIR],[sysconfdir],[Set to sysconfdir.])
AC_DEFINE_DIR([LIBDIR],[libdir],[Set to libdir.])
AC_DEFINE_DIR([DATADIR],[datadir],[Set to datadir.])
AC_DEFINE_DIR([LOCALSTATEDIR],[localstatedir],[Set to localstatedir.])
AC_DEFINE([SHARED_SUFFIX],[".so"],[Suffix for shared libraries on this platform.]) # wtf is this needed? replace libdl with libltdl to correct
AC_CONFIG_FILES([Makefile libio/Makefile libio/comm/Makefile libio/mem/Makefile libio/misc/Makefile libio/net/Makefile libio/string/Makefile etc/Makefile modules/Makefile src/Makefile src/conf/Makefile src/ruby_module/Makefile languages/Makefile include/Makefile include/conf/Makefile sql/Makefile])
AC_CONFIG_SUBDIRS(libltdl)
AC_OUTPUT
