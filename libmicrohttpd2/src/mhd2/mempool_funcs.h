/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2007--2024 Daniel Pittman and Christian Grothoff
  Copyright (C) 2016--2025 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/mempool_funcs.h
 * @brief memory pool; mostly used for efficient (de)allocation
 *        for each connection and bounding memory use for each
 *        request
 * @author Christian Grothoff
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_MEMPOOL_FUNCS_H
#define MHD_MEMPOOL_FUNCS_H 1

#include "mhd_sys_options.h"
#include "sys_base_types.h"
#include "sys_bool_type.h"
#include "mempool_types.h"


#ifdef __BIGGEST_ALIGNMENT__
/**
 * Alignment size used by memory pool function.
 */
#  define mhd_MEMPOOL_ALIGN_SIZE        __BIGGEST_ALIGNMENT__
#else  /* ! __BIGGEST_ALIGNMENT__ */
/**
 * Alignment size used by memory pool function.
 * This is 2x pointer size (similar to GNU libc).
 */
#  define mhd_MEMPOOL_ALIGN_SIZE        (2 * sizeof(void*))
#endif /* ! __BIGGEST_ALIGNMENT__ */

/**
 * Perform one-time initialisation of the internal values required for
 * memory pools functions
 */
void
mhd_init_mem_pools (void);


/**
 * Destroy a memory pool.
 *
 * @param pool memory pool to destroy
 */
MHD_INTERNAL void
mhd_pool_destroy (struct mhd_MemoryPool *restrict pool);


/**
 * Create a memory pool.
 *
 * @param max maximum size of the pool
 * @param zeroing the request zeroing of allocated and deallocated memory
 * @return pointer to the new object on success,
 *         NULL on error
 */
MHD_INTERNAL struct mhd_MemoryPool *
mhd_pool_create (size_t max,
                 enum mhd_MemPoolZeroing zeroing)
mhd_FN_RET_UNALIASED mhd_FN_OBJ_CONSTRUCTOR (mhd_pool_destroy);


/**
 * Allocate size bytes from the pool.
 *
 * @param pool memory pool to use for the operation
 * @param size number of bytes to allocate
 * @param from_end allocate from end of pool (set to 'true');
 *        use this for small, persistent allocations that
 *        will not be reallocated until pool reset
 * @return pointer to the new allocated memory region on success,
 *         NULL if the pool does not have enough free memory.
 */
MHD_INTERNAL void *
mhd_pool_allocate (struct mhd_MemoryPool *restrict pool,
                   size_t size,
                   bool from_end)
mhd_FN_RET_UNALIASED mhd_FN_RET_SIZED (2)
mhd_FN_RET_ALIGNED (mhd_MEMPOOL_ALIGN_SIZE);

/**
 * Checks whether allocated block is re-sizable in-place.
 * If block is not re-sizable in-place, it still could be shrunk, but freed
 * memory will not be re-used until reset of the pool.
 * @param pool the memory pool to use
 * @param block the pointer to the allocated block to check
 * @param block_size the size of the allocated @a block
 * @return true if block can be resized in-place in the optimal way,
 *         false otherwise
 */
MHD_INTERNAL bool
mhd_pool_is_resizable_inplace (struct mhd_MemoryPool *restrict pool,
                               void *restrict block,
                               size_t block_size);

/**
 * Try to allocate @a size bytes memory area from the @a pool.
 *
 * If allocation fails, @a required_bytes is updated with size required to be
 * freed in the @a pool from rellocatable area to allocate requested number
 * of bytes.
 * Allocated memory area is always not rellocatable ("from end").
 *
 * @param pool memory pool to use for the operation
 * @param size the size of memory in bytes to allocate
 * @param[out] required_bytes the pointer to variable to be updated with
 *                            the size of the required additional free
 *                            memory area, set to 0 if function succeeds.
 *                            Cannot be NULL.
 * @return the pointer to allocated memory area if succeed,
 *         NULL if the pool doesn't have enough space, required_bytes is updated
 *         with amount of space needed to be freed in rellocatable area or
 *         set to SIZE_MAX if requested size is too large for the pool.
 */
MHD_INTERNAL void *
mhd_pool_try_alloc (struct mhd_MemoryPool *restrict pool,
                    size_t size,
                    size_t *restrict required_bytes)
mhd_FN_RET_UNALIASED mhd_FN_RET_SIZED (2)
mhd_FN_RET_ALIGNED (mhd_MEMPOOL_ALIGN_SIZE);


/**
 * Reallocate a block of memory obtained from the pool.
 * This is particularly efficient when growing or
 * shrinking the block that was last (re)allocated.
 * If the given block is not the most recently
 * (re)allocated block, the memory of the previous
 * allocation may be not released until the pool is
 * destroyed or reset.
 *
 * @param pool memory pool to use for the operation
 * @param old the existing block
 * @param old_size the size of the existing block
 * @param new_size the new size of the block
 * @return new address of the block, or
 *         NULL if the pool cannot support @a new_size
 *         bytes (old continues to be valid for @a old_size)
 */
MHD_INTERNAL void *
mhd_pool_reallocate (struct mhd_MemoryPool *restrict pool,
                     void *restrict old,
                     size_t old_size,
                     size_t new_size)
mhd_FN_RET_SIZED (4);

/**
 * Get the size of the memory pool
 * @param pool pool to check
 * @return the maximum allocation size possible with empty memory pool
 */
MHD_INTERNAL size_t
mhd_pool_get_size (const struct mhd_MemoryPool *restrict pool)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PURE_;

/**
 * Check how much memory is left in the @a pool
 *
 * @param pool pool to check
 * @return number of bytes still available in @a pool
 */
MHD_INTERNAL size_t
mhd_pool_get_free (struct mhd_MemoryPool *restrict pool);


/**
 * Deallocate a block of memory obtained from the pool.
 *
 * If the given block is not the most recently
 * (re)allocated block, the memory of the this block
 * allocation may be not released until the pool is
 * destroyed or reset.
 *
 * @param pool memory pool to use for the operation
 * @param block the allocated block, the NULL is tolerated
 * @param block_size the size of the allocated block
 */
MHD_INTERNAL void
mhd_pool_deallocate (struct mhd_MemoryPool *restrict pool,
                     void *restrict block,
                     size_t block_size);


/**
 * Clear all entries from the memory pool except
 * for @a keep of the given @a copy_bytes.  The pointer
 * returned should be a buffer of @a new_size where
 * the first @a copy_bytes are from @a keep.
 *
 * @param pool memory pool to use for the operation
 * @param keep pointer to the entry to keep (maybe NULL)
 * @param copy_bytes how many bytes need to be kept at this address
 * @param new_size how many bytes should the allocation we return have?
 *                 (should be larger or equal to @a copy_bytes)
 * @return addr new address of @a keep (if it had to change)
 */
MHD_INTERNAL void *
mhd_pool_reset (struct mhd_MemoryPool *restrict pool,
                void *restrict keep,
                size_t copy_bytes,
                size_t new_size)
mhd_FN_RET_SIZED (4) mhd_FN_RET_ALIGNED (mhd_MEMPOOL_ALIGN_SIZE);

#endif /* ! MHD_MEMPOOL_FUNCS_H */
