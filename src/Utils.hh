// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#ifndef UTILS_HH
#define UTILS_HH

#ifndef STDINT_H
#include <stdint.h>
#define STDINT_H
#endif

#ifndef MATH_H
#include <math.h>
#define MATH_H
#endif

#ifndef STRING_H
#include <string.h>
#define STRING_H
#endif

#ifndef STD_STRING
#include <string>
#define STD_STRING
#endif

#ifndef BOOST_STATIC_ASSERT_HPP
#include <boost/static_assert.hpp>
#define BOOST_STATIC_ASSERT_HPP
#endif

#ifndef STD_MEMORY
#include <memory>
#define STD_MEMORY
#endif

#ifndef STD_ITERATOR
#include <iterator>
#define STD_ITERATOR
#endif

#ifndef STD_IOSTREAM
#include <iostream>
#define STD_IOSTREAM
#endif

#ifndef BOOST_MATH_CONSTANTS_CONSTANTS_HPP
#include <boost/math/constants/constants.hpp>
#define BOOST_MATH_CONSTANTS_CONSTANTS_HPP
#endif

#ifndef BOOST_OPERATORS_HPP
#include <boost/operators.hpp>
#define BOOST_OPERATORS_HPP
#endif

#ifndef SIGNAL_H
#include <signal.h>
#define SIGNAL_H
#endif

#ifndef STD_OPTIONAL
#include <optional>
#define STD_OPTIONAL
#endif

#ifndef STD_FUTURE
#include <future>
#define STD_FUTURE
#endif



#ifndef MACHDEPPLATFORM_HH
#include "MachDepPlatform.hh"
#endif

class MachineAutoSetup
{
public:
    MachineAutoSetup(bool pDelaySetup = false)
    {
        if (!pDelaySetup)
        {
            setup();        
        }
    }

    void operator()()
    {
        setup();
    }

private:
    static void setup();
    static void setupMachineSpecific();
};

namespace Gossamer
{
    static constexpr unsigned sPageAlignBits = 16;

    // Align up
    inline constexpr uint64_t align_up(uint64_t x, unsigned bits)
    {
        const uint64_t mask = (uint64_t(1) << bits) - 1;
        return (x + mask) & ~mask;
    }

    // Align down
    inline constexpr uint64_t align_down(uint64_t x, unsigned bits)
    {
        const uint64_t mask = (uint64_t(1) << bits) - 1;
        return x & ~mask;
    }


/// Get the number of logical processors on this machine.
uint32_t logicalProcessorCount();


inline uint32_t popcnt(uint64_t pWord)
{
    BOOST_STATIC_ASSERT(sizeof(long) == 8 || sizeof(unsigned long long) == 8);

#if defined(GOSS_WINDOWS_X64)
    return __popcnt64(pWord);
#elif defined(GOSS_LINUX_X64) || defined(GOSS_MACOSX_X64)
    if (sizeof(long) == 8)
    {
        return __builtin_popcountl(pWord);
    }
    else if (sizeof(unsigned long long) == 8)
    {
        return __builtin_popcountll(pWord);
    }
#else
#error "This platform is not yet supported"
#endif
}

inline uint64_t count_leading_zeroes(uint64_t pWord)
{
    BOOST_STATIC_ASSERT(sizeof(long) == 8 || sizeof(unsigned long long) == 8);

#if defined(GOSS_LINUX_X64) || defined(GOSS_MACOSX_X64)
    if (sizeof(long) == 8)
    {
        // Returns the number of leading 0-bits in x, starting at
        // the most significant bit position. If x is 0, the result
        // is undefined
        return pWord ? __builtin_clzl(pWord) : 64;
    }
    else if (sizeof(unsigned long long) == 8)
    {
        return pWord ? __builtin_clzll(pWord) : 64;
    }
#elif defined(GOSS_WINDOWS_X64)
    // Note that the GNUC intrinsic function will return undefined
    // values if the input is zero. So lets be sane here and return
    // 64 which should mean all zeros.
    if( pWord == 0 )
    {
        return 64;
    }
    else
    {
        unsigned long idx;
        // If a set bit is found, the bit position (zero based) of the first
        // set bit found is returned in the first parameter.
        // If no set bit is found, 0 is returned; otherwise, 1 is returned.
        _BitScanReverse64(&idx, pWord);
        // Should return the number of leading zeros.
        return (63-idx);
    }
#else
    pWord |= pWord >> 1;
    pWord |= pWord >> 2;
    pWord |= pWord >> 4;
    pWord |= pWord >> 8;
    pWord |= pWord >> 16;
    pWord |= pWord >> 32;

    return popcnt(~pWord);
#endif
}

inline uint64_t find_first_set(uint64_t pWord)
{
    BOOST_STATIC_ASSERT(sizeof(long) == 8 || sizeof(unsigned long long) == 8);

#if defined(GOSS_LINUX_X64) || defined(GOSS_MACOSX_X64)
    if (sizeof(long) == 8)
    {
        // Returns one plus the index of the least significant 1-bit
        // of x, or if x is zero, returns zero.
        return __builtin_ffsl(pWord);
    }
    else if (sizeof(unsigned long long) == 8)
    {
        // Returns one plus the index of the least significant 1-bit
        // of x, or if x is zero, returns zero.
        return __builtin_ffsll(pWord);
    }
#elif defined(GOSS_WINDOWS_X64)
 
#if 0
    // _BitScanForward64 returns the zero based
    // index of the first set bit. We want 1 
    // Note that VC++'s documentation does not
    // say whether idx will be set to zero or remain
    // unmodified when input is zero.
    if( pWord == 0 )
    {
        return 0;
    }
    else
    {
        unsigned long idx;
        _BitScanForward64(&idx, pWord);
        return idx + 1;
    }
#else
    return _tzcnt_u64(pWord) + 1;
#endif

#else
    // ffsl() is in XSB as of POSIX:2004.
    // This may need to be ported to non-compliant platforms.
    // GOSS_WINDOWS_X64 seems to work fine.
    //
    // Some platforms provide ffsll(), but at the moment
    // they all seem to be __GNUC__ platforms too.
    if (sizeof(long) == 8)
    {
        return ffsl(static_cast<long>(pWord));
    }
    else if (sizeof(long) == 4)
    {
        return pWord & ((1ull<<32)-1)
            ? ffsl(static_cast<long>(pWord))
            : ffsl(static_cast<long>(pWord >> 32));
    }
#endif
}


inline uint64_t select_by_ffs(uint64_t pWord, uint64_t pR)
{
    uint64_t bit = 0;
    for (++pR; pR > 0; --pR)
    {
        bit = pWord & -pWord;
        pWord &= ~bit;
    }
    return find_first_set(bit) - 1;
}


// The following algorithm is from Vigna, "Broadword implementation of
// rank/select queries".

const uint64_t sMSBs8 = 0x8080808080808080ull;
const uint64_t sLSBs8 = 0x0101010101010101ull;


inline uint64_t
leq_bytes(uint64_t pX, uint64_t pY)
{
    return ((((pY | sMSBs8) - (pX & ~sMSBs8)) ^ pX ^ pY) & sMSBs8) >> 7;
}


inline uint64_t
gt_zero_bytes(uint64_t pX)
{
    return ((pX | ((pX | sMSBs8) - sLSBs8)) & sMSBs8) >> 7;
}


inline uint64_t select_by_vigna(uint64_t pWord, uint64_t pR)
{
    const uint64_t sOnesStep4  = 0x1111111111111111ull;
    const uint64_t sIncrStep8  = 0x8040201008040201ull;

    uint64_t byte_sums = pWord - ((pWord & 0xA*sOnesStep4) >> 1);
    byte_sums = (byte_sums & 3*sOnesStep4) + ((byte_sums >> 2) & 3*sOnesStep4);
    byte_sums = (byte_sums + (byte_sums >> 4)) & 0xF*sLSBs8;
    byte_sums *= sLSBs8;

    const uint64_t k_step_8 = pR * sLSBs8;
    const uint64_t place
        = (leq_bytes( byte_sums, k_step_8 ) * sLSBs8 >> 53) & ~0x7;

    const int byte_rank = pR - (((byte_sums << 8) >> place) & 0xFF);

    const uint64_t spread_bits = (pWord >> place & 0xFF) * sLSBs8 & sIncrStep8;
    const uint64_t bit_sums = gt_zero_bytes(spread_bits) * sLSBs8;

    const uint64_t byte_rank_step_8 = byte_rank * sLSBs8;

    return place + (leq_bytes( bit_sums, byte_rank_step_8 ) * sLSBs8 >> 56);
}


inline uint64_t select_by_mask(uint64_t pWord, int pR)
{
    int t, i = pR, r = 0;
    const uint64_t m1 = 0x5555555555555555ULL; // even bits
    const uint64_t m2 = 0x3333333333333333ULL; // even 2-bit groups
    const uint64_t m4 = 0x0f0f0f0f0f0f0f0fULL; // even nibbles
    const uint64_t m8 = 0x00ff00ff00ff00ffULL; // even bytes
    uint64_t c1 = pWord;
    uint64_t c2 = c1 - ((c1 >> 1) & m1);
    uint64_t c4 = ((c2 >> 2) & m2) + (c2 & m2);
    uint64_t c8 = ((c4 >> 4) + c4) & m4;
    uint64_t c16 = ((c8 >> 8) + c8) & m8;
    uint64_t c32 = (c16 >> 16) + c16;
    uint64_t c64 = (((c32 >> 32) + c32) & 0x7f);
    t = (c32) & 0x3f;
    if (i >= t) { r += 32; i -= t; }
    t = (c16 >> r) & 0x1f;
    if (i >= t) { r += 16; i -= t; }
    t = (c8 >> r) & 0x0f;
    if (i >= t) { r += 8; i -= t; }
    t = (c4 >> r) & 0x07;
    if (i >= t) { r += 4; i -= t; }
    t = (c2 >> r) & 0x03;
    if (i >= t) { r += 2; i -= t; }
    t = (c1 >> r) & 0x01;
    if (i >= t) { r += 1; }
    return pR >= c64 ? -1 : r;
}


#ifdef GOSS_USE_PDEP
inline uint64_t select_by_pdep(uint64_t pWord, uint64_t pR)
{
    uint64_t bit = _pdep_u64(uint64_t(1) << pR, pWord);
    return find_first_set(bit) - 1;
}
#endif


inline uint64_t select1(uint64_t pWord, uint64_t pRank)
{
#ifdef GOSS_USE_PDEP
    return select_by_pdep(pWord, pRank);
#else
    return select_by_vigna(pWord, pRank);
#endif
}


inline uint64_t log2(uint64_t pX)
{
#if defined(GOSS_WINDOWS_X64) || defined(GOSS_LINUX_X64) || defined(GOSS_MACOSX_X64)
    return pX == 1 ? 0 : (64 - count_leading_zeroes(pX - 1));
#else
    pWord |= pWord >> 1;
    pWord |= pWord >> 2;
    pWord |= pWord >> 4;
    pWord |= pWord >> 8;
    pWord |= pWord >> 16;
    pWord |= pWord >> 32;
    return popcnt(pWord) - 1;
#endif
}


inline uint64_t roundUpToNextPowerOf2(uint64_t pX)
{
    if (pX == 0)
    {
        return 1ull;
    }
    uint64_t lowerBound = 1ull << log2(pX); 
    return (pX == lowerBound) ? pX : (lowerBound << 1);
}


// Base-4 hamming distance
inline uint32_t hammingDistanceBase4(uint64_t x, uint64_t y)
{
    static const uint64_t m = 0x5555555555555555ULL;
    uint64_t v = x ^ y;
    return popcnt((v & m) | ((v >> 1) & m));
}


// Base-4 reverse
inline uint64_t reverseBase4(uint64_t x)
{
#if 0
    static const uint64_t m2  = 0x3333333333333333ULL;
    static const uint64_t m2p = m2 << 2;
    static const uint64_t m4  = 0x0F0F0F0F0F0F0F0FULL;
    static const uint64_t m4p = m4 << 4;
    static const uint64_t m8  = 0x00FF00FF00FF00FFULL;
    static const uint64_t m8p = m8 << 8;
    static const uint64_t m16 = 0x0000FFFF0000FFFFULL;
    static const uint64_t m16p = m16 << 16;
    static const uint64_t m32 = 0x00000000FFFFFFFFULL;
    static const uint64_t m32p = m32 << 32;

    x = ((x & m2) << 2) | ((x & m2p) >> 2);
    x = ((x & m4) << 4) | ((x & m4p) >> 4);
    x = ((x & m8) << 8) | ((x & m8p) >> 8);
    x = ((x & m16) << 16) | ((x & m16p) >> 16);
    x = ((x & m32) << 32) | ((x & m32p) >> 32);
    return x;
#else
    static const uint64_t m2 = 0x3333333333333333ULL;
    static const uint64_t m2p = m2 << 2;
    static const uint64_t m4 = 0x0F0F0F0F0F0F0F0FULL;
    static const uint64_t m4p = m4 << 4;

    x = ((x & m2) << 2) | ((x & m2p) >> 2);
    x = ((x & m4) << 4) | ((x & m4p) >> 4);
    return Gossamer::byte_swap_64(x);
#endif
}


// Reverse complement
inline uint64_t reverseComplement(const uint64_t k, uint64_t x)
{
    x = reverseBase4(~x);
    return x >> (2 * (32 - k));
}


template <int X>
struct Log2
{
};

template <> struct Log2<1> { static const uint64_t value = 0; };
template <> struct Log2<2> { static const uint64_t value = 1; };
template <> struct Log2<3> { static const uint64_t value = 2; };
template <> struct Log2<4> { static const uint64_t value = 2; };
template <> struct Log2<5> { static const uint64_t value = 3; };
template <> struct Log2<6> { static const uint64_t value = 3; };
template <> struct Log2<7> { static const uint64_t value = 3; };
template <> struct Log2<8> { static const uint64_t value = 3; };
template <> struct Log2<9> { static const uint64_t value = 4; };
template <> struct Log2<10> { static const uint64_t value = 4; };
template <> struct Log2<11> { static const uint64_t value = 4; };
template <> struct Log2<12> { static const uint64_t value = 4; };
template <> struct Log2<13> { static const uint64_t value = 4; };
template <> struct Log2<14> { static const uint64_t value = 4; };
template <> struct Log2<15> { static const uint64_t value = 4; };
template <> struct Log2<16> { static const uint64_t value = 4; };
template <> struct Log2<17> { static const uint64_t value = 5; };
template <> struct Log2<18> { static const uint64_t value = 5; };
template <> struct Log2<19> { static const uint64_t value = 5; };


// Ramanujan's approximation to the logarithm of factorial.
inline double logFac(uint64_t n)
{
    if (n < 2)
    {
        return 0;
    }
    return n * log((long double)n) - n + log((long double)(n * (1 + 4 * n * (1 + 2 * n)))) / 6 + log(boost::math::constants::pi<long double>()) / 2;
}


// Approximation to the logarithm of { n \choose k }
inline double logChoose(uint64_t n, uint64_t k)
{
    return logFac(n) - logFac(k) - logFac(n - k);
}


// Binomial distribution confidence interval.
inline std::pair<double,double>
binomialConfidenceInterval(uint64_t m, uint64_t n, double z = 1.96)
{
    const double p = (double)m / n;
    const double z2 = z * z;
    const double invdenom = 0.5 / (n + z2);
    const double width = z * std::sqrt(z2 - 1.0 / n + 4 * n * p * (1 - p) + (4 * p - 1)) + 1;
    const double mid = 2 * n * p + z2;
    const double wmin = p == 0 ? 0 : std::max(0.0, (mid - width) * invdenom);
    const double wmax = p == 1 ? 1 : std::min(1.0, (mid + width) * invdenom);
    return std::pair(wmin, wmax);
}


// Base-2^64 expansion of the golden ratio, useful for hash seeds.
constexpr uint64_t sPhi0 = 0x9e3779b97f4a7c15ull;
constexpr uint64_t sPhi1 = 0xf39cc0605cedc834ull;
constexpr uint64_t sPhi2 = 0x1082276bf3a27251ull;
constexpr uint64_t sPhi3 = 0xf86c6a11d0c18e95ull;
constexpr uint64_t sPhi4 = 0x2767f0b153d27b7full;
constexpr uint64_t sPhi5 = 0x0347045b5bf1827full;
constexpr uint64_t sPhi6 = 0x01886f0928403002ull;
constexpr uint64_t sPhi7 = 0xc1d64ba40f335e36ull;


template<typename T>
inline
T clamp(T pMin, T pX, T pMax)
{
    if (pX < pMin)
    {
        return pMin;
    }
    if (pX > pMax)
    {
        return pMax;
    }
    return pX;
}


// Ensure that a container has enough capacity for some new
// elements, doing a geometric resize if not.
//
template<class Container>
inline void
ensureCapacity(Container& pContainer, uint64_t pNewElements = 1)
{
    uint64_t oldSize = pContainer.size();
    uint64_t oldCapacity = pContainer.capacity();
    uint64_t newCapacity = oldSize + pNewElements;
    if (newCapacity < oldCapacity)
    {
        newCapacity = std::min<uint64_t>(newCapacity,
                        (oldCapacity * 3 + 1) / 2);
        newCapacity = std::max<uint64_t>(newCapacity, 4);
        pContainer.reserve(newCapacity);
    }
}


// Sort-and-unique a container.
//
template<class Container>
inline void
uniqueAfterSort(Container& pContainer)
{
    pContainer.erase(std::unique(pContainer.begin(), pContainer.end()),
        pContainer.end());
}



// Sort-and-unique a container.
//
template<class Container>
inline void
sortAndUnique(Container& pContainer)
{
    std::sort(pContainer.begin(), pContainer.end());
    uniqueAfterSort(pContainer);
}


std::string defaultTmpDir(); // OS dependent


// Helper class to implement the empty member optimisation.
// See http://www.cantrip.org/emptyopt.html for details.
//
template<class Base, class Member>
struct BaseOpt : Base
{
    Member m;

    BaseOpt(const Base& pBase, const Member& pMember)
        : Base(pBase), m(pMember)
    {
    }

    BaseOpt(const Base& pBase)
        : Base(pBase)
    {
    }

    BaseOpt(const Member& pMember)
        : m(pMember)
    {
    }

    BaseOpt(const BaseOpt& pOpt)
        : Base(pOpt), m(pOpt.m)
    {
    }

    BaseOpt()
    {
    }

    void swap(BaseOpt& pOpt)
    {
        std::swap<Base>(*this, pOpt);
        std::swap(m, pOpt.m);
    }
};


// Custom version of lower_bound, optimised for pointers.
//
template<typename T, int Min = 32>
const T*
lower_bound_on_pointers(const T* pBegin, const T* pEnd, const T& pKey)
{
    ptrdiff_t len = pEnd - pBegin;

    while (len > Min)
    {
        ptrdiff_t half = len >> 1;
        const T* middle = pBegin + half;
        if (*middle < pKey)
        {
            pBegin = middle + 1;
            len -= half + 1;
        }
        else
        {
            len = half;
        }
    }

    while (len > 0 && *pBegin < pKey) {
        --len;
        ++pBegin;
    }

    return pBegin;
}


// Custom version of lower_bound, which converts from binary
// search to linear search once the gap is small enough.
//
template<typename It, typename T, typename Comp, int Min = 32>
It
tuned_lower_bound(const It& pBegin, const It& pEnd, const T& pKey, Comp pComp = Comp())
{
    auto len = std::distance(pBegin, pEnd);
    It s = pBegin;

    while (len > Min)
    {
        auto half = len >> 1;
        It m = s;
        std::advance(m, half);
        if (pComp(*m, pKey))
        {
            s = m;
            std::advance(s, 1);
            len -= half + 1;
        }
        else
        {
            len = half;
        }
    }

    while (len > 0 && pComp(*s,pKey)) {
        --len;
        ++s;
    }

    return s;
}


// Custom version of upper_bound, optimised for pointers.
//
template<typename T, int Min = 32>
const T*
upper_bound_on_pointers(const T* pBegin, const T* pEnd, const T& pKey)
{
    ptrdiff_t len = pEnd - pBegin;

    while (len > Min)
    {
        ptrdiff_t half = len >> 1;
        const T* middle = pBegin + half;
        if (pKey < *middle)
        {
            len = half;
        }
        else
        {
            pBegin = middle + 1;
            len -= half + 1;
        }
    }

    auto s = pBegin + len;
    while (len > 0) {
        --s;
        if (!(pKey < *s)) {
            ++s;
            break;
        }
        --len;
    }

    return s;
}



// Custom version of upper_bound, which converts from binary
// search to linear search once the gap is small enough.
//
template<typename It, typename T, typename Comp = std::less<T>, int Min = 32>
It
tuned_upper_bound(const It& pBegin, const It& pEnd, const T& pKey, Comp pComp = Comp())
{
    auto len = std::distance(pBegin, pEnd);
    It s = pBegin;

    while (len > Min)
    {
        auto half = len >> 1;
        It m = s;
        std::advance(m, half);
        if (pComp(pKey,*m))
        {
            len = half;
        }
        else
        {
            s = m;
            std::advance(s, 1);
            len -= half + 1;
        }
    }

    std::advance(s, len);
    while (len > 0) {
        --s;
        if (!pComp(pKey, *s)) {
            ++s;
            break;
        }
        --len;
    }

    return s;
}


// Combined upper_bound and lower_bound.
//
template<typename T, int Min = 32>
std::pair<const T*, const T*>
lower_and_upper_bound_on_pointers(const T* pBegin, const T* pEnd, const T& pLowerKey, const T& pUpperKey)
{
    std::pair<const T*, const T*> retval;
    ptrdiff_t len = pEnd - pBegin;
    const T* s = pBegin;

    while (len > Min)
    {
        auto half = len >> 1;
        const T* m = s + half;
        if (*m < pLowerKey)
        {
            s = m;
            ++s;
            len -= half + 1;
        }
        else if (pUpperKey < *m)
        {
            len = half;
        }
        else {
            auto e = s + len;
            retval.first = lower_bound_on_pointers(s, m, pLowerKey);
            retval.second = upper_bound_on_pointers(m, e, pUpperKey);
            return retval;
        }
    }

    while (len > 0 && *s < pLowerKey) {
        --len;
        ++s;
    }
    retval.first = s;

    s += len;
    while (len > 0) {
        --s;
        if (!(pUpperKey < *s)) {
            ++s;
            break;
        }
        --len;
    }
    retval.second = s;
    return retval;
}




// Combined upper_bound and lower_bound.
//
template<typename It, typename T, typename Comp = std::less<T>, int Min = 32>
void
tuned_lower_and_upper_bound(const It& pBegin, const It& pEnd, const T& pLowerKey, T& pUpperKey, It& pLower, It& pUpper, Comp pComp = Comp())
{
    auto len = std::distance(pBegin, pEnd);
    It s = pBegin;

    while (len > Min)
    {
        auto half = len >> 1;
        It m = s;
        std::advance(m, half);
        if (pComp(*m, pLowerKey))
        {
            s = m;
            std::advance(s, 1);
            len -= half + 1;
        }
        else if (pComp(pUpperKey, *m))
        {
            len = half;
        }
        else {
            auto e = s;
            std::advance(e, len);
            pLower = tuned_lower_bound<It, T, Comp, Min>(s, m, pLowerKey, pComp);
            pUpper = tuned_upper_bound<It, T, Comp, Min>(m, e, pUpperKey, pComp);
            return;
        }
    }

    while (len > 0 && pComp(*s, pLowerKey)) {
        --len;
        ++s;
    }
    pLower = s;

    std::advance(s, len);
    while (len > 0) {
        --s;
        if (!pComp(pUpperKey, *s)) {
            ++s;
            break;
        }
        --len;
    }
    pUpper = s;
}


// Unroll a loop.
//
namespace detail {
    template<class T, T... inds, class F>
    inline constexpr void unrolled_loop(std::integer_sequence<T, inds...>, F&& f) {
        (f(std::integral_constant<T, inds>{}), ...);
    }
}

template<class T, T count, class F>
inline constexpr void unrolled_loop(F&& f) {
    detail::unrolled_loop(std::make_integer_sequence<T, count>{}, std::forward<F>(f));
}


// Check if a std::future is ready.
//
template<typename T>
inline bool
future_is_ready(const std::future<T>& f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

class semaphore : private boost::noncopyable
{
private:
    std::mutex mMutex;
    std::condition_variable mCondVar;
    size_t mCount;

public:
    semaphore(size_t pCount)
        : mCount(pCount)
    {
    }

    void acquire()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mCondVar.wait(lock, [&]() { return mCount > 0; } );
        --mCount;
    }

    void release()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        ++mCount;
        mCondVar.notify_one();
    }

    bool try_acquire()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (!mCount) {
            return false;
        }

        --mCount;
        return true;
    }

#if 0
    template <typename Duration>
    bool try_acquire_for(const Duration& duration)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (!mCondVar.wait_for(lock, duration, [&]{ return mCount > 0; })) {
            return false;
        }
        --mCount;
        return true;
    }

    template <class TimePoint>
    bool try_acquire_until(const TimePoint& timePoint)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (!mCondVar.wait_until(lock, duration, [&]{ return mCount > 0; })) {
            return false;
        }
        --mCount;
        return true;
    }
#endif
};

} // namespace Gossamer

#endif // UTILS_HH
