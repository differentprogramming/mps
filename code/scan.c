/* scan.c: SCANNING FUNCTIONS
 *
 * $Id$
 * Copyright (c) 2001-2020 Ravenbrook Limited.
 * See end of file for license.
 *
 * .outside: The code in this file is written as if *outside* the MPS,
 * and so is restricted to facilities in the MPS interface.  MPS users
 * are invited to read this code and use it as a basis for their own
 * scanners.  See topic "Area Scanners" in the MPS manual.
 *
 * TODO: Design document.
 */

#include "mps.h"
#include "mpstd.h" /* for MPS_BUILD_MV */


#ifdef MPS_BUILD_MV
/* MSVC warning 4127 = conditional expression is constant */
/* Objects to: MPS_SCAN_AREA(1). */
#pragma warning( disable : 4127 )
#endif

#define UNUSED_(x) (void)(x)


#define MPS_SCAN_AREA(test) \
  MPS_SCAN_BEGIN(ss) {                                  \
    mps_word_t *p = base;                               \
    while (p < (mps_word_t *)limit) {                   \
      mps_word_t word = *p;                             \
      mps_word_t tag_bits = word & mask;                \
      if (test) {                                       \
        mps_addr_t ref = (mps_addr_t)(word ^ tag_bits); \
        if (MPS_FIX1(ss, ref)) {                        \
          mps_res_t res = MPS_FIX2(ss, &ref);           \
          if (res != MPS_RES_OK)                        \
            return res;                                 \
          *p = (mps_word_t)ref | tag_bits;              \
        }                                               \
      }                                                 \
      ++p;                                              \
    }                                                   \
  } MPS_SCAN_END(ss);

#define MPS_CUSTOM_SCAN_AREA(is_nan,is_naked) \
  MPS_SCAN_BEGIN(ss) {                                  \
    mps_word_t *p = base;                               \
    while (p < (mps_word_t *)limit) {                   \
      mps_word_t tag_bits = *p;                         \
      const _Bool b_is_naked = is_naked;                \
      const _Bool b_is_nan = b_is_naked?0:(is_nan);     \
      if (b_is_nan) tag_bits = 0x7fffffffffffffffull^tag_bits;               \
      if (b_is_naked||b_is_nan) {                       \
        mps_addr_t ref = (mps_addr_t)(tag_bits);        \
        if (MPS_FIX1(ss, ref)) {                        \
          mps_res_t res = MPS_FIX2(ss, &ref);           \
          if (res != MPS_RES_OK)                        \
            return res;                                 \
          *p = b_is_naked?(mps_word_t)ref:(0x7fffffffffffffffull^(mps_word_t)ref);\
        }                                               \
      }                                                 \
      ++p;                                              \
    }                                                   \
  } MPS_SCAN_END(ss);

/* mps_custom_scam -- scan stack containing nan-boxed references and raw pointers
 *
 * Like mps_scan_area_tagged, except references whose masked bits are
 * zero are fixed in addition positive NaNs.
 * In this case the masked bits are the exponent of a 64 bit ieee float
 * as well as the bits above the 47 bits that are significant in current
 * operating systems.
 *
 * if these bits are all on, and the sign bit is off then those bits (excluding
 * sign bit) will be complemented to generate a trial address.
 *
 * Despite the extra cost in the mutator, the sign bit is flipped so that negative
 * 64 bit integers with maginitudes under 140 trillian and pairs of 32 bit values
 * where the first value is a negative integer with a magnitude under 32000
 * won't all alias to being addresses.
 * This way false positives will be much more rare.
 *
 * This scanner is most useful for ambiguously scanning the stack and
 * registers when using an optimising C compiler and nan-boxing on
 * references, since the compiler is likely to leave unboxed
 * addresses of objects around which must not be ignored.
 */

mps_res_t mps_custom_scan_nan_or_untagged(mps_ss_t ss,
                                       void *base, void *limit,
                                       void *closure)
{
    UNUSED_(closure);
    MPS_CUSTOM_SCAN_AREA((tag_bits & 0xffff800000000000ull) == 0x7fff800000000000ull, (tag_bits & 0xffff800000000000ull) == 0);

    return MPS_RES_OK;
}

/* mps_scan_area_tagged -- scan area selecting by tag
 *
 * Like mps_scan_area_masked, except only containing nan_boxed references
 *
 * In this case the masked bits are the exponent of a 64 bit ieee float
 * as well as the bits above the 47 bits that are significant in current
 * operating systems.
 *
 * if these bits are all on, and the sign bit is off then those bits (excluding
 * sign bit) will be complemented to generate a trial address.
 *
 * Despite the extra cost in the mutator, the sign bit is flipped so that negative
 * 64 bit integers with maginitudes under 140 trillian and pairs of 32 bit values
 * where the first value is a negative integer with a magnitude under 32000
 * won't all alias to being addresses.
 * This way false positives will be much more rare.
 */

mps_res_t mps_custom_scan_area_nan(mps_ss_t ss,
                               void *base, void *limit,
                               void *closure)
{
  UNUSED_(closure);
  MPS_CUSTOM_SCAN_AREA((tag_bits & 0xffff800000000000ull) == 0x7fff800000000000ull, 0);

  return MPS_RES_OK;
}

/* mps_scan_area -- scan contiguous area of references
 *
 * This is a convenience function for scanning the contiguous area
 * [base, limit).  I.e., it calls Fix on all words from base up to
 * limit, inclusive of base and exclusive of limit.
 *
 * This scanner is appropriate for use when all words in the area are
 * simple untagged references.
 */

mps_res_t mps_scan_area(mps_ss_t ss,
                        void *base, void *limit,
                        void *closure)
{
  mps_word_t mask = 0;

  (void)closure; /* unused */

  MPS_SCAN_AREA(1);

  return MPS_RES_OK;
}


/* mps_scan_area_masked -- scan area masking off tag bits
 *
 * Like mps_scan_area, but removes tag bits before fixing references,
 * and restores them afterwards.
 *
 * For example, if mask is 7, then this scanner will clear the bottom
 * three bits of each word before fixing.
 *
 * This scanner is useful when all words in the area must be treated
 * as references no matter what tag they have.
 */

mps_res_t mps_scan_area_masked(mps_ss_t ss,
                               void *base, void *limit,
                               void *closure)
{
  mps_scan_tag_t tag = closure;
  mps_word_t mask = tag->mask;

  MPS_SCAN_AREA(1);

  return MPS_RES_OK;
}


/* mps_scan_area_tagged -- scan area selecting by tag
 *
 * Like mps_scan_area_masked, except only references whose masked bits
 * match a particular tag pattern are fixed.
 *
 * For example, if mask is 7 and pattern is 5, then this scanner will
 * only fix words whose low order bits are 0b101.
 */

mps_res_t mps_scan_area_tagged(mps_ss_t ss,
                               void *base, void *limit,
                               void *closure)
{
  mps_scan_tag_t tag = closure;
  mps_word_t mask = tag->mask;
  mps_word_t pattern = tag->pattern;

  MPS_SCAN_AREA(tag_bits == pattern);

  return MPS_RES_OK;
}


/* mps_scan_area_tagged_or_zero -- scan area selecting by tag or zero
 *
 * Like mps_scan_area_tagged, except references whose masked bits are
 * zero are fixed in addition to those that match the pattern.
 *
 * For example, if mask is 7 and pattern is 3, then this scanner will
 * fix words whose low order bits are 0b011 and words whose low order
 * bits are 0b000, but not any others.
 *
 * This scanner is most useful for ambiguously scanning the stack and
 * registers when using an optimising C compiler and non-zero tags on
 * references, since the compiler is likely to leave untagged
 * addresses of objects around which must not be ignored.
 */

mps_res_t mps_scan_area_tagged_or_zero(mps_ss_t ss,
                                       void *base, void *limit,
                                       void *closure)
{
  mps_scan_tag_t tag = closure;
  mps_word_t mask = tag->mask;
  mps_word_t pattern = tag->pattern;

  MPS_SCAN_AREA(tag_bits == 0 || tag_bits == pattern);

  return MPS_RES_OK;
}


/* C. COPYRIGHT AND LICENSE
 *
 * Copyright (C) 2001-2020 Ravenbrook Limited <https://www.ravenbrook.com/>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
