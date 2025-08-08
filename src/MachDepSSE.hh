// Copyright (c) 2023 Andrew J. Bromage.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#ifndef MACHDEPSSE_HH
#define MACHDEPSSE_HH

#include <immintrin.h>

#define GOSS_HAVE_SIMD128

#ifdef GOSS_EXT_AVX
#include <emmintrin.h>
#endif

namespace Gossamer {
	namespace simd {
		typedef __m128i int128;
		GOSS_FORCE_INLINE int128 zero_128() { return _mm_set_epi64x(0, 0); }

		inline bool test_equal(int128 x, int128 y) {
#ifdef GOSS_EXT_SSE41
			auto vcmp = _mm_xor_si128(x, y);
			return _mm_testz_si128(vcmp, vcmp);
#else
			auto vcmp = _mm_cmpeq_epi8(x, y);
			return _mm_movemask_epi8(vcmp) == 0xFFFF;
#endif
		}

		GOSS_FORCE_INLINE int128 load_64(uint64_t x) {
			return _mm_set_epi64x(0, x);
		}

		GOSS_FORCE_INLINE int128 load_2x64_128(uint64_t x, uint64_t y) {
			return _mm_set_epi64x(x, y);
		}

		GOSS_FORCE_INLINE int128 load_aligned_128(const void* ptr) {
			return _mm_load_si128((const __m128i*)ptr);
		}

		GOSS_FORCE_INLINE int128 load_unaligned_128(const void* ptr) {
			return _mm_loadu_si128((const __m128i*)ptr);
		}

		GOSS_FORCE_INLINE void store_aligned_128(void* ptr, int128 x) {
			return _mm_store_si128((__m128i*)ptr, x);
		}

		GOSS_FORCE_INLINE void store_unaligned_128(void* ptr, int128 x) {
			return _mm_storeu_si128((__m128i*)ptr, x);
		}

		GOSS_FORCE_INLINE int128 not_128(int128 x) {
			return _mm_xor_si128(x, _mm_cmpeq_epi32(x, x));
		}

		GOSS_FORCE_INLINE int128 or_128(int128 x, int128 y) {
			return _mm_or_si128(x, y);
		}

		GOSS_FORCE_INLINE int128 and_128(int128 x, int128 y) {
			return _mm_and_si128(x, y);
		}

		GOSS_FORCE_INLINE int128 xor_128(int128 x, int128 y) {
			return _mm_xor_si128(x, y);
		}

		// x & ~y
		GOSS_FORCE_INLINE int128 andnot_128(int128 x, int128 y) {
			return _mm_andnot_si128(y, x);
		}

		GOSS_FORCE_INLINE int128 load1_8x8_128(uint8_t x) {
			return _mm_set1_epi8(x);
		}

		GOSS_FORCE_INLINE int128 compare_gt8_128(int128 x, int128 y) {
			return _mm_cmpgt_epi8(x, y);
		}

		GOSS_FORCE_INLINE uint64_t movemask_8_128(int128 x) {
			return _mm_movemask_epi8(x);
		}

#ifdef GOSS_EXT_SSSE3
		GOSS_FORCE_INLINE int128 byte_reverse_128(int128 x) {
			return _mm_shuffle_epi8(x, _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
		}

#endif

#ifdef GOSS_EXT_AVX
#define GOSS_HAVE_REVERSE4_128

		GOSS_FORCE_INLINE int128 reverse4_128_by_bitshift(int128 x) {
			const auto mask2 = _mm_set_epi8(0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33);
			const auto mask4 = _mm_set_epi8(0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F);

			auto x2 = _mm_or_si128(_mm_slli_epi16(_mm_and_si128(mask2, x), 2), _mm_srli_epi16(_mm_andnot_si128(mask2, x), 2));
			auto x4 = _mm_or_si128(_mm_slli_epi16(_mm_and_si128(mask4, x2), 4), _mm_srli_epi16(_mm_andnot_si128(mask4, x2), 4));

			return byte_reverse_128(x4);
		}

		GOSS_FORCE_INLINE int128 reverse4_128_by_shuffle(int128 x) {
			__m128i AND_MASK = _mm_set_epi32(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f);
			__m128i LO_MASK = _mm_set_epi32(0x0F0B0703, 0x0E0A0602, 0x0D090501, 0x0C080400);
			__m128i HI_MASK = _mm_set_epi32(0xF0B07030, 0xE0A06020, 0xD0905010, 0xC0804000);
			auto tmp2 = _mm_srli_epi16(x, 4);
			auto tmp1 = _mm_and_si128(x, AND_MASK);
			tmp2 = _mm_and_si128(tmp2, AND_MASK);
			tmp1 = _mm_shuffle_epi8(HI_MASK, tmp1);
			tmp2 = _mm_shuffle_epi8(LO_MASK, tmp2);
			tmp1 = _mm_xor_si128(tmp1, tmp2);
			return byte_reverse_128(tmp1);
		}
		
		// TODO(ajb): reverse4_128 also has an efficient implementation using GFNI, but I don't have one of those handy to do timings.

		GOSS_FORCE_INLINE int128 reverse4_128(int128 x) {
			return reverse4_128_by_shuffle(x);
		}
#endif
	}
}


#endif
