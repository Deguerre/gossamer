// Copyright (c) 2023 Andrew J. Bromage.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#ifndef MACHDEPSIMD_HH
#define MACHDEPSIMD_HH


#undef GOSS_NO_SIMD

#if defined(GOSS_EXT_SSE2)
#include "MachDepSSE.hh"
#elif defined(GOSS_EXT_NEON)
#include "MachDepNeon.hh"
#else
#include "MachDepNoSimd.hh"
#endif

#endif