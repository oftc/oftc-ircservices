dnl Copyright (C) 2006 Luca Filipozzi
dnl vim: set fdm=marker sw=2 ts=2 et si:
dnl {{{ ax_check_lib_ipv4
AC_DEFUN([AX_CHECK_LIB_IPV4],[
  AC_SEARCH_LIBS([socket],[socket],,[AC_MSG_ERROR([socket library not found])])
  AC_CHECK_FUNCS([inet_aton inet_ntop inet_pton])
  AC_CHECK_TYPES([struct sockaddr_in, struct sockaddr_storage, struct addrinfo],,,[#include <netdb.h>])
  AC_CHECK_MEMBERS([struct sockaddr_in.sin_len],,,[#include <netdb.h>])
])dnl }}}
dnl {{{ ax_check_lib_ipv6
AC_DEFUN([AX_CHECK_LIB_IPV6],[
  AC_CHECK_TYPES([struct sockaddr_in6],,[AC_DEFINE([IPV6],[1],[Define to 1 if you have IPv6 support.])],,[#include <netdb.h>])
])dnl }}}
dnl {{{ ax_check_lib_ruby
AC_DEFUN([AX_CHECK_LIB_RUBY],[
  AC_ARG_ENABLE([ruby],[AC_HELP_STRING([--disable-ruby],[Disable Ruby scripting engine])],[use_ruby="$enableval"],[use_ruby="yes"])
  if test "$use_ruby" = "no"; then
    have_ruby="no"
  else
    AC_PATH_PROG([RUBY],[ruby],[no])
    if test "$RUBY" = "no" ; then
      have_ruby="no"
    else
      AC_SEARCH_LIBS([ruby_init],[ruby1.8],[have_ruby="yes"],[have_ruby="no"])
      if test "$have_ruby" = "yes" ; then
        ruby_cflags=$($RUBY -r mkmf -e 'print "-I" + Config::CONFIG[["archdir"]]')
        ruby_ldflags=$($RUBY -r mkmf -e 'print "-L" + Config::CONFIG[["libdir"]] + " " + Config::CONFIG[["LIBS"]]')
        AC_SUBST([RUBY_CFLAGS],["$ruby_cflags"])
        AC_SUBST([RUBY_LDFLAGS],["$ruby_ldflags"])
				AC_DEFINE_UNQUOTED([HAVE_RUBY], [$have_ruby], [Is ruby enabled])
      else
        AC_MSG_WARN([Ruby 1.8 not found, disabling])
      fi
    fi
  fi
  AM_CONDITIONAL([USE_RUBY], [test "$have_ruby" = "yes"])
])dnl }}}
dnl {{{ ax_check_lib_openssl
AC_DEFUN([AX_CHECK_LIB_OPENSSL],[
  AC_CHECK_HEADER([openssl/sha.h],,[AC_MSG_ERROR([openssl header files not found])])
  AC_CHECK_LIB([ssl],[SHA_Init],,[AC_MSG_ERROR([openssl library not found])])
  AC_CHECK_LIB([crypto],[EVP_MD_CTX_init],,[AC_MSG_ERROR([openssl library not found])])
])dnl }}}
dnl {{{ ax_check_lib_yada
AC_DEFUN([AX_CHECK_LIB_YADA],[
  AC_ARG_WITH([yada],
    AC_HELP_STRING([--with-yada=@<:@ARG@:>@],
      [use yada library, optionally specify path]
      ),
    )
  AC_CHECK_HEADER([yada.h],,[AC_MSG_ERROR([yada header files not found])])
  AC_CHECK_LIB([yada],[yada_init],,[AC_MSG_ERROR([yada library not found])])
])dnl }}}
dnl {{{ ax_arg_enable_ioloop_mechanism (FIXME)
AC_DEFUN([AX_ARG_ENABLE_IOLOOP_MECHANISM],[
  dnl {{{ allow the user to specify desired mechanism
  desired_iopoll_mechanism="none"
  dnl FIXME need to handle arguments a bit better (see ac_arg_disable_block_alloc)
  AC_ARG_ENABLE([kqueue], [AC_HELP_STRING([--enable-kqueue], [Force kqueue usage.])], [desired_iopoll_mechanism="kqueue"])
  AC_ARG_ENABLE([epoll],  [AC_HELP_STRING([--enable-epoll],  [Force epoll usage.])],  [desired_iopoll_mechanism="epoll"])
  AC_ARG_ENABLE([devpoll],[AC_HELP_STRING([--enable-devpoll],[Force devpoll usage.])],[desired_iopoll_mechanism="devpoll"])
  AC_ARG_ENABLE([rtsigio],[AC_HELP_STRING([--enable-rtsigio],[Force rtsigio usage.])],[desired_iopoll_mechanism="rtsigio"])
  AC_ARG_ENABLE([poll],   [AC_HELP_STRING([--enable-poll],   [Force poll usage.])],   [desired_iopoll_mechanism="poll"]) 
  AC_ARG_ENABLE([select], [AC_HELP_STRING([--enable-select], [Force select usage.])], [desired_iopoll_mechanism="select"])
  dnl }}}
  dnl {{{ preamble
  AC_MSG_CHECKING([for optimal/desired iopoll mechanism])
  iopoll_mechanism_none=0
  AC_DEFINE_UNQUOTED([__IOPOLL_MECHANISM_NONE],[$iopoll_mechanism_none],[no iopoll mechanism])
  dnl }}}
  dnl {{{ check for kqueue mechanism support
  iopoll_mechanism_kqueue=1
  AC_DEFINE_UNQUOTED([__IOPOLL_MECHANISM_KQUEUE],[$iopoll_mechanism_kqueue],[kqueue mechanism])
  AC_LINK_IFELSE([AC_LANG_FUNC_LINK_TRY([kevent])],[is_kqueue_mechanism_available="yes"],[is_kqueue_mechanism_available="no"])
  dnl }}}
  dnl {{{ check for epoll oechanism support
  iopoll_mechanism_epoll=2
  AC_DEFINE_UNQUOTED([__IOPOLL_MECHANISM_EPOLL],[$iopoll_mechanism_epoll],[epoll mechanism])
  AC_RUN_IFELSE([
#include <sys/epoll.h>
#include <sys/syscall.h>
#if defined(__stub_epoll_create) || defined(__stub___epoll_create) || defined(EPOLL_NEED_BODY)
#if !defined(__NR_epoll_create)
#if defined(__ia64__)
#define __NR_epoll_create 1243
#elif defined(__x86_64__)
#define __NR_epoll_create 214
#elif defined(__sparc64__) || defined(__sparc__)
#define __NR_epoll_create 193
#elif defined(__s390__) || defined(__m68k__)
#define __NR_epoll_create 249
#elif defined(__ppc64__) || defined(__ppc__)
#define __NR_epoll_create 236
#elif defined(__parisc__) || defined(__arm26__) || defined(__arm__)
#define __NR_epoll_create 224
#elif defined(__alpha__)
#define __NR_epoll_create 407
#elif defined(__sh64__)
#define __NR_epoll_create 282
#elif defined(__i386__) || defined(__sh__) || defined(__m32r__) || defined(__h8300__) || defined(__frv__)
#define __NR_epoll_create 254
#else
#error No system call numbers defined for epoll family.
#endif
#endif
_syscall1(int, epoll_create, int, size)
#endif
main() { return epoll_create(256) == -1 ? 1 : 0; }
  ],[is_epoll_mechanism_available="yes"],[is_epoll_mechanism_available="no"])
  dnl }}}
  dnl {{{ check for devpoll mechanism support
  iopoll_mechanism_devpoll=3
  AC_DEFINE_UNQUOTED([__IOPOLL_MECHANISM_DEVPOLL],[$iopoll_mechanism_devpoll],[devpoll mechanism])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <devpoll.h>])],[is_devpoll_mechanism_available="yes"],[is_devpoll_mechanism_available="no"])
  if test "$is_devpoll_mechanism_available" = "yes" ; then
    AC_DEFINE([HAVE_DEVPOLL_H],[1],[Define to 1 if you have the <devpoll.h> header file.])
  fi
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <sys/devpoll.h>])],[is_devpoll_mechanism_available="yes"],[is_devpoll_mechanism_available="no"])
  if test "$is_devpoll_mechanism_available" = "yes" ; then
    AC_DEFINE([HAVE_SYS_DEVPOLL_H],[1],[Define to 1 if you have the <sys/devpoll.h> header file.])
  fi
  dnl }}}
  dnl {{{ check for rtsigio mechanism support
  iopoll_mechanism_rtsigio=4
  AC_DEFINE_UNQUOTED([__IOPOLL_MECHANISM_RTSIGIO],[$iopoll_mechanism_rtsigio],[rtsigio mechanism])
  AC_RUN_IFELSE([AC_LANG_PROGRAM([
#include <fcntl.h>
#ifdef F_SETSIG
main () { return 0; } /* F_SETSIG defined */
#else
main () { return 1; } /* F_SETSIG not defined */
#endif
  ])],[is_rtsigio_mechanism_available="yes"],[is_rtsigio_mechanism_available="no"])
  dnl }}}
  dnl {{{ check for poll mechanism support
  iopoll_mechanism_poll=5
  AC_DEFINE_UNQUOTED([__IOPOLL_MECHANISM_POLL],[$iopoll_mechanism_poll],[poll mechanism])
  AC_LINK_IFELSE([AC_LANG_FUNC_LINK_TRY([poll])],[is_poll_mechanism_available="yes"],[is_poll_mechanism_available="no"])
  dnl }}}
  dnl {{{ check for select mechanism support
  iopoll_mechanism_select=6
  AC_DEFINE_UNQUOTED([__IOPOLL_MECHANISM_SELECT],[$iopoll_mechanism_select],[select mechanism])
  AC_LINK_IFELSE([AC_LANG_FUNC_LINK_TRY([select])],[is_select_mechanism_available="yes"],[is_select_mechanism_available="no"])
  dnl }}}
  dnl {{{ determine the optimal mechanism
  optimal_iopoll_mechanism="none"
  for mechanism in "kqueue" "epoll" "devpoll" "rtsigio" "poll" "select" ; do # order is important
    eval "is_optimal_iopoll_mechanism_available=\$is_${mechanism}_mechanism_available"
    if test "$is_optimal_iopoll_mechanism_available" = "yes" ; then
      optimal_iopoll_mechanism="$mechanism"
      break
    fi
  done
  dnl }}}
  dnl {{{ select between optimal mechanism and desired mechanism (if specified)
  if test "$desired_iopoll_mechanism" = "none" ; then
    if test "$optimal_iopoll_mechanism" = "none" ; then
      AC_MSG_RESULT([none])
      AC_MSG_ERROR([no iopoll mechanism found!])
    else
      selected_iopoll_mechanism=$optimal_iopoll_mechanism
      AC_MSG_RESULT([$selected_iopoll_mechanism])
    fi
  else
    eval "is_desired_iopoll_mechanism_available=\$is_${desired_iopoll_mechanism}_mechanism_available"
    if test "$is_desired_iopoll_mechanism_available" = "yes" ; then
      selected_iopoll_mechanism=$desired_iopoll_mechanism
      AC_MSG_RESULT([$selected_iopoll_mechanism])
    else
      AC_MSG_RESULT([none])
      AC_MSG_ERROR([desired iopoll mechanism, $desired_iopoll_mechanism, is not available])
    fi
  fi
  dnl }}}
  dnl {{{ postamble
  eval "use_iopoll_mechanism=\$iopoll_mechanism_${selected_iopoll_mechanism}"
  AC_DEFINE_UNQUOTED([USE_IOPOLL_MECHANISM],[$use_iopoll_mechanism],[use this iopoll mechanism])
  dnl }}}
])dnl }}}
dnl {{{ ax_arg_disable_block_alloc
AC_DEFUN([AX_ARG_DISABLE_BLOCK_ALLOC],[
  AC_ARG_ENABLE([block-alloc],[AC_HELP_STRING([--disable-block-alloc],[Disable block alloc.])],[block_alloc="$enableval"],[block_alloc="yes"])
  if test "$block_alloc" = "no" ; then
    use_block_alloc=0
  else
    use_block_alloc=1
  fi
  AC_DEFINE_UNQUOTED([USE_BLOCK_ALLOC],[$use_block_alloc],[use block alloc])
])dnl }}}
dnl {{{ ax_arg_disable_shared_modules (FIXME)
AC_DEFUN([AX_ARG_DISABLE_SHARED_MODULES],[
  AC_ARG_ENABLE([shared-modules],[AC_HELP_STRING([--disable-shared-modules],[Disable shared modules.])],[shared_modules="$enableval"],[shared_modules="yes"])
  AC_CHECK_HEADERS([dlfcn.h link.h])
  AC_CHECK_FUNCS([dlopen dlinfo])
  if test "$shared_modules" = "yes" ; then
    use_shared_modules="yes"
    AC_CHECK_LIB([dl],[dlopen],,[AC_MSG_ERROR([dl library not found])])
    AC_DEFINE([USE_SHARED_MODULES],[1],[Define to 1 if you want to use shared modules.])
  else
    use_shared_modules="no"
    AC_MSG_WARN([shared module support has been disabled per supplied configure option])
  fi
  AM_CONDITIONAL([USE_SHARED_MODULES],[test "$shared_modules" = "yes"])
])dnl }}}
dnl {{{ ax_arg_with_nicklen
AC_DEFUN([AX_ARG_WITH_NICKLEN],[
  AC_ARG_WITH([nicklen],[AC_HELP_STRING([--with-nicklen=<value>],[Set nickname length (default 9).])],[nicklen="$withval"],[nicklen="9"])
  AC_DEFINE_UNQUOTED([NICKLEN],[($nicklen+1)],[Length of nicknames.]) 
])dnl }}}
dnl {{{ ax_arg_with_userlen
AC_DEFUN([AX_ARG_WITH_USERLEN],[
  AC_ARG_WITH([userlen],[AC_HELP_STRING([--with-userlen=<value>],[Set username length (default 9).])],[userlen="$withval"],[userlen="9"])
  AC_DEFINE_UNQUOTED([USERLEN],[($userlen+1)],[Length of nicknames.]) 
])dnl }}}
dnl {{{ ax_arg_with_hostlen
AC_DEFUN([AX_ARG_WITH_HOSTLEN],[
  AC_ARG_WITH([hostlen],[AC_HELP_STRING([--with-hostlen=<value>],[Set username length (default 62).])],[hostlen="$withval"],[hostlen="62"])
  AC_DEFINE_UNQUOTED([HOSTLEN],[($hostlen+1)],[Length of nicknames.]) 
])dnl }}}
dnl {{{ ax_arg_with_topiclen
AC_DEFUN([AX_ARG_WITH_TOPICLEN],[
  AC_ARG_WITH([topiclen],[AC_HELP_STRING([--with-topiclen=<value>],[Set topic length (default 300).])],[topiclen="$withval"],[topiclen="300"])
  AC_DEFINE_UNQUOTED([TOPICLEN],[($topiclen+1)],[Length of topics.]) 
])dnl }}}
dnl {{{ ax_arg_with_client_heap_size
AC_DEFUN([AX_ARG_WITH_CLIENT_HEAP_SIZE],[
  AC_ARG_WITH([client-heap-size],[AC_HELP_STRING([--client-heap-size=<value>],[Set client heap size (default 256).])],[client_heap_size="$withval"],[client_heap_size="256"])
  AC_DEFINE_UNQUOTED([CLIENT_HEAP_SIZE],[$client_heap_size],[Size of the client heap.])
])dnl }}}
dnl {{{ ax_arg_with_channel_heap_size
AC_DEFUN([AX_ARG_WITH_CHANNEL_HEAP_SIZE],[
  AC_ARG_WITH([channel-heap-size],[AC_HELP_STRING([--channel-heap-size=<value>],[Set channel heap size (default 256).])],[channel_heap_size="$withval"],[channel_heap_size="256"])
  AC_DEFINE_UNQUOTED([CHANNEL_HEAP_SIZE],[$channel_heap_size],[Size of the channel heap.])
])dnl }}}
dnl {{{ ax_arg_with_dbuf_heap_size
AC_DEFUN([AX_ARG_WITH_DBUF_HEAP_SIZE],[
  AC_ARG_WITH([dbuf-heap-size],[AC_HELP_STRING([--dbuf-heap-size=<value>],[Set dbuf heap size (default 64).])],[dbuf_heap_size="$withval"],[dbuf_heap_size="64"])
  AC_DEFINE_UNQUOTED([DBUF_HEAP_SIZE],[$dbuf_heap_size],[Size of the dbuf heap.])
])dnl }}}
dnl {{{ ax_arg_with_dnode_heap_size
AC_DEFUN([AX_ARG_WITH_DNODE_HEAP_SIZE],[
  AC_ARG_WITH([dnode-heap-size],[AC_HELP_STRING([--dnode-heap-size=<value>],[Set dnode heap size (default 256).])],[dnode_heap_size="$withval"],[dnode_heap_size="256"])
  AC_DEFINE_UNQUOTED([DNODE_HEAP_SIZE],[$dnode_heap_size],[Size of the dlink_node heap.])
])dnl }}}
dnl {{{ ax_arg_with_ban_heap_size
AC_DEFUN([AX_ARG_WITH_BAN_HEAP_SIZE],[
  AC_ARG_WITH([ban-heap-size],[AC_HELP_STRING([--ban-heap-size=<value>],[Set ban heap size (default 128).])],[ban_heap_size="$withval"],[ban_heap_size="128"])
  AC_DEFINE_UNQUOTED([BAN_HEAP_SIZE],[$ban_heap_size],[Size of the ban heap.])
])dnl }}}
dnl {{{ ax_arg_with_topic_heap_size
AC_DEFUN([AX_ARG_WITH_TOPIC_HEAP_SIZE],[
  AC_ARG_WITH([topic-heap-size],[AC_HELP_STRING([--topic-heap-size=<value>],[Set topic heap size (default 256).])],[topic_heap_size="$withval"],[topic_heap_size="256"])
  AC_DEFINE_UNQUOTED([TOPIC_HEAP_SIZE],[$topic_heap_size],[Size of the topic heap.])
])dnl }}}
dnl {{{ ax_arg_with_services_heap_size
AC_DEFUN([AX_ARG_WITH_SERVICES_HEAP_SIZE],[
  AC_ARG_WITH([services-heap-size],[AC_HELP_STRING([--services-heap-size=<value>],[Set services heap size (default 8).])],[services_heap_size="$withval"],[services_heap_size="8"])
  AC_DEFINE_UNQUOTED([SERVICES_HEAP_SIZE],[$services_heap_size],[Size of the services heap.])
])dnl }}}
dnl {{{ ax_arg_with_mqueue_heap_size
AC_DEFUN([AX_ARG_WITH_MQUEUE_HEAP_SIZE],[
  AC_ARG_WITH([mqueue-heap-size],[AC_HELP_STRING([--mqueue-size=<value>],[Set mqueue heap size (default 256).])],[mqueue_heap_size="$withval"],[mqueue_heap_size="256"])
  AC_DEFINE_UNQUOTED([MQUEUE_HEAP_SIZE],[$mqueue_heap_size],[Size of the floodserv mqueue heap.])
])dnl }}}
dnl {{{ ax_arg_with_fmsg_heap_size
AC_DEFUN([AX_ARG_WITH_FMSG_HEAP_SIZE],[
  AC_ARG_WITH([fmsg-heap-size],[AC_HELP_STRING([--fmsg-size=<value>],[Set fmsg heap size (default 256).])],[fmsg_heap_size="$withval"],[fmsg_heap_size="256"])
  AC_DEFINE_UNQUOTED([FMSG_HEAP_SIZE],[$fmsg_heap_size],[Size of the floodserv fmsg heap.])
])dnl }}}
dnl {{{ ax_arg_enable_halfops
AC_DEFUN([AX_ARG_ENABLE_HALFOPS],[
  AC_ARG_ENABLE([halfops],[AC_HELP_STRING([--enable-halfops],[Enable halfops support.])],[halfops="$enableval"],[halfops="no"])
  if test "$halfops" = "yes" ; then
    AC_DEFINE([HALFOPS],[1],[Define to 1 if you want halfops support.])
  fi
])dnl }}}
dnl {{{ ax_arg_enable_debugging
AC_DEFUN([AX_ARG_ENABLE_DEBUGGING],[
  AC_ARG_ENABLE([debugging],[AC_HELP_STRING([--enable-debugging],[Enable debugging.])],[debugging="$enableval"],[debugging="no"])
  if test "$debugging" = "yes" ; then
    AC_DEFINE([DEBUG],[1],[Define to 1 to enable debugging.])
    CFLAGS="$CFLAGS -Wall -Werror -g -O0"
  else
    AC_DEFINE([NDEBUG],[1],[Define to 1 to disable debugging.])
  fi
])dnl }}}
dnl {{{ ax_arg_enable_warnings
AC_DEFUN([AX_ARG_ENABLE_WARNINGS],[
  AC_ARG_ENABLE([warnings],[AC_HELP_STRING([--enable-warnings],[Enable compiler warnings.])],[warnings="$enableval"],[warnings="no"])
  if test "$warnings" = "yes" ; then
    CFLAGS="-Wcast-qual -Wmissing-declarations -Wmissing-prototypes -Wnested-externs -Wredundant-decls -Wshadow -Wwrite-strings -Wno-unused"
  fi
])dnl }}}
dnl {{{ ax_arg_enable_efence
AC_DEFUN([AX_ARG_ENABLE_EFENCE],[
  AC_ARG_ENABLE([efence],[AC_HELP_STRING([--enable-efence],[Enable compilation with ElectricFence.])],[efence="$enableval"],[efence="no"])
  if test "$efence" = "yes" ; then
    AC_SEARCH_LIBS([malloc],[efence],,[AC_MSG_ERROR([efence library not found])])
  fi
])dnl }}}
dnl {{{ ax_arg_enable_syslog
AC_DEFUN([AX_ARG_ENABLE_SYSLOG],[
  AC_CHECK_HEADERS([syslog.h sys/syslog.h])
  AC_ARG_ENABLE([syslog],[AC_HELP_STRING([--enable-syslog="EVENTS"],[Enable syslog for events: users oper (space separated in quotes; default: disabled).])],[syslog="$enableval"],[syslog="no"])
  if test "$syslog" != "no" ; then
    for option in $enableval ; do
      case "$option" in
        users) AC_DEFINE([SYSLOG_USERS],[1],[Send user log stuff to syslog.]) ;;
        oper) AC_DEFINE([SYSLOG_OPER],[1],[Send oper log stuff to syslog.]) ;;
        yes) : ;;
        *) AC_MSG_ERROR([invalid value for --enable-syslog: $option]) ;;
      esac
    done
    AC_DEFINE([USE_SYSLOG],[1],[Define to 1 if you want to use syslog.])
  fi
])dnl }}}
dnl {{{ ax_arg_with_syslog_facility
AC_DEFUN([AX_ARG_WITH_SYSLOG],[
  AC_ARG_WITH([syslog-facility],[AC_HELP_STRING([--with-syslog-facility=LOG],[Define the syslog facility to use (default: LOG_LOCAL4)])],[syslog_facility="$withval"],[syslog_facility="LOG_LOCAL4"])
  AC_DEFINE_UNQUOTED([LOG_FACILITY],[$syslog_facility],[Set to syslog facility to use.])
])dnl }}}
dnl {{{ ac_define_dir
dnl http://autoconf-archive.cryp.to/ac_define_dir.html
AC_DEFUN([AC_DEFINE_DIR], [
  prefix_NONE=
  exec_prefix_NONE=
  test "x$prefix" = xNONE && prefix_NONE=yes && prefix=$ac_default_prefix
  test "x$exec_prefix" = xNONE && exec_prefix_NONE=yes && exec_prefix=$prefix
  eval ac_define_dir="\"[$]$2\""
  eval ac_define_dir="\"$ac_define_dir\""
  AC_SUBST($1, "$ac_define_dir")
  AC_DEFINE_UNQUOTED($1, "$ac_define_dir", [$3])
  test "$prefix_NONE" && prefix=NONE
  test "$exec_prefix_NONE" && exec_prefix=NONE
])dnl }}}
]) dnl }}}
