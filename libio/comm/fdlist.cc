/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  fdlist.c: Maintains a list of file descriptors.
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
 *  $Id$
 */
#include "libioinc.h"
#include "rlimits.h"

fde_t *fd_hash[FD_HASH_SIZE];
fde_t *fd_next_in_loop = NULL;
int number_fd = LEAKED_FDS;
int hard_fdlimit = 0;
struct Callback *fdlimit_cb = NULL;

static void *
changing_fdlimit(va_list args)
{
  hard_fdlimit = va_arg(args, int);
  return NULL;
}

void
fdlist_init(void)
{
  memset(&fd_hash, 0, sizeof(fd_hash));

  fdlimit_cb = register_callback("changing_fdlimit", changing_fdlimit);
  eventAddIsh("recalc_fdlimit", recalc_fdlimit, NULL, 58);
  recalc_fdlimit(NULL);
}

void
recalc_fdlimit(void *unused)
{
#ifdef _WIN32
  /* this is what WSAStartup() usually returns. Even though they say
   * the value is for compatibility reasons and should be ignored,
   * we actually can create even more sockets... */
  hard_fdlimit = 32767;
#else
  int fdmax;
  struct rlimit limit;

  if (!getrlimit(RLIMIT_FD_MAX, &limit))
  {
    limit.rlim_cur = limit.rlim_max;
    setrlimit(RLIMIT_FD_MAX, &limit);
  }

  fdmax = getdtablesize();

  /*
   * under no condition shall this raise over 65536
   * for example user ip heap is sized 2*hard_fdlimit
   */
  fdmax = LIBIO_MIN(fdmax, 65536);

  if (fdmax != hard_fdlimit)
    execute_callback(fdlimit_cb, fdmax);
#endif
}

static inline unsigned int
hash_fd(int fd)
{
#ifdef _WIN32
  return ((((unsigned) fd) >> 2) % FD_HASH_SIZE);
#else
  return (((unsigned) fd) % FD_HASH_SIZE);
#endif
}

fde_t *
lookup_fd(int fd)
{
  fde_t *F = fd_hash[hash_fd(fd)];

  for (; F; F = F->hnext)
    if (F->fd == fd)
      return F;

  return NULL;
}

/* Called to open a given filedescriptor */
void
fd_open(fde_t *F, int fd, int is_socket, const char *desc)
{
  unsigned int hashv = hash_fd(fd);
  assert(fd >= 0);

  F->fd = fd;
  F->comm_index = -1;

  if (desc)
    strlcpy(F->desc, desc, sizeof(F->desc));

  /* Note: normally we'd have to clear the other flags,
   * but currently F is always cleared before calling us.. */
  F->flags.open = 1;
  F->flags.is_socket = is_socket;
  F->hnext = fd_hash[hashv];
  fd_hash[hashv] = F;

  ++number_fd;
}

/* Called to close a given filedescriptor */
void
fd_close(fde_t *F)
{
  unsigned int hashv = hash_fd(F->fd);

  if (F == fd_next_in_loop)
    fd_next_in_loop = F->hnext;

  if (F->flags.is_socket)
    comm_setselect(F, COMM_SELECT_WRITE | COMM_SELECT_READ, NULL, NULL, 0);

  if (F->dns_query != NULL)
  {
    delete_resolver_queries(F->dns_query);
    MyFree(F->dns_query);
  }

#ifdef HAVE_LIBCRYPTO
  if (F->ssl)
    SSL_free(F->ssl);
#endif

  if (fd_hash[hashv] == F)
    fd_hash[hashv] = F->hnext;
  else {
    fde_t *prev;

    /* let it core if not found */
    for (prev = fd_hash[hashv]; prev->hnext != F; prev = prev->hnext)
      ;
    prev->hnext = F->hnext;
  }

  /* Unlike squid, we're actually closing the FD here! -- adrian */
#ifdef _WIN32
  if (F->flags.is_socket)
    closesocket(F->fd);
  else
    CloseHandle((HANDLE)F->fd);
#else
  close(F->fd);
#endif
  --number_fd;

  /* Wouldn't be "F->flags.open = 0" enough? */
  memset(F, 0, sizeof(fde_t));
}

/*
 * fd_note() - set the fd note
 *
 * Note: must be careful not to overflow fd_table[fd].desc when
 *       calling.
 */
void
fd_note(fde_t *F, const char *format, ...)
{
  va_list args;

  if (format != NULL)
  {
    va_start(args, format);
    vsnprintf(F->desc, sizeof(F->desc), format, args);
    va_end(args);
  }
  else
    F->desc[0] = '\0';
}

/* Make sure stdio descriptors (0-2) and profiler descriptor (3)
 * always go somewhere harmless.  Use -foreground for profiling
 * or executing from gdb */
#ifndef _WIN32
void
close_standard_fds(void)
{
  int i;

  for (i = 0; i < LOWEST_SAFE_FD; i++)
  {
    close(i);

    if (open(PATH_DEVNULL, O_RDWR) < 0)
      exit(-1); /* we're hosed if we can't even open /dev/null */
  }
}
#endif

void
close_fds(fde_t *one)
{
  int i;
  fde_t *F;

  for (i = 0; i < FD_HASH_SIZE; ++i)
  {
    for (F = fd_hash[i]; F != NULL; F = F->hnext)
    {
      if (F != one)
      {
#ifdef _WIN32
        if (F->flags.is_socket)
          closesocket(F->fd);
        else
	  CloseHandle((HANDLE)F->fd);
#else
        close(F->fd);
#endif	 
      }
    }
  }
}
