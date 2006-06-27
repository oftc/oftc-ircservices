#ifndef DEFINESH
#define DEFINESH

#define ETCPATH     SERVICES_PREFIX "/etc/"
#define AUTOMODPATH SERVICES_PREFIX "/modules/autoload/"
#define MODPATH     SERVICES_PREFIX "/modules/"

#define CPATH       ETCPATH "services.conf"
#define DPATH       SERVICES_PREFIX

#define IRC_BUFSIZE 512
#define CONNECTTIMEOUT  30
#define READBUF_SIZE  16384
#define IRCD_MAXPARA    15     /* Maximum allowed parameters a command may have */
#define REALLEN         50
#define CHANNELLEN      200
#define IRC_MAXSID 3
#define IRC_MAXUID 6
#define TOTALSIDUID (IRC_MAXSID + IRC_MAXUID)

#define IRC_MAX(a, b)  ((a) > (b) ? (a) : (b))
#define IRC_MIN(a, b)  ((a) < (b) ? (a) : (b))

#endif
