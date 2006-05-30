/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  fdlist.h: The file descriptor list header.
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id: fdlist.h 153 2005-10-17 21:20:34Z adx $
 */

#define FILEIO_V2

#define FD_DESC_SZ      128  /* hostlen + comment */
#define LOWEST_SAFE_FD  4    /* skip stdin, stdout, stderr, and profiler */

enum {
    COMM_OK,
    COMM_ERR_BIND,
    COMM_ERR_DNS,
    COMM_ERR_TIMEOUT,
    COMM_ERR_CONNECT,
    COMM_ERROR,
    COMM_ERR_MAX
};

struct _fde;
struct DNSQuery;

/* Callback for completed IO events */
typedef void PF(struct _fde *, void *);

/* Callback for completed connections */
typedef void CNCB(struct _fde *, int, void *);

/* This is to get around the fact that some implementations have ss_len and
 * others do not
 */
struct irc_ssaddr
{
  struct sockaddr_storage ss;
  unsigned char ss_len;
  in_port_t ss_port;
};

typedef struct _fde {
  /* New-school stuff, again pretty much ripped from squid */
  /*
   * Yes, this gives us only one pending read and one pending write per
   * filedescriptor. Think though: when do you think we'll need more?
   */
  int fd;		/* So we can use the fde_t as a callback ptr */
  int comm_index;	/* where in the poll list we live */
  int evcache;          /* current fd events as set up by the underlying I/O */
  char desc[FD_DESC_SZ];
  PF *read_handler;
  void *read_data;
  PF *write_handler;
  void *write_data;
  PF *timeout_handler;
  void *timeout_data;
  time_t timeout;
  PF *flush_handler;
  void *flush_data;
  time_t flush_timeout;
  struct DNSQuery *dns_query;
  struct {
    unsigned int open:1;
    unsigned int is_socket:1;
#ifdef HAVE_LIBCRYPTO
    unsigned int pending_read:1;
#endif
  } flags;

  struct {
    /* We don't need the host here ? */
    struct irc_ssaddr S;
    struct irc_ssaddr hostaddr;
    CNCB *callback;
    void *data;
    /* We'd also add the retry count here when we get to that -- adrian */
  } connect;
#ifdef HAVE_LIBCRYPTO
  SSL *ssl;
#endif
  struct _fde *hnext;
} fde_t;

#define FD_HASH_SIZE CLIENT_HEAP_SIZE

LIBIO_EXTERN int number_fd;
LIBIO_EXTERN int hard_fdlimit;
LIBIO_EXTERN fde_t *fd_hash[];
LIBIO_EXTERN fde_t *fd_next_in_loop;
LIBIO_EXTERN struct Callback *fdlimit_cb;

#ifdef IN_MISC_C
extern void fdlist_init(void);
#endif
LIBIO_EXTERN fde_t *lookup_fd(int);
LIBIO_EXTERN void fd_open(fde_t *, int, int, const char *);
LIBIO_EXTERN void fd_close(fde_t *);
#ifndef __GNUC__
LIBIO_EXTERN void fd_note(fde_t *, const char *format, ...);
#else
LIBIO_EXTERN void  fd_note(fde_t *, const char *format, ...)
  __attribute__((format (printf, 2, 3)));
#endif
#ifdef IN_MISC_C
extern void close_standard_fds(void);
#endif
LIBIO_EXTERN void close_fds(fde_t *);
LIBIO_EXTERN void recalc_fdlimit(void *);
