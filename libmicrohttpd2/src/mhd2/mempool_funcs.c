/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2007--2024 Daniel Pittman and Christian Grothoff
  Copyright (C) 2014--2024 Evgeny Grin (Karlson2k)

  GNU libmicrohttpd is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  GNU libmicrohttpd is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  Alternatively, you can redistribute GNU libmicrohttpd and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License, or (at your option) any later version, together
  with the eCos exception, as follows:

    As a special exception, if other files instantiate templates or
    use macros or inline functions from this file, or you compile this
    file and link it with other works to produce a work based on this
    file, this file does not by itself cause the resulting work to be
    covered by the GNU General Public License. However the source code
    for this file must still be made available in accordance with
    section (3) of the GNU General Public License v2.

    This exception does not invalidate any other reasons why a work
    based on this file might be covered by the GNU General Public
    License.

  You should have received copies of the GNU Lesser General Public
  License and the GNU General Public License along with this library;
  if not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file src/mhd2/mempool_funcs.c
 * @brief memory pool
 * @author Christian Grothoff
 * @author Karlson2k (Evgeny Grin)
 * TODO:
 * + Update code style
 * + Detect mmap() in configure (it is purely optional!)
 */
#include "mhd_sys_options.h"
#include "mempool_funcs.h"
#include "compat_calloc.h"

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#include <string.h>
#include "mhd_assert.h"
#ifdef HAVE_SYS_MMAN_H
#  include <sys/mman.h>
#endif
#ifdef _WIN32
#  include <windows.h>
#endif
#ifdef HAVE_SYSCONF
#  include <unistd.h>
#  if defined(_SC_PAGE_SIZE)
#    define MHD_SC_PAGESIZE _SC_PAGE_SIZE
#  elif defined(_SC_PAGESIZE)
#    define MHD_SC_PAGESIZE _SC_PAGESIZE
#  endif /* _SC_PAGESIZE */
#endif /* HAVE_SYSCONF */

#if defined(MHD_USE_PAGESIZE_MACRO) || defined(MHD_USE_PAGE_SIZE_MACRO)
#  ifndef HAVE_SYSCONF /* Avoid duplicate include */
#    include <unistd.h>
#  endif /* HAVE_SYSCONF */
#  ifdef HAVE_LIMITS_H
#    include <limits.h>
#  endif
#  ifdef HAVE_SYS_PARAM_H
#    include <sys/param.h>
#  endif /* HAVE_SYS_PARAM_H */
#endif /* MHD_USE_PAGESIZE_MACRO || MHD_USE_PAGE_SIZE_MACRO */

#include "mhd_limits.h"

#ifndef mhd_FALLBACK_PAGE_SIZE
/**
 * Fallback value of page size
 */
#  define mhd_FALLBACK_PAGE_SIZE (4096)
#endif

#if defined(MHD_USE_PAGESIZE_MACRO)
#  define mhd_DEF_PAGE_SIZE PAGESIZE
#elif defined(MHD_USE_PAGE_SIZE_MACRO)
#  define mhd_DEF_PAGE_SIZE PAGE_SIZE
#else  /* ! PAGESIZE */
#  define mhd_DEF_PAGE_SIZE mhd_FALLBACK_PAGE_SIZE
#endif /* ! PAGESIZE */


#ifdef MHD_ASAN_POISON_ACTIVE
#include <sanitizer/asan_interface.h>
#endif /* MHD_ASAN_POISON_ACTIVE */

#if defined(MAP_ANONYMOUS)
#  define mhd_MAP_ANONYMOUS MAP_ANONYMOUS
#endif

#if ! defined(mhd_MAP_ANONYMOUS) && defined(MAP_ANON)
#  define mhd_MAP_ANONYMOUS MAP_ANON
#endif

#if defined(mhd_MAP_ANONYMOUS) || defined(_WIN32)
#  define mhd_USE_LARGE_ALLOCS 1
#endif

#ifdef mhd_USE_LARGE_ALLOCS
#  if defined(_WIN32)
#    define mhd_MAP_FAILED NULL
#  elif defined(MAP_FAILED)
#    define mhd_MAP_FAILED MAP_FAILED
#  else
#    define mhd_MAP_FAILED ((void*) -1)
#  endif
#endif

/**
 * Round up 'n' to a multiple of ALIGN_SIZE.
 */
#define mhd_ROUND_TO_ALIGN(n) \
        (((n) + (mhd_MEMPOOL_ALIGN_SIZE - 1)) \
         / (mhd_MEMPOOL_ALIGN_SIZE) * (mhd_MEMPOOL_ALIGN_SIZE))


#ifndef MHD_ASAN_POISON_ACTIVE
#  define mhd_NOSANITIZE_PTRS /**/
#  define mhd_RED_ZONE_SIZE (0)
#  define mhd_ROUND_TO_ALIGN_PLUS_RED_ZONE(n) mhd_ROUND_TO_ALIGN (n)
#  define mhd_POISON_MEMORY(pointer, size) (void) 0
#  define mhd_UNPOISON_MEMORY(pointer, size) (void) 0
/**
 * Boolean 'true' if the first pointer is less or equal the second pointer
 */
#  define mp_ptr_le_(p1,p2) \
        (((const uint8_t*) (p1)) <= ((const uint8_t*) (p2)))
/**
 * The difference in bytes between positions of the first and
 * the second pointers
 */
#  define mp_ptr_diff_(p1,p2) \
        ((size_t) (((const uint8_t*) (p1)) - ((const uint8_t*) (p2))))
#else  /* MHD_ASAN_POISON_ACTIVE */
#  define mhd_RED_ZONE_SIZE (mhd_MEMPOOL_ALIGN_SIZE)
#  define mhd_ROUND_TO_ALIGN_PLUS_RED_ZONE(n) \
        (mhd_ROUND_TO_ALIGN (n) + mhd_RED_ZONE_SIZE)
#  define mhd_POISON_MEMORY(pointer, size) \
        ASAN_POISON_MEMORY_REGION ((pointer), (size))
#  define mhd_UNPOISON_MEMORY(pointer, size) \
        ASAN_UNPOISON_MEMORY_REGION ((pointer), (size))
#  if defined(FUNC_PTRCOMPARE_CAST_WORKAROUND_WORKS)
/**
 * Boolean 'true' if the first pointer is less or equal the second pointer
 */
#    define mp_ptr_le_(p1,p2) \
        (((uintptr_t) ((const void*) (p1))) <= \
         ((uintptr_t) ((const void*) (p2))))
/**
 * The difference in bytes between positions of the first and
 * the second pointers
 */
#    define mp_ptr_diff_(p1,p2) \
        ((size_t) (((uintptr_t) ((const uint8_t*) (p1))) - \
                   ((uintptr_t) ((const uint8_t*) (p2)))))
#elif defined(FUNC_ATTR_PTRCOMPARE_WORKS) && \
  defined(FUNC_ATTR_PTRSUBTRACT_WORKS)
#    ifndef NDEBUG
/**
 * Boolean 'true' if the first pointer is less or equal the second pointer
 */
__attribute__((no_sanitize ("pointer-compare"))) static bool
mp_ptr_le_ (const void *p1, const void *p2)
{
  return (((const uint8_t *) p1) <= ((const uint8_t *) p2));
}


#    endif /* _DEBUG */


/**
 * The difference in bytes between positions of the first and
 * the second pointers
 */
__attribute__((no_sanitize ("pointer-subtract"))) static size_t
mp_ptr_diff_ (const void *p1, const void *p2)
{
  return (size_t) (((const uint8_t *) p1) - ((const uint8_t *) p2));
}


#  elif defined(FUNC_ATTR_NOSANITIZE_WORKS)
#    ifndef NDEBUG
/**
 * Boolean 'true' if the first pointer is less or equal the second pointer
 */
__attribute__((no_sanitize ("address"))) static bool
mp_ptr_le_ (const void *p1, const void *p2)
{
  return (((const uint8_t *) p1) <= ((const uint8_t *) p2));
}


    #endif /* _DEBUG */

/**
 * The difference in bytes between positions of the first and
 * the second pointers
 */
__attribute__((no_sanitize ("address"))) static size_t
mp_ptr_diff_ (const void *p1, const void *p2)
{
  return (size_t) (((const uint8_t *) p1) - ((const uint8_t *) p2));
}


#  else  /* ! FUNC_ATTR_NOSANITIZE_WORKS */
#error User-poisoning cannot be used
#  endif /* ! FUNC_ATTR_NOSANITIZE_WORKS */
#endif /* MHD_ASAN_POISON_ACTIVE */

#ifdef mhd_USE_LARGE_ALLOCS
/**
 * Size of memory page
 */
static size_t MHD_sys_page_size_ = (size_t)
#  if defined(MHD_USE_PAGESIZE_MACRO_STATIC)
                                   PAGESIZE;
#  elif defined(MHD_USE_PAGE_SIZE_MACRO_STATIC)
                                   PAGE_SIZE;
#  else  /* ! MHD_USE_PAGE_SIZE_MACRO_STATIC */
                                   mhd_FALLBACK_PAGE_SIZE; /* Default fallback value */
#  endif /* ! MHD_USE_PAGE_SIZE_MACRO_STATIC */
#endif /* mhd_USE_LARGE_ALLOCS */

void
mhd_init_mem_pools (void)
{
#ifdef mhd_USE_LARGE_ALLOCS
#ifdef MHD_SC_PAGESIZE
  long result;
  result = sysconf (MHD_SC_PAGESIZE);
  if (-1 != result)
    MHD_sys_page_size_ = (size_t) result;
  else
    MHD_sys_page_size_ = (size_t) mhd_DEF_PAGE_SIZE;
#elif defined(_WIN32)
  SYSTEM_INFO si;
  GetSystemInfo (&si);
  MHD_sys_page_size_ = (size_t) si.dwPageSize;
#else
  MHD_sys_page_size_ = (size_t) mhd_DEF_PAGE_SIZE;
#endif /* _WIN32 */
  mhd_assert (0 == (MHD_sys_page_size_ % mhd_MEMPOOL_ALIGN_SIZE));
#endif /* mhd_USE_LARGE_ALLOCS */
  (void) 0;
}


/**
 * Handle for a memory pool.  Pools are not reentrant and must not be
 * used by multiple threads.
 */
struct mhd_MemoryPool
{

  /**
   * Pointer to the pool's memory
   */
  uint8_t *memory;

  /**
   * Size of the pool.
   */
  size_t size;

  /**
   * Offset of the first unallocated byte.
   */
  size_t pos;

  /**
   * Offset of the byte after the last unallocated byte.
   */
  size_t end;

#ifdef mhd_USE_LARGE_ALLOCS
  /**
   * 'false' if pool was malloc'ed, 'true' if mmapped (VirtualAlloc'ed for W32).
   */
  bool is_large_alloc;
#endif

  /**
   * Memory allocation zeroing mode
   */
  enum mhd_MemPoolZeroing zeroing;
};


MHD_INTERNAL mhd_FN_RET_UNALIASED
mhd_FN_OBJ_CONSTRUCTOR (mhd_pool_destroy)
struct mhd_MemoryPool *
mhd_pool_create (size_t max,
                 enum mhd_MemPoolZeroing zeroing)
{
  struct mhd_MemoryPool *pool;
  size_t alloc_size;

  mhd_assert (max > 0);
  mhd_assert (mhd_RED_ZONE_SIZE < (max + mhd_RED_ZONE_SIZE));
  max += mhd_RED_ZONE_SIZE;
  alloc_size = 0;
  pool = (struct mhd_MemoryPool *) malloc (sizeof (struct mhd_MemoryPool));
  if (NULL == pool)
    return NULL;
  pool->zeroing = zeroing;
#ifdef mhd_USE_LARGE_ALLOCS
  pool->is_large_alloc = false;
  if ( (max <= 32 * 1024) ||
       (max < MHD_sys_page_size_ * 4 / 3) )
  {
    pool->memory = (uint8_t *) mhd_MAP_FAILED;
  }
  else
  {
    /* Round up allocation to page granularity. */
    alloc_size = max + MHD_sys_page_size_ - 1;
    alloc_size -= alloc_size % MHD_sys_page_size_;
#  if defined(mhd_MAP_ANONYMOUS)
    pool->memory = (uint8_t *) mmap (NULL,
                                     alloc_size,
                                     PROT_READ | PROT_WRITE,
                                     MAP_PRIVATE | mhd_MAP_ANONYMOUS,
                                     -1,
                                     0);
#  else  /* ! mhd_MAP_ANONYMOUS */
    pool->memory = (uint8_t *) VirtualAlloc (NULL,
                                             alloc_size,
                                             MEM_COMMIT | MEM_RESERVE,
                                             PAGE_READWRITE);
#  endif /* ! mhd_MAP_ANONYMOUS */
  }
  if (mhd_MAP_FAILED != pool->memory)
    pool->is_large_alloc = true;
  else
#endif /* mhd_USE_LARGE_ALLOCS */
  if (! 0)
  {
    alloc_size = mhd_ROUND_TO_ALIGN (max);
    if (MHD_MEMPOOL_ZEROING_NEVER == zeroing)
      pool->memory = (uint8_t *) malloc (alloc_size);
    else
      pool->memory = (uint8_t *) mhd_calloc (1, alloc_size);
    if (((uint8_t *) NULL) == pool->memory)
    {
      free (pool);
      return NULL;
    }
  }
  mhd_assert (0 == (((uintptr_t) pool->memory) % mhd_MEMPOOL_ALIGN_SIZE));
  pool->pos = 0;
  pool->end = alloc_size;
  pool->size = alloc_size;
  mhd_assert (0 < alloc_size);
  mhd_POISON_MEMORY (pool->memory, pool->size);
  return pool;
}


MHD_INTERNAL void
mhd_pool_destroy (struct mhd_MemoryPool *restrict pool)
{
  if (NULL == pool)
    return;

  mhd_assert (pool->end >= pool->pos);
  mhd_assert (pool->size >= pool->end - pool->pos);
  mhd_assert (pool->pos == mhd_ROUND_TO_ALIGN (pool->pos));
  mhd_UNPOISON_MEMORY (pool->memory, pool->size);
#ifdef mhd_USE_LARGE_ALLOCS
  if (pool->is_large_alloc)
  {
#  if defined(mhd_MAP_ANONYMOUS)
    munmap (pool->memory,
            pool->size);
#  else
    VirtualFree (pool->memory,
                 0,
                 MEM_RELEASE);
#  endif
  }
  else
#endif /* mhd_USE_LARGE_ALLOCS*/
  if (! 0)
    free (pool->memory);

  free (pool);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PURE_ size_t
mhd_pool_get_size (const struct mhd_MemoryPool *restrict pool)
{
  return (pool->size - mhd_RED_ZONE_SIZE);
}


MHD_INTERNAL size_t
mhd_pool_get_free (struct mhd_MemoryPool *restrict pool)
{
  mhd_assert (pool->end >= pool->pos);
  mhd_assert (pool->size >= pool->end - pool->pos);
  mhd_assert (pool->pos == mhd_ROUND_TO_ALIGN (pool->pos));
#ifdef MHD_ASAN_POISON_ACTIVE
  if ((pool->end - pool->pos) <= mhd_RED_ZONE_SIZE)
    return 0;
#endif /* MHD_ASAN_POISON_ACTIVE */
  return (pool->end - pool->pos) - mhd_RED_ZONE_SIZE;
}


MHD_INTERNAL mhd_FN_RET_UNALIASED
mhd_FN_RET_SIZED (2)
mhd_FN_RET_ALIGNED (mhd_MEMPOOL_ALIGN_SIZE) void *
mhd_pool_allocate (struct mhd_MemoryPool *restrict pool,
                   size_t size,
                   bool from_end)
{
  void *ret;
  size_t asize;

  mhd_assert (pool->end >= pool->pos);
  mhd_assert (pool->size >= pool->end - pool->pos);
  mhd_assert (pool->pos == mhd_ROUND_TO_ALIGN (pool->pos));
  asize = mhd_ROUND_TO_ALIGN_PLUS_RED_ZONE (size);
  if ( (0 == asize) && (0 != size) )
    return NULL; /* size too close to SIZE_MAX */
  if (asize > pool->end - pool->pos)
    return NULL;
  if (from_end)
  {
    ret = &pool->memory[pool->end - asize];
    pool->end -= asize;
  }
  else
  {
    ret = &pool->memory[pool->pos];
    pool->pos += asize;
  }
  mhd_UNPOISON_MEMORY (ret, size);
  return ret;
}


MHD_INTERNAL bool
mhd_pool_is_resizable_inplace (struct mhd_MemoryPool *restrict pool,
                               void *restrict block,
                               size_t block_size)
{
  mhd_assert (pool->end >= pool->pos);
  mhd_assert (pool->size >= pool->end - pool->pos);
  mhd_assert (block != NULL || block_size == 0);
  mhd_assert (pool->size >= block_size);
  if (NULL != block)
  {
    const size_t block_offset = mp_ptr_diff_ (block, pool->memory);
    mhd_assert (mp_ptr_le_ (pool->memory, block));
    mhd_assert (pool->size >= block_offset);
    mhd_assert (pool->size >= block_offset + block_size);
    return (pool->pos ==
            mhd_ROUND_TO_ALIGN_PLUS_RED_ZONE (block_offset + block_size));
  }
  return false; /* Unallocated blocks cannot be resized in-place */
}


MHD_INTERNAL mhd_FN_RET_UNALIASED
mhd_FN_RET_SIZED (2)
mhd_FN_RET_ALIGNED (mhd_MEMPOOL_ALIGN_SIZE) void *
mhd_pool_try_alloc (struct mhd_MemoryPool *restrict pool,
                    size_t size,
                    size_t *restrict required_bytes)
{
  void *ret;
  size_t asize;

  mhd_assert (pool->end >= pool->pos);
  mhd_assert (pool->size >= pool->end - pool->pos);
  mhd_assert (pool->pos == mhd_ROUND_TO_ALIGN (pool->pos));
  asize = mhd_ROUND_TO_ALIGN_PLUS_RED_ZONE (size);
  if ( (0 == asize) && (0 != size) )
  { /* size is too close to SIZE_MAX, very unlikely */
    *required_bytes = SIZE_MAX;
    return NULL;
  }
  if (asize > pool->end - pool->pos)
  {
    mhd_assert ((pool->end - pool->pos) == \
                mhd_ROUND_TO_ALIGN (pool->end - pool->pos));
    if (asize <= pool->end)
      *required_bytes = asize - (pool->end - pool->pos);
    else
      *required_bytes = SIZE_MAX;
    return NULL;
  }
  *required_bytes = 0;
  ret = &pool->memory[pool->end - asize];
  pool->end -= asize;
  mhd_UNPOISON_MEMORY (ret, size);
  return ret;
}


MHD_INTERNAL
mhd_FN_RET_SIZED (4)
void *
mhd_pool_reallocate (struct mhd_MemoryPool *restrict pool,
                     void *restrict old,
                     size_t old_size,
                     size_t new_size)
{
  size_t asize;
  uint8_t *new_blc;

  mhd_assert (pool->end >= pool->pos);
  mhd_assert (pool->size >= pool->end - pool->pos);
  mhd_assert (old != NULL || old_size == 0);
  mhd_assert (pool->size >= old_size);
  mhd_assert (pool->pos == mhd_ROUND_TO_ALIGN (pool->pos));
#if defined(MHD_ASAN_POISON_ACTIVE) && defined(HAVE___ASAN_REGION_IS_POISONED)
  mhd_assert (NULL == __asan_region_is_poisoned (old, old_size));
#endif /* MHD_ASAN_POISON_ACTIVE && HAVE___ASAN_REGION_IS_POISONED */

  if (NULL != old)
  {   /* Have previously allocated data */
    const size_t old_offset = mp_ptr_diff_ (old, pool->memory);
    const bool shrinking = (old_size > new_size);

    mhd_assert (mp_ptr_le_ (pool->memory, old));
    /* (pool->memory + pool->size >= (uint8_t*) old + old_size) */
    mhd_assert ((pool->size - mhd_RED_ZONE_SIZE) >= (old_offset + old_size));
    /* Blocks "from the end" must not be reallocated */
    /* (old_size == 0 || pool->memory + pool->pos > (uint8_t*) old) */
    mhd_assert ((old_size == 0) || \
                (pool->pos > old_offset));
    mhd_assert ((old_size == 0) || \
                ((pool->end - mhd_RED_ZONE_SIZE) >= (old_offset + old_size)));
    /* Try resizing in-place */
    if (shrinking)
    {     /* Shrinking in-place, zero-out freed part */
      if (MHD_MEMPOOL_ZEROING_ON_RESET < pool->zeroing)
        memset ((uint8_t *) old + new_size, 0, old_size - new_size);
      mhd_POISON_MEMORY ((uint8_t *) old + new_size, old_size - new_size);
    }
    if (pool->pos ==
        mhd_ROUND_TO_ALIGN_PLUS_RED_ZONE (old_offset + old_size))
    {     /* "old" block is the last allocated block */
      const size_t new_apos =
        mhd_ROUND_TO_ALIGN_PLUS_RED_ZONE (old_offset + new_size);
      if (! shrinking)
      {                               /* Grow in-place, check for enough space. */
        if ( (new_apos > pool->end) ||
             (new_apos < pool->pos) ) /* Value wrap */
          return NULL;                /* No space */
      }
      /* Resized in-place */
      pool->pos = new_apos;
      mhd_UNPOISON_MEMORY (old, new_size);
      return old;
    }
    if (shrinking)
      return old;   /* Resized in-place, freed part remains allocated */
  }
  /* Need to allocate new block */
  asize = mhd_ROUND_TO_ALIGN_PLUS_RED_ZONE (new_size);
  if ( ( (0 == asize) &&
         (0 != new_size) ) || /* Value wrap, too large new_size. */
       (asize > pool->end - pool->pos) ) /* Not enough space */
    return NULL;

  new_blc = pool->memory + pool->pos;
  pool->pos += asize;

  mhd_UNPOISON_MEMORY (new_blc, new_size);
  if (0 != old_size)
  {
    /* Move data to new block, old block remains allocated */
    memcpy (new_blc, old, old_size);
    /* Zero-out old block */
    if (MHD_MEMPOOL_ZEROING_ON_RESET < pool->zeroing)
      memset (old, 0, old_size);
    mhd_POISON_MEMORY (old, old_size);
  }
  return new_blc;
}


MHD_INTERNAL void
mhd_pool_deallocate (struct mhd_MemoryPool *restrict pool,
                     void *restrict block,
                     size_t block_size)
{
  mhd_assert (pool->end >= pool->pos);
  mhd_assert (pool->size >= pool->end - pool->pos);
  mhd_assert (block != NULL || block_size == 0);
  mhd_assert (pool->size >= block_size);
  mhd_assert (pool->pos == mhd_ROUND_TO_ALIGN (pool->pos));

  if (NULL != block)
  {   /* Have previously allocated data */
    const size_t block_offset = mp_ptr_diff_ (block, pool->memory);
    mhd_assert (mp_ptr_le_ (pool->memory, block));
    mhd_assert (block_offset <= pool->size);
    mhd_assert ((block_offset != pool->pos) || (block_size == 0));
    /* Zero-out deallocated region */
    if (0 != block_size)
    {
      if (MHD_MEMPOOL_ZEROING_ON_RESET < pool->zeroing)
        memset (block, 0, block_size);
      mhd_POISON_MEMORY (block, block_size);
    }
#if ! defined(MHD_FAVOR_SMALL_CODE) && ! defined(MHD_ASAN_POISON_ACTIVE)
    else
      return; /* Zero size, no need to do anything */
#endif /* ! MHD_FAVOR_SMALL_CODE && ! MHD_ASAN_POISON_ACTIVE */
    if (block_offset <= pool->pos)
    {
      /* "Normal" block, not allocated "from the end". */
      const size_t alg_end =
        mhd_ROUND_TO_ALIGN_PLUS_RED_ZONE (block_offset + block_size);
      mhd_assert (alg_end <= pool->pos);
      if (alg_end == pool->pos)
      {
        /* The last allocated block, return deallocated block to the pool */
        size_t alg_start = mhd_ROUND_TO_ALIGN (block_offset);
        mhd_assert (alg_start >= block_offset);
#if defined(MHD_ASAN_POISON_ACTIVE)
        if (alg_start != block_offset)
        {
          mhd_POISON_MEMORY (pool->memory + block_offset, \
                             alg_start - block_offset);
        }
        else if (0 != alg_start)
        {
          bool need_red_zone_before;
          mhd_assert (mhd_RED_ZONE_SIZE <= alg_start);
#if defined(HAVE___ASAN_REGION_IS_POISONED)
          need_red_zone_before =
            (NULL == __asan_region_is_poisoned (pool->memory
                                                + alg_start
                                                - mhd_RED_ZONE_SIZE,
                                                mhd_RED_ZONE_SIZE));
#elif defined(HAVE___ASAN_ADDRESS_IS_POISONED)
          need_red_zone_before =
            (0 == __asan_address_is_poisoned (pool->memory + alg_start - 1));
#else  /* ! HAVE___ASAN_ADDRESS_IS_POISONED */
          need_red_zone_before = true; /* Unknown, assume new red zone needed */
#endif /* ! HAVE___ASAN_ADDRESS_IS_POISONED */
          if (need_red_zone_before)
          {
            mhd_POISON_MEMORY (pool->memory + alg_start, mhd_RED_ZONE_SIZE);
            alg_start += mhd_RED_ZONE_SIZE;
          }
        }
#endif /* MHD_ASAN_POISON_ACTIVE */
        mhd_assert (alg_start <= pool->pos);
        mhd_assert (alg_start == mhd_ROUND_TO_ALIGN (alg_start));
        pool->pos = alg_start;
      }
    }
    else
    {
      /* Allocated "from the end" block. */
      /* The size and the pointers of such block should not be manipulated by
         MHD code (block split is disallowed). */
      mhd_assert (block_offset >= pool->end);
      mhd_assert (mhd_ROUND_TO_ALIGN (block_offset) == block_offset);
      if (block_offset == pool->end)
      {
        /* The last allocated block, return deallocated block to the pool */
        const size_t alg_end =
          mhd_ROUND_TO_ALIGN_PLUS_RED_ZONE (block_offset + block_size);
        pool->end = alg_end;
      }
    }
  }
}


MHD_INTERNAL
mhd_FN_RET_SIZED (4) mhd_FN_RET_ALIGNED (mhd_MEMPOOL_ALIGN_SIZE)
void *
mhd_pool_reset (struct mhd_MemoryPool *restrict pool,
                void *restrict keep,
                size_t copy_bytes,
                size_t new_size)
{
  mhd_assert (pool->end >= pool->pos);
  mhd_assert (pool->size >= pool->end - pool->pos);
  mhd_assert (copy_bytes <= new_size);
  mhd_assert (copy_bytes + mhd_RED_ZONE_SIZE <= pool->size);
  mhd_assert (keep != NULL || copy_bytes == 0);
  mhd_assert (keep == NULL || mp_ptr_le_ (pool->memory, keep));
  /* (keep == NULL || pool->memory + pool->size >= (uint8_t*) keep + copy_bytes) */
  mhd_assert ((keep == NULL) || \
              (pool->size >= mp_ptr_diff_ (keep, pool->memory) + copy_bytes));
#if defined(MHD_ASAN_POISON_ACTIVE) && defined(HAVE___ASAN_REGION_IS_POISONED)
  mhd_assert (NULL == __asan_region_is_poisoned (keep, copy_bytes));
#endif /* MHD_ASAN_POISON_ACTIVE && HAVE___ASAN_REGION_IS_POISONED */
  mhd_UNPOISON_MEMORY (pool->memory, new_size);
  if ( (NULL != keep) &&
       (keep != pool->memory) )
  {
    if (0 != copy_bytes)
      memmove (pool->memory,
               keep,
               copy_bytes);
  }
  if ((MHD_MEMPOOL_ZEROING_NEVER != pool->zeroing) &&
      (pool->size > copy_bytes))
  {
    size_t to_zero;   /** Size of area to zero-out */

    to_zero = pool->size - copy_bytes;
    mhd_UNPOISON_MEMORY (pool->memory + copy_bytes, to_zero);
#if defined(mhd_USE_LARGE_ALLOCS) && defined(_WIN32)
    if (pool->is_large_alloc)
    {
      size_t to_recommit;     /** Size of decommitted and re-committed area. */
      uint8_t *recommit_addr;
      /* Round down to page size */
      to_recommit = to_zero - to_zero % MHD_sys_page_size_;
      recommit_addr = pool->memory + pool->size - to_recommit;

      /* De-committing and re-committing again clear memory and make
       * pages free / available for other needs until accessed. */
      if (VirtualFree (recommit_addr,
                       to_recommit,
                       MEM_DECOMMIT))
      {
        to_zero -= to_recommit;

        if (recommit_addr != VirtualAlloc (recommit_addr,
                                           to_recommit,
                                           MEM_COMMIT,
                                           PAGE_READWRITE))
          abort ();      /* Serious error, must never happen */
      }
    }
#endif /* mhd_USE_LARGE_ALLOCS && _WIN32 */
    memset (&pool->memory[copy_bytes],
            0,
            to_zero);
  }
  pool->pos = mhd_ROUND_TO_ALIGN_PLUS_RED_ZONE (new_size);
  pool->end = pool->size;
  mhd_POISON_MEMORY (((uint8_t *) pool->memory) + new_size, \
                     pool->size - new_size);
  return pool->memory;
}


/* end of memorypool.c */
