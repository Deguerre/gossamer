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
		typedef __m128i int128;
		inline int128 zero_128() { return _mm_set_epi64x(0, 0); }

		inline int128 load_aligned_128(const void* ptr) {
			return _mm_load_si128((const __m128i*)ptr);
		}

		inline int128 load_unaligned_128(const void* ptr) {
			return _mm_loadu_si128((const __m128i*)ptr);
		}

		inline void store_aligned_128(void* ptr, int128 x) {
			return _mm_store_si128((__m128i*)ptr, x);
		}

		inline void store_unaligned_128(void* ptr, int128 x) {
			return _mm_storeu_si128((__m128i*)ptr, x);
		}

		inline int128 not_128(int128 x) {
			return _mm_xor_si128(x, _mm_cmpeq_epi32(x, x));
		}

#ifdef GOSS_EXT_SSSE3
		inline int128 byte_reverse_128(int128 x) {
			return _mm_shuffle_epi8(x, _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
		}

#endif

#ifdef GOSS_EXT_AVX
#define GOSS_HAVE_REVERSE4_128
		inline int128 reverse4_128(int128 x) {
			const auto mask2 = _mm_set_epi8(0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33);
			const auto mask4 = _mm_set_epi8(0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F);

			auto x2 = _mm_or_si128(_mm_slli_epi16(_mm_and_si128(mask2, x), 2), _mm_srli_epi16(_mm_andnot_si128(mask2, x), 2));
			auto x4 = _mm_or_si128(_mm_slli_epi16(_mm_and_si128(mask4, x2), 4), _mm_srli_epi16(_mm_andnot_si128(mask4, x2), 4));

			return byte_reverse_128(x4);
		}
#endif

}


#endif
