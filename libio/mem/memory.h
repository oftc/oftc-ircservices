/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  memory.h: A header for the memory functions.
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

#ifndef INCLUDED_libio_mem_memory_h
#define INCLUDED_libio_mem_memory_h
LIBIO_EXTERN void (* outofmemory) (void);

LIBIO_EXTERN void *MyMalloc(size_t);
LIBIO_EXTERN void *MyRealloc(void *, size_t);
LIBIO_EXTERN void MyFree(void *);
LIBIO_EXTERN void _DupString(char **, const char *);
LIBIO_EXTERN void mem_frob(void *, int);

/* forte (and maybe others) don't like double declarations, 
 * so we don't declare the inlines unless GNUC
 */
#if defined(__GNUC__) && !defined(__cplusplus)
LIBIO_EXTERN inline void *
MyMalloc(size_t size)
{
  void *ret = calloc(1, size);

  if (ret == NULL)
    outofmemory();
  return(ret);
}

LIBIO_EXTERN inline void *
MyRealloc(void *x, size_t y)
{
  void *ret = realloc(x, y);

  if (ret == NULL)
    outofmemory();
  return(ret);    
}

LIBIO_EXTERN inline void
MyFree(void *x)
{
  if (x != NULL)
    free(x);
}

LIBIO_EXTERN inline void
_DupString(char **x, const char *y)
{
  if(y == NULL)
    return;

  (*x) = (char*)malloc(strlen(y) + 1);

  if (x == NULL)
    outofmemory();
  strcpy((*x), y); 
}
#endif /* __GNUC__ */

#define DupString(x,y) _DupString(&x, y)

#endif /* INCLUDED_libio_mem_memory_h */
