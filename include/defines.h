/* TODO: add copyright block */

#ifndef INCLUDED_defines_h
#define INCLUDED_defines_h

#define AUTOMODPATH LIBDIR "/" PACKAGE "/autoload/"
#define MODPATH     LIBDIR "/" PACKAGE "/"
#define LANGPATH    DATADIR "/" PACKAGE "/"
#define CPATH       SYSCONFDIR "/services.conf"
#define DPATH       SYSCONFDIR
#define LPATH       DATADIR "/services.log"
#define DBLPATH     DATADIR "/db.log"
#define PPATH       SYSCONFDIR "/services.pid"

#define IRC_BUFSIZE     512
#define CONNECTTIMEOUT   30
#define READBUF_SIZE  16384
#define IRCD_MAXPARA     15     /* Maximum allowed parameters a command may have */
#define REALLEN          50
#define CHANNELLEN      200
#define KICKLEN         160
#define KEYLEN           24
#define REASONLEN       120
#define TOPICLEN        300
#define PASSLEN          40
#define SALTLEN          16
#define DATALEN         255
#define USERHOSTLEN     USERLEN+HOSTLEN+1+1
#define USERHOST_REPLYLEN       (NICKLEN+HOSTLEN+USERLEN+5)

#define IRC_MAXSID 3
#define IRC_MAXUID 6
#define TOTALSIDUID (IRC_MAXSID + IRC_MAXUID)

#define IRC_MAX(a, b)  ((a) > (b) ? (a) : (b))
#define IRC_MIN(a, b)  ((a) < (b) ? (a) : (b))

#define TRUE 1
#define FALSE 0

#ifndef _WIN32
# define EXTERN extern
#else
# ifdef IN_IRCD
#  define EXTERN extern __declspec(dllexport)
# else
#  define EXTERN extern __declspec(dllimport)
# endif
# define _modinit   __declspec(dllexport) _modinit
# define _moddeinit __declspec(dllexport) _moddeinit
# define _version   __declspec(dllexport) _version
#endif

#ifdef HAVE_STRTOK_R
# define strtoken(x, y, z) strtok_r(y, z, x)
#endif

#endif /* INCLUDED_defines_h */
