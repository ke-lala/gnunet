/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2025 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/mhd_predict.h
 * @brief  Macros that provide the compiler with hints about
 *         the anticipated outcomes of conditional expressions.
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_PREDICT_H
#define MHD_PREDICT_H 1

#include "mhd_sys_options.h"

/**
 * @defgroup mhd2_predict Branch prediction hints
 *
 * Internal helpers to annotate most probable branch outcomes.
 *
 * @warning Misusing prediction (marking the uncommon path as common) can harm
 *          performance. Measuring before and after is highly recommended.
 *
 * @see MHD_NO_PREDICT_COND
 *
 * @{
 */


/**
 * @def MHD_NO_PREDICT_COND
 *
 * When defined, disables all prediction hints regardless of compiler
 * support, turning the macros into plain boolean expressions.
 *
 * This can be useful for benchmarking or debugging to assess the impact of
 * branch prediction hints on a given workload and compiler.
 */

#ifdef MHD_NO_PREDICT_COND
#  ifdef MHD_HAVE___BUILTIN_EXPECT_WITH_PROBABILITY
#    undef MHD_HAVE___BUILTIN_EXPECT_WITH_PROBABILITY
#  endif
#  ifdef MHD_HAVE___BUILTIN_EXPECT
#    undef MHD_HAVE___BUILTIN_EXPECT
#  endif
#else
#  ifdef __has_builtin
#    if ! defined(MHD_HAVE___BUILTIN_EXPECT_WITH_PROBABILITY) && \
  __has_builtin (__builtin_expect_with_probability)
#      define MHD_HAVE___BUILTIN_EXPECT_WITH_PROBABILITY        1
#    endif
#    if ! defined(MHD_HAVE___BUILTIN_EXPECT) && __has_builtin (__builtin_expect)
#      define MHD_HAVE___BUILTIN_EXPECT         1
#    endif
#  endif
#endif


/*
 * True/false primitives with direct probability specification
 */

/**
 * @def mhd_COND_PRED_TRUE_P(cond, prob)
 *
 * Hint that @p cond is expected to be true with probability @p prob.
 *
 * @param cond  Boolean-like expression; evaluated exactly once.
 * @param prob  Anticipated probability in the range [0.0, 1.0].
 *              Only compile-time constants; avoid 0.0 or 1.0.
 *
 * Expands to compiler-specific intrinsics when available; otherwise to
 * a boolean normalisation of @p cond.
 *
 * @see mhd_COND_PRED_FALSE_P
 */

/**
 * @def mhd_COND_PRED_FALSE_P(cond, prob)
 *
 * Hint that @p cond is expected to be false with probability @p prob.
 *
 * @param cond  Boolean-like expression; evaluated exactly once.
 * @param prob  Anticipated probability in the range [0.0, 1.0].
 *              Only compile-time constants; avoid 0.0 or 1.0.
 *
 * Expands to compiler-specific intrinsics when available; otherwise to
 * a boolean normalisation of @p cond.
 *
 * @see mhd_COND_PRED_TRUE_P
 */


#if defined(MHD_HAVE___BUILTIN_EXPECT_WITH_PROBABILITY)
#  define mhd_COND_PRED_TRUE_P(cond,prob) \
        __builtin_expect_with_probability (! ! (cond), ! 0, (prob))
#  define mhd_COND_PRED_FALSE_P(cond,prob) \
        __builtin_expect_with_probability (! ! (cond), 0, (prob))
#elif defined(MHD_HAVE___BUILTIN_EXPECT)
#  define mhd_COND_PRED_TRUE_P(cond,prob)    __builtin_expect (! ! (cond), ! 0)
#  define mhd_COND_PRED_FALSE_P(cond,prob)   __builtin_expect (! ! (cond), 0)
#else
#  define mhd_COND_PRED_TRUE_P(cond,prob)    (! ! (cond))
#  define mhd_COND_PRED_FALSE_P(cond,prob)   (! ! (cond))
#endif

/*
 * Implementation notes:
 *  - The double negation (!!) normalises any non-zero to 1 and keeps 0 as 0,
 *    ensuring the 'expected' value matches the intrinsic's contract.
 *  - The literal '!0' expresses boolean true without implying an integer
 *    width.
 */


/*
 * Readable convenience wrappers
 */

/**
 * Hint: @p cond is true ~99.999% of the time.
 *
 * Useful for conditions whose failure indicates exceptional or defensive
 * paths that almost never triggered in normal operation.
 */
#define mhd_COND_VIRTUALLY_ALWAYS(cond) mhd_COND_PRED_TRUE_P ((cond), 0.99999)
/**
 * Hint: @p cond is true ~99% of the time.
 */
#define mhd_COND_ALMOST_ALWAYS(cond)    mhd_COND_PRED_TRUE_P ((cond), 0.99)
/**
 * Hint: @p cond is true ~95% of the time.
 */
#define mhd_COND_PREDOMINANTLY(cond)    mhd_COND_PRED_TRUE_P ((cond), 0.95)
/**
 * Hint: @p cond is true ~90% of the time.
 */
#define mhd_COND_USUALLY(cond)          mhd_COND_PRED_TRUE_P ((cond), 0.9)

/**
 * Hint: @p cond is false ~90% of the time (i.e., the true branch is rare).
 */
#define mhd_COND_RARELY(cond)           mhd_COND_PRED_FALSE_P ((cond), 0.9)
/**
 * Hint: @p cond is false ~95% of the time (i.e., the true branch is very rare).
 */
#define mhd_COND_VERY_RARELY(cond)      mhd_COND_PRED_FALSE_P ((cond), 0.95)
/**
 * Hint: @p cond is false ~99% of the time (i.e., the true branch almost never
 * triggers).
 */
#define mhd_COND_ALMOST_NEVER(cond)     mhd_COND_PRED_FALSE_P ((cond), 0.99)
/**
 * Hint: @p cond is false ~99.999% of the time.
 *
 * Useful for conditions whose truth indicates exceptional or defensive
 * paths that almost never triggered in normal operation.
 */
#define mhd_COND_HARDLY_EVER(cond)      mhd_COND_PRED_FALSE_P ((cond), 0.99999)

/** @} */ /* end of mhd2_predict group */

#endif /* ! MHD_PREDICT_H */
