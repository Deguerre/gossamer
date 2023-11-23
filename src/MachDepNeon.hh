// Copyright (c) 2023 Andrew J. Bromage.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#ifndef MACHDEPNEON_HH
#define MACHDEPNEON_HH

#include <arm_acle.h>
#include <arm_neon.h>

#define GOSS_HAVE_SIMD128

namespace Gossamer {
	namespace simd {
		typedef uint64x2_t int128;
	}
}

#endif