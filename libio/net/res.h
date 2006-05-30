/*
 * res.h for referencing functions in libio/net/res.c
 *
 * $Id: res.h 364 2006-01-08 15:39:24Z michael $
 */

/* Maximum number of nameservers in /etc/resolv.conf we care about */
#define IRCD_MAXNS 2

struct DNSReply
{
  char *h_name;
  struct irc_ssaddr addr;
};

struct DNSQuery
{
#ifdef _WIN32
  dlink_node node;
  HANDLE handle;
  char reply[MAXGETHOSTSTRUCT];
#endif
  void *ptr; /* pointer used by callback to identify request */
  void (*callback)(void* vptr, struct DNSReply *reply); /* callback to call */
};

LIBIO_EXTERN struct irc_ssaddr irc_nsaddr_list[];
LIBIO_EXTERN int irc_nscount;

#ifdef IN_MISC_C
extern void init_resolver(void);
#endif
LIBIO_EXTERN void restart_resolver(void);
LIBIO_EXTERN void delete_resolver_queries(const struct DNSQuery *);
LIBIO_EXTERN void gethost_byname_type(const char *, struct DNSQuery *, int);
LIBIO_EXTERN void gethost_byname(const char *, struct DNSQuery *);
LIBIO_EXTERN void gethost_byaddr(const struct irc_ssaddr *, struct DNSQuery *);
LIBIO_EXTERN void add_local_domain(char *, size_t);
