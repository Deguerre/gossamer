// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#ifndef MACHDEPPLATFORM_HH
#define MACHDEPPLATFORM_HH

// Identify the platform.  At the moment, we only have three
// platforms:
//
//     - Linux x86-64 (here called GOSS_LINUX_X64), GCC or Clang
//     - Mac OSX x86-64 (here called GOSS_MACOSX_X64), Clang
//     - Windows x86-64 (here called GOSS_WINDOWS_X64), MSVC

#undef GOSS_WINDOWS_X64
#undef GOSS_MACOSX_X64
#undef GOSS_LINUX_X64
#undef GOSS_CPU_X64
#undef GOSS_CPU_ARM8

#if defined(GOSS_PLATFORM_WINDOWS)

#if !defined(GOSS_COMPILER_MSVC) && !defined(GOSS_COMPILER_CLANG)
#warning "This appears to be a build on Windows with an unknown compiler. This may not work."
#endif

#define GOSS_WINDOWS_X64
#define GOSS_CPU_X64
#include "MachDepWindows.hh"

#elif defined(GOSS_PLATFORM_OSX)

#ifndef GOSS_COMPILER_CLANG
#error "This appears to be a build on OS X with a compiler other than Clang. This is an untested combination."
#endif

#if defined(__x86_64__)
#define GOSS_MACOSX_X64
#define GOSS_CPU_X64
#include "MachDepMacOSX.hh"
#elif defined(__ARM_ARCH) && __ARM_ARCH >= 8
#define GOSS_MACOSX_ARM8
#define GOSS_CPU_ARM8
#else
#error "This appears to be a build on OS X on an unknown platform."
#endif

#elif defined(GOSS_PLATFORM_UNIX)

#if defined(linux) || defined(__linux__)
#define GOSS_PLATFORM_LINUX
#if !defined(GOSS_COMPILER_GNU) && !defined(GOSS_COMPILER_CLANG)
#error "This appears to be a build on Linux with an unknown compiler."
#endif
#else
#error "This appears to be a build on an unknown Unix-like OS. This is an untested combination."
#endif

#if !defined(__x86_64__)
#error "This appears to be a build on a non-Intel Unix-like platform. This is an untested combination."
#endif

#define GOSS_LINUX_X64
#define GOSS_CPU_X64
#include "MachDepLinux.hh"

#else

#error "Can't work out what platform this is. This build will probably fail."

#endif

#ifdef GOSS_CPU_X64
#define GOSS_EXT_SSE2
#define GOSS_EXT_SSSE3
#define GOSS_EXT_AVX

namespace Gossamer {
	inline bool add64(uint64_t pA, uint64_t pB, bool pCarryIn, uint64_t& pResult)
	{
		return _addcarryx_u64(pCarryIn, pA, pB, &pResult);
	}

	inline bool sub64(uint64_t pA, uint64_t pB, bool pCarryIn, uint64_t& pResult)
	{
		return _subborrow_u64(pCarryIn, pA, pB, &pResult);
	}

	inline void cache_prefetch_l1_read(const void* addr) {
		_mm_prefetch((const char*)addr, _MM_HINT_T0);
	}

	inline void cache_prefetch_l1_write(const void* addr) {
		_mm_prefetch((const char*)addr, _MM_HINT_T0);
	}

	inline void cache_prefetch_l2_read(const void* addr) {
		_mm_prefetch((const char*)addr, _MM_HINT_T1);
	}

	inline void cache_prefetch_l2_write(const void* addr) {
		_mm_prefetch((const char*)addr, _MM_HINT_T1);
	}

	inline void cache_prefetch_l3_read(const void* addr) {
		_mm_prefetch((const char*)addr, _MM_HINT_T2);
	}

	inline void cache_prefetch_l3_write(const void* addr) {
		_mm_prefetch((const char*)addr, _MM_HINT_T2);
	}

	inline void cpu_relax() {
		_mm_pause();
	}

	inline uint64_t byte_swap_64(uint64_t x) {
#ifdef GOSS_PLATFORM_WINDOWS
		return _byteswap_uint64(x);
#else
		return _bswap64(x);
#endif
	}
}

#endif


#ifdef GOSS_CPU_ARM8
#define GOSS_EXT_NEON

// Assumes ACLE and Clang
namespace Gossamer {
	inline bool add64(uint64_t pA, uint64_t pB, bool pCarryIn, uint64_t& pResult)
	{
		uint64_t carryout;
		pResult = __builtin_addcll(pA, pB, pCarryIn, &carryout);
		return carryout;
	}

	inline bool sub64(uint64_t pA, uint64_t pB, bool pCarryIn, uint64_t& pResult)
	{
		uint64_t carryout;
		pResult = __builtin_subcll(pA, pB, pCarryIn, &carryout);
		return carryout;
	}

	inline void cache_prefetch_l1_read(const void* addr) {
		__pldx(0, 0, 0, (void const volatile*)addr);
	}

	inline void cache_prefetch_l1_write(const void* addr) {
		__pldx(1, 0, 0, (void const volatile*)addr);
	}

	inline void cache_prefetch_l2_read(const void* addr) {
		__pldx(0, 1, 0, (void const volatile*)addr);
	}

	inline void cache_prefetch_l2_write(const void* addr) {
		__pldx(1, 1, 0, (void const volatile*)addr);
	}

	inline void cache_prefetch_l3_read(const void* addr) {
		__pldx(0, 2, 0, (void const volatile*)addr);
	}

	inline void cache_prefetch_l3_write(const void* addr) {
		__pldx(1, 2, 0, (void const volatile*)addr);
	}

	inline void cpu_relax() {
		__yield();
	}

	inline uint64_t byte_swap_64(uint64_t x) {
		return vrev64_s8(x);
	}
}

#endif



#endif
