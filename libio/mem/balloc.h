/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
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
 */

/*! \file balloc.h
 * \brief A block allocator
 * \version $Id$
 * \todo Get rid of all typedefs in this file
 */

#ifndef INCLUDED_libio_mem_balloc_h
#define INCLUDED_libio_mem_balloc_h

#include "libioinc.h"
#if USE_BLOCK_ALLOC

/*! \brief Block contains status information for
 *         an allocated block in our heap.
 */
struct Block {
  int		freeElems;	/*!< Number of available elems */
  size_t	alloc_size;	/*!< Size of data space for each block */
  struct Block*	next;		/*!< Next in our chain of blocks */
  void*		elems;		/*!< Points to allocated memory */
  dlink_list	free_list;	/*!< Chain of free memory blocks */
};

typedef struct Block Block;

struct MemBlock {
  dlink_node self;		/*!< Node for linking into free_list */
  Block *block;			/*!< Which block we belong to */
};
typedef struct MemBlock MemBlock;

/*! \brief BlockHeap contains the information for the root node of the
 *         memory heap.
 */
struct BlockHeap {
   size_t  elemSize;            /*!< Size of each element to be stored */
   int     elemsPerBlock;       /*!< Number of elements per block */
   int     blocksAllocated;     /*!< Number of blocks allocated */
   int     freeElems;           /*!< Number of free elements */
   Block*  base;                /*!< Pointer to first block */
   const char *name;		/*!< Name of the heap */
   dlink_node node;
};

typedef struct BlockHeap BlockHeap;

struct BlockHeapInfo {
  dlink_node node;
  char *name;
  size_t used_mem;
  size_t free_mem;
  size_t size_mem;
  unsigned int used_elm;
  unsigned int free_elm;
  unsigned int size_elm;
};

LIBIO_EXTERN int        BlockHeapFree(BlockHeap *, void *);
LIBIO_EXTERN void *     BlockHeapAlloc(BlockHeap *);

LIBIO_EXTERN BlockHeap* BlockHeapCreate(const char *const, size_t, int);
LIBIO_EXTERN int        BlockHeapDestroy(BlockHeap *);
#ifdef IN_MISC_C
extern void	  initBlockHeap(void);
#endif

LIBIO_EXTERN size_t block_heap_get_used_mem(const BlockHeap *);
LIBIO_EXTERN size_t block_heap_get_free_mem(const BlockHeap *);
LIBIO_EXTERN size_t block_heap_get_size_mem(const BlockHeap *);
LIBIO_EXTERN unsigned int block_heap_get_used_elm(const BlockHeap *);
LIBIO_EXTERN unsigned int block_heap_get_free_elm(const BlockHeap *);
LIBIO_EXTERN unsigned int block_heap_get_size_elm(const BlockHeap *);
LIBIO_EXTERN const dlink_list *block_heap_get_heap_list(void);
LIBIO_EXTERN dlink_list *block_heap_get_usage();

#else /* not USE_BLOCK_ALLOC */

typedef struct BlockHeap BlockHeap;
/* This is really kludgy, passing ints as pointers is always bad. */
#define BlockHeapCreate(blah, es, epb) ((BlockHeap*)(es))
#define BlockHeapAlloc(x) MyMalloc((int)x)
#define BlockHeapFree(x,y) MyFree(y)

#endif /* USE_BLOCK_ALLOC */
#endif /* INCLUDED_libio_mem_balloc_h */
