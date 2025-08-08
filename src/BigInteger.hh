// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
/** \file
 * A restricted implementation of unsigned big integers.
 *
 * \note if the maximum BigInteger is MAX_BIG_INTEGER then by definition
 *  -# MAX_BIG_INTEGER + 1 == 0
 *  -# MAX_BIG_INTEGER == 0 - 1
 *
 */

#ifndef BIGINTEGER_HH
#define BIGINTEGER_HH

#ifndef STDINT_H
#include <stdint.h>
#define STDINT_H
#endif // STDINT_H

#ifndef STD_CSTRING
#include <cstring>
#define STD_CSTRING
#endif // STD_CSTRING

#ifndef STD_LIMITS
#include <limits>
#define STD_LIMITS
#endif // STD_LIMITS

#ifndef STD_IOSTREAM
#include <iostream>
#define STD_IOSTREAM
#endif // STD_IOSTREAM

#ifndef STD_UNORDERED_SET
#include <unordered_set>
#define STD_UNORDERED_SET
#endif

#ifndef UTILS_HH
#include "Utils.hh"
#endif // UTILS_HH

#ifndef NUMERIC_CONVERSION_UTIL_HH
#include "NumericConversionUtil.hh"
#endif // NUMERIC_CONVERSION_UTIL_HH

#ifndef MACHDEPSIMD_HH
#include "MachDepSimd.hh"
#endif // MACHDEPSIMD_HH


/**
 * A brief description of the class goes here.
 *
 * The number of words for the representation is either the minimal number
 * of machine words required to represent Bits, or the number required to
 * store 64 bits, whichever is larger.  This ensures that we can always
 * place a uint64_t in the storage space without overflowing the bounds.
 * It is okay if Bits < 64, because extra bits will be masked off.
 */

struct BigIntegerBase
{
    typedef uint64_t word_type;
#ifdef GOSS_HAVE_SIMD128
    typedef Gossamer::simd::int128 double_word_type;
#endif

    typedef std::numeric_limits<word_type> word_type_limits;

    BOOST_STATIC_ASSERT(word_type_limits::is_specialized
        && !word_type_limits::is_signed
        && word_type_limits::radix == 2);

    static const uint64_t sBitsPerWord = word_type_limits::digits;

    static void writeDecimal(std::ostream& pIO, word_type* pWords,
                             uint64_t pNumWords);

    static void readDecimal(std::istream& pIO, word_type* pWords,
                            uint64_t pNumWords);
};


template<int Words>
struct BigIntegerRepresentation
{
    static constexpr bool sHaveSingleWord = false;
    union alignas(16) {
        BigIntegerBase::word_type mWords[Words];
    };

};


template<>
struct BigIntegerRepresentation<1>
{
    static constexpr bool sHaveSingleWord = true;
    union alignas(8) {
        BigIntegerBase::word_type mWords[1];
        BigIntegerBase::word_type mSingleWord;
    };
};



#ifdef GOSS_HAVE_SIMD128
template<>
struct BigIntegerRepresentation<2>
{
    static constexpr bool sHaveSingleWord = true;
    union alignas(16) {
        BigIntegerBase::word_type mWords[2];
        BigIntegerBase::double_word_type mSingleWord;
    };
};
#endif



/**
 * Unsigned big integer support.
 *
 * Only the operations essential for supporting positions in large bitmaps are supported.
 * 
 *  \param Words The number of 64-bit words.
 */
template<int Words>
class BigInteger
    : public BigIntegerBase
{
private:
    static_assert(Words > 0);
    static_assert(Words == 1 || Words % 2 == 0);

public:
    typedef BigIntegerRepresentation<Words> representation_type;

    static constexpr uint64_t sWords = Words;
#ifdef GOSS_HAVE_SIMD128
    static constexpr uint64_t sDWords = Words/2;
#endif
    static constexpr uint64_t sBits = sBitsPerWord * sWords;

    BigInteger()
    {
        clear();
    }

    explicit BigInteger(uint64_t pRhs)
    {
        clear();
        mImpl.mWords[0] = static_cast<word_type>(pRhs);
    }

    explicit BigInteger(representation_type pRhs)
        : mImpl(pRhs)
    {
    }

    bool boolean_test() const
    {
        word_type test(0);
#pragma unroll
        for (int i = 0; i < Words; ++i) {
            test |= mImpl.mWords[i];
        }
        return test != 0;
    }

    bool fitsIn64Bits() const
    {
        if constexpr (sWords == 1) {
            return true;
        }
        else if constexpr (sWords == 2) {
            return !mImpl.mWords[1];
        }
        else {
            word_type test(0);
#pragma unroll
            for (int i = 1; i < Words; ++i)
            {
                test |= mImpl.mWords[i];
            }
            return !test;
        }
    }

    uint64_t asUInt64() const
    {
        return mImpl.mWords[0];
    }

    uint64_t mostSigWord() const
    {
        return mImpl.mWords[Words - 1];
    }

    double asDouble() const
    {
        double scale = static_cast<double>(std::numeric_limits<word_type>::max()) + 1;
        double value = 0;
#pragma unroll
        for (int i = Words - 1; i >= 0; --i) {
            value = value * scale + mImpl.mWords[i];
        }
        return value;
    }

    /// Reverse two-bit sequences.
    void reverse()
    {
#pragma unroll
        for (int i = 0; i < Words / 2; ++i) {
            word_type tmp = Gossamer::reverseBase4(mImpl.mWords[i]);
            mImpl.mWords[i] = Gossamer::reverseBase4(mImpl.mWords[Words - i - 1]);
            mImpl.mWords[Words - i - 1] = tmp;
        }
    }

    /// Reverse complement.
    void reverseComplement(uint64_t pK)
    {
#ifdef GOSS_HAVE_REVERSE4_128
        if constexpr (Words == 2) {
            using namespace Gossamer::simd;
            auto x = load_aligned_128(&mImpl.mWords[0]);
            auto rc = reverse4_128(not_128(x));
            store_aligned_128(&mImpl.mWords[0], rc);
            *this >>= sBits - 2 * pK;
            return;
        }
#endif

#pragma unroll
        for (int i = 0; i < Words / 2; ++i) {
            word_type tmp = Gossamer::reverseBase4(~mImpl.mWords[i]);
            mImpl.mWords[i] = Gossamer::reverseBase4(~mImpl.mWords[Words - i - 1]);
            mImpl.mWords[Words - i - 1] = tmp;
        }
        if constexpr (Words & 1)
        {
            mImpl.mWords[Words/2] = Gossamer::reverseBase4(~mImpl.mWords[Words/2]);
        }
        *this >>= sBits - 2*pK;
    }


    /// True if the k-mer is canonical by Wittler's method
    bool wittlerCanonical(uint64_t pK)
    {
        BigInteger y(*this);
        y.reverseComplement(pK);

        bool found = false;
        bool xIsReverseComplement = true;
#pragma unroll
        for (int i = Words - 1; i >= 0; --i) {
            auto wx = mImpl.mWords[i];
            auto wy = y.mImpl.mWords[i];
            auto w = wx ^ wy;
            w |= (w & 0x5555555555555555ull) << 1;
            auto bitpos = BigInteger::sBitsPerWord - Gossamer::count_leading_zeroes(w) - 2;
            auto mask = word_type(0x3) << bitpos;
            bool xNtIsLess = (wx & mask) < (wy & mask);
            if (w && !found) {
                found = true;
                xIsReverseComplement = xNtIsLess;
            }
        }
        return xIsReverseComplement;
    }


    /// Canonicalise a kmer by Wittler's method
    void wittlerCanonicalise(uint64_t pK)
    {
        BigInteger y(*this);
        y.reverseComplement(pK);

        bool found = false;
        bool xIsReverseComplement = true;
#pragma unroll
        for (int i = Words - 1; i >= 0; --i) {
            auto wx = mImpl.mWords[i];
            auto wy = y.mImpl.mWords[i];
            auto w = wx ^ wy;
            w |= (w & 0x5555555555555555ull) << 1;
            auto bitpos = BigInteger::sBitsPerWord - Gossamer::count_leading_zeroes(w) - 2;
            auto mask = word_type(0x3) << bitpos;
            bool xNtIsLess = (wx & mask) < (wy & mask);
            if (w && !found) {
                found = true;
                xIsReverseComplement = xNtIsLess;
            }
        }
        if (!xIsReverseComplement) {
            *this = y;
        }
    }


    /// Encode a canonical k-mer using Wittler's method
    BigInteger wittlerEncode(uint64_t pK)
    {
        BigInteger code;

        BigInteger y(*this);
        y.reverseComplement(pK);

        uint64_t i = 0;
        for (; i < pK/2; ++i) {
            auto bit = (pK - i - 1) * 2;
            auto w = bit / BigInteger::sBitsPerWord;
            auto b = bit % BigInteger::sBitsPerWord;
            auto l = (mImpl.mWords[w] >> b) & 3;
            auto r = (y.mImpl.mWords[w] >> b) & 3;
            if (l < r) {
                break;
            }

            {
                auto w = (i * 2) / BigInteger::sBitsPerWord;
                auto b = (i * 2) % BigInteger::sBitsPerWord;
                code.mImpl.mWords[w] |= uint64_t(r) << b;
            }
        }

        if (2 * i + 1 == pK) {
            auto bit = (pK - i - 1) * 2;
            auto w = bit / BigInteger::sBitsPerWord;
            auto b = bit % BigInteger::sBitsPerWord;
            auto r = (y.mImpl.mWords[w] >> b) & 3;
            code.mImpl.mWords[w] |= uint64_t(r) << b;
        }
        else if (i < pK / 2) {
            auto bit = (pK - i - 1) * 2;
            auto w = bit / BigInteger::sBitsPerWord;
            auto b = bit % BigInteger::sBitsPerWord;
            auto l = (mImpl.mWords[w] >> b) & 3;
            auto r = (y.mImpl.mWords[w] >> b) & 3;

            auto rank = (5 - l) * l / 2 + r - 1;
            auto rank_l = rank / 4 + 1;
            auto rank_r = rank % 4;
            code.mImpl.mWords[w] |= uint64_t(rank_l) << b;

            {
                auto bit = (pK - i - 2) * 2;
                auto w = bit / BigInteger::sBitsPerWord;
                auto b = bit % BigInteger::sBitsPerWord;
                code.mImpl.mWords[w] |= uint64_t(rank_r) << b;
            }

            BigInteger remainder(*this);
            remainder >>= 2;
            for (auto r = i + 1; r < pK - i - 1; ++r) {
                auto bit = (pK - r - 2) * 2;
                auto w = bit / BigInteger::sBitsPerWord;
                auto b = bit % BigInteger::sBitsPerWord;
                auto x = remainder.mImpl.mWords[w] & (uint64_t(3) << b);
                code.mImpl.mWords[w] |= x;
            }
        }

//        std::cerr << "Pre-adjust code: " << code.asUInt64() << '\n';

        auto k1 = pK - i - 1;
        auto k2 = (pK + 1) / 2;
        if (k1 > k2) {
            BigInteger missing_1(1);
            missing_1 <<= k1 * 2;
            BigInteger missing_2(1);
            missing_2 <<= k2 * 2;
            missing_1 -= missing_2;
            missing_1 >>= 1;
            code -= missing_1;
        }

//        std::cerr << "Pre-midadjust code: " << code.asUInt64() << '\n';

        if (pK % 2 == 1) {
            BigInteger midadjust(1);
            midadjust <<= ((pK + 1) & ~uint64_t(1)) - 1;
            code -= midadjust;
        }
//        std::cerr << "Post-adjust code: " << code.asUInt64() << '\n';

        return code;
    }

#if 0
    /// Decode a canonical k-mer using Wittler's method
    BigInteger wittlerDecode(uint64_t pK)
    {
        BigInteger kmer;
        BigInteger decode(*this);
        std::cerr << "Pre-midadjust decode: " << decode.asUInt64() << '\n';
        if (pK % 2 == 1) {
            BigInteger midadjust(1);
            midadjust <<= ((pK + 1) & ~uint64_t(1)) - 1;
            decode += midadjust;
        }
        std::cerr << "Post-midadjust decode: " << decode.asUInt64() << '\n';

        uint64_t i = 0;
        unsigned rankl;
        bool found_i = false;
        do {
            auto bitl = (pK - i - 1) * 2;
            auto wl = bitl / BigInteger::sBitsPerWord;
            auto bl = bitl % BigInteger::sBitsPerWord;
            rankl = (decode.mImpl.mWords[wl] >> bl) & 3;
            if (rankl) {
                found_i = true;
                break;
            }
            auto bitr = i * 2;
            auto wr = bitr / BigInteger::sBitsPerWord;
            auto br = bitr % BigInteger::sBitsPerWord;
            auto symr = (decode.mImpl.mWords[wr] >> br) & 3;
            auto syml = (~uint64_t(symr)) & 3;
            kmer.mImpl.mWords[wr] |= (syml << br);
            kmer.mImpl.mWords[wl] |= (symr << bl);
            ++i;
        }  while (i < pK / 2);

        auto k1 = pK - i - 1;
        auto k2 = (pK + 1) / 2;
        if (k1 > k2) {
            BigInteger missing_1(1);
            missing_1 <<= k1 * 2;
            BigInteger missing_2(1);
            missing_2 <<= k2 * 2;
            missing_1 -= missing_2;
            missing_1 >>= 1;
            decode += missing_1;
        }
        std::cerr << "Post-adjust decode: " << decode.asUInt64() << '\n';

        if (found_i) {
            auto bitr = (pK - i - 2) * 2;
            auto wr = bitr / BigInteger::sBitsPerWord;
            auto br = bitr % BigInteger::sBitsPerWord;
            auto rankr = (decode.mImpl.mWords[wr] >> br) & 3;
            auto rank = (rankl - 1) * 4 + rankr;
            uint64_t syml = 0, symr = 0;
            switch (rank) {
            case 0: syml = 0; symr = 2; break;
            case 1: syml = 0; symr = 1; break;
            case 2: syml = 0; symr = 0; break;
            case 3: syml = 1; symr = 1; break;
            case 4: syml = 1; symr = 0; break;
            case 5: syml = 2; symr = 0; break;
            }
            {
                auto bitl = (pK - i - 1) * 2;
                auto wl = bitl / BigInteger::sBitsPerWord;
                auto bl = bitl % BigInteger::sBitsPerWord;
                kmer.mImpl.mWords[wl] |= syml << bl;
                auto bitr = i * 2;
                auto wr = bitr / BigInteger::sBitsPerWord;
                auto br = bitr % BigInteger::sBitsPerWord;
                kmer.mImpl.mWords[wr] |= symr << br;
            }

            for (auto j = i + 1; j < pK - i - 1; ++j) {
                auto bitf = (pK - j - 2) * 2;
                auto wf = bitf / BigInteger::sBitsPerWord;
                auto bf = bitf % BigInteger::sBitsPerWord;
                auto bitt = (pK - j - 1) * 2;
                auto wt = bitt / BigInteger::sBitsPerWord;
                auto bt = bitt % BigInteger::sBitsPerWord;
                auto sym = (decode.mImpl.mWords[wf] >> bf) & 3;
                kmer.mImpl.mWords[wt] |= (sym << bt);
            }
            std::cerr << decode.asUInt64() << ' ' << i << ' ' << kmer.asUInt64() << '\n';
        }
        else if (2*i + 1 == pK) {
            // Middle symbol
            auto bit = i * 2;
            auto w = bit / BigInteger::sBitsPerWord;
            auto b = bit % BigInteger::sBitsPerWord;
            kmer.mImpl.mWords[w] |= ~decode.mImpl.mWords[w] & (3 << b);
            std::cerr << decode.asUInt64() << ' ' << i << ' ' << "mid" << '\n';
        }
        else {
            std::cerr << decode.asUInt64() << ' ' << i << ' ' << "other" << '\n';
        }

#if 0

        BigInteger y(*this);
        y.reverseComplement(pK);

        uint64_t i = 0;
        for (; i < pK / 2; ++i) {
            auto bit = (pK - i - 1) * 2;
            auto w = bit / BigInteger::sBitsPerWord;
            auto b = bit % BigInteger::sBitsPerWord;
            auto l = (mImpl.mWords[w] >> b) & 3;
            auto r = (y.mImpl.mWords[w] >> b) & 3;
            if (l < r) {
                break;
            }

            {
                auto w = (i * 2) / BigInteger::sBitsPerWord;
                auto b = (i * 2) % BigInteger::sBitsPerWord;
                code.mImpl.mWords[w] |= uint64_t(r) << b;
            }
        }

        if (2 * i + 1 == pK) {
            auto bit = (pK - i - 1) * 2;
            auto w = bit / BigInteger::sBitsPerWord;
            auto b = bit % BigInteger::sBitsPerWord;
            auto r = (y.mImpl.mWords[w] >> b) & 3;
            code.mImpl.mWords[w] |= uint64_t(r) << b;
        }
        else if (i < pK / 2) {
            auto bit = (pK - i - 1) * 2;
            auto w = bit / BigInteger::sBitsPerWord;
            auto b = bit % BigInteger::sBitsPerWord;
            auto l = (mImpl.mWords[w] >> b) & 3;
            auto r = (y.mImpl.mWords[w] >> b) & 3;

            auto rank = (5 - l) * l / 2 + r - 1;
            auto rank_l = rank / 4 + 1;
            auto rank_r = rank % 4;
            code.mImpl.mWords[w] |= uint64_t(rank_l) << b;

            {
                auto bit = (pK - i - 2) * 2;
                auto w = bit / BigInteger::sBitsPerWord;
                auto b = bit % BigInteger::sBitsPerWord;
                code.mImpl.mWords[w] |= uint64_t(rank_r) << b;
            }

            BigInteger remainder(*this);
            remainder >>= 2;
            for (auto r = i + 1; r < pK - i - 1; ++r) {
                auto bit = (pK - r - 2) * 2;
                auto w = bit / BigInteger::sBitsPerWord;
                auto b = bit % BigInteger::sBitsPerWord;
                auto x = remainder.mImpl.mWords[w] & (uint64_t(3) << b);
                code.mImpl.mWords[w] |= x;
            }
        }

#endif
        return kmer;
    }
#endif

    BigInteger& operator+=(uint64_t pRhs)
    {
        bool carry = Gossamer::add64(mImpl.mWords[0], pRhs, false, mImpl.mWords[0]);
#pragma unroll
        for (int i = 1; i < Words; ++i) {
            carry = Gossamer::add64(mImpl.mWords[i], 0, carry, mImpl.mWords[i]);
        }
        return *this;
    }

    BigInteger& operator+=(const BigInteger& pRhs)
    {
        bool carry = Gossamer::add64(mImpl.mWords[0], pRhs.mImpl.mWords[0], false, mImpl.mWords[0]);
#pragma unroll
        for (int i = 1; i < Words; ++i) {
            carry = Gossamer::add64(mImpl.mWords[i], pRhs.mImpl.mWords[i], carry, mImpl.mWords[i]);
        }
        return *this;
    }

    // Optimised add-plus-1.
    BigInteger& add1(uint64_t pRhs)
    {
        bool carry = Gossamer::add64(mImpl.mWords[0], pRhs, true, mImpl.mWords[0]);
#pragma unroll
        for (int i = 1; i < Words; ++i) {
            carry = Gossamer::add64(mImpl.mWords[i], 0, carry, mImpl.mWords[i]);
        }
        return *this;
    }

    // Optimised add-plus-1.
    BigInteger& add1(const BigInteger& pRhs)
    {
        bool carry = Gossamer::add64(mImpl.mWords[0], pRhs.mImpl.mWords[0], true, mImpl.mWords[0]);
#pragma unroll
        for (int i = 1; i < Words; ++i) {
            carry = Gossamer::add64(mImpl.mWords[i], pRhs.mImpl.mWords[i], carry, mImpl.mWords[i]);
        }
        return *this;
    }

    BigInteger& operator-=(uint64_t pRhs)
    {
        bool carry = Gossamer::sub64(mImpl.mWords[0], pRhs, false, mImpl.mWords[0]);
#pragma unroll
        for (int i = 1; i < Words; ++i) {
            carry = Gossamer::sub64(mImpl.mWords[i], 0, carry, mImpl.mWords[i]);
        }
        return *this;
    }

    BigInteger& operator-=(const BigInteger& pRhs)
    {
        bool carry = Gossamer::sub64(mImpl.mWords[0], pRhs.mImpl.mWords[0], false, mImpl.mWords[0]);
#pragma unroll
        for (int i = 1; i < Words; ++i) {
            carry = Gossamer::sub64(mImpl.mWords[i], pRhs.mImpl.mWords[i], carry, mImpl.mWords[i]);
        }
        return *this;
    }

    // Optimised subtract-minus-1.
    BigInteger& subtract1(const BigInteger& pRhs)
    {
        bool carry = Gossamer::sub64(mImpl.mWords[0], pRhs.mImpl.mWords[0], true, mImpl.mWords[0]);
#pragma unroll
        for (int i = 1; i < Words; ++i) {
            carry = Gossamer::sub64(mImpl.mWords[i], pRhs.mImpl.mWords[i], carry, mImpl.mWords[i]);
        }
        return *this;
    }

    BigInteger& operator<<=(uint64_t pShift)
    {
        if (pShift >= sBits)
        {
            clear();
            return *this;
        }

        if constexpr (Words == 1) {
            mImpl.mWords[0] <<= pShift;
            return *this;
        }

        if constexpr (Words == 2) {
            auto x1 = mImpl.mWords[1];
            auto x0 = mImpl.mWords[0];
            if (pShift >= sBitsPerWord) {
                x1 = x0;
                x0 = 0;
            }
            auto shift = pShift & (sBitsPerWord-1);
            mImpl.mWords[1] = x1 << shift | ((shift ? x0 : 0) >> (sBitsPerWord - shift));
            mImpl.mWords[0] = x0 << shift;
            return *this;
        }

        int64_t wordShift = pShift / sBitsPerWord;
#pragma unroll
        for (int i = Words - 1; i >= 0; --i) {
            auto sw = mImpl.mWords[i - wordShift];
            mImpl.mWords[i] = i >= wordShift ? sw : word_type(0);
        }
        pShift -= wordShift * sBitsPerWord;

        if (pShift > 0) {
            word_type carry = 0;
#pragma unroll
            for (int i = 0; i < Words; ++i) {
                word_type nextCarry = mImpl.mWords[i] >> (sBitsPerWord - pShift);
                mImpl.mWords[i] = (mImpl.mWords[i] << pShift) | carry;
                carry = nextCarry;
            }
        }

        return *this;
    }

    BigInteger& operator>>=(uint64_t pShift)
    {
        if (pShift >= sBits)
        {
            clear();
            return *this;
        }

        if constexpr (Words == 1) {
            mImpl.mWords[0] >>= pShift;
            return *this;
        }

        if constexpr (Words == 2) {
            auto x1 = mImpl.mWords[1];
            auto x0 = mImpl.mWords[0];
            if (pShift >= sBitsPerWord) {
                x0 = x1;
                x1 = 0;
            }
            auto shift = pShift & (sBitsPerWord-1);
            mImpl.mWords[1] = x1 >> shift;
            mImpl.mWords[0] = x0 >> shift | ((shift ? x1 : 0) << (sBitsPerWord - shift));
            return *this;
        }

        int64_t wordShift = pShift / sBitsPerWord;
#pragma unroll
        for (int i = 0; i < Words; ++i) {
            auto sw = mImpl.mWords[i + wordShift];
            mImpl.mWords[i] = i + wordShift < Words ? sw : word_type(0);
        }
        pShift -= wordShift * sBitsPerWord;

        if (pShift > 0)
        {
            word_type carry = 0;
#pragma unroll
            for (int i = Words - 1; i >= 0; --i) {
                word_type nextCarry = mImpl.mWords[i] << (sBitsPerWord - pShift);
                mImpl.mWords[i] = (mImpl.mWords[i] >> pShift) | carry;
                carry = nextCarry;
            }
        }

        return *this;
    }

    BigInteger& operator|=(uint64_t pRhs)
    {
        mImpl.mWords[0] |= word_type(pRhs);
        return *this;
    }

    BigInteger& operator|=(const BigInteger& pRhs)
    {
#ifdef GOSS_HAVE_SIMD128
        if constexpr (Words == 2) {
            using namespace Gossamer::simd;
            auto l = load_aligned_128(&mImpl.mWords[0]);
            auto r = load_aligned_128(&pRhs.mImpl.mWords[0]);
            store_aligned_128(&mImpl.mWords[0], or_128(l, r));
            return *this;
        }
#endif

#pragma unroll
        for (int i = 0; i < Words; ++i) {
            mImpl.mWords[i] |= pRhs.mImpl.mWords[i];
        }
        return *this;
    }

    BigInteger& operator&=(uint64_t pRhs)
    {
        uint64_t w = asUInt64() & pRhs;
        clear();
        mImpl.mWords[0] = word_type(w);
        return *this;
    }

    uint64_t operator&(uint64_t pRhs) const
    {
        return asUInt64() & pRhs;
    }

    BigInteger& operator&=(const BigInteger& pRhs)
    {
#ifdef GOSS_HAVE_SIMD128
        if constexpr (Words == 2) {
            using namespace Gossamer::simd;
            auto l = load_aligned_128(&mImpl.mWords[0]);
            auto r = load_aligned_128(&pRhs.mImpl.mWords[0]);
            store_aligned_128(&mImpl.mWords[0], and_128(l, r));
            return *this;
        }
#endif
#pragma unroll
        for (int i = 0; i < Words; ++i) {
            mImpl.mWords[i] &= pRhs.mImpl.mWords[i];
        }
        return *this;
    }

    BigInteger& operator^=(const BigInteger& pRhs)
    {
#ifdef GOSS_HAVE_SIMD128
        if constexpr (Words == 2) {
            using namespace Gossamer::simd;
            auto l = load_aligned_128(&mImpl.mWords[0]);
            auto r = load_aligned_128(&pRhs.mImpl.mWords[0]);
            store_aligned_128(&mImpl.mWords[0], xor_128(l, r));
            return *this;
        }
#endif
#pragma unroll
        for (int i = 0; i < Words; ++i) {
            mImpl.mWords[i] ^= pRhs.mImpl.mWords[i];
        }
        return *this;
    }

    BigInteger& operator++()
    {
        bool carry = Gossamer::add64(mImpl.mWords[0], 0, true, mImpl.mWords[0]);
#pragma unroll
        for (int i = 1; i < Words; ++i) {
            carry = Gossamer::add64(mImpl.mWords[i], 0, carry, mImpl.mWords[i]);
        }
        return *this;
    }

    BigInteger& operator--()
    {
        bool carry = Gossamer::sub64(mImpl.mWords[0], 0, true, mImpl.mWords[0]);
#pragma unroll
        for (int i = 1; i < Words; ++i) {
            carry = Gossamer::sub64(mImpl.mWords[i], 0, carry, mImpl.mWords[i]);
        }
        return *this;
    }

    uint64_t log2() const
    {
        // XXX TODO SIMD implementation
#pragma unroll
        for (int i = Words - 1; i >= 0; --i)
        {
            if (mImpl.mWords[i])
            {
                return Gossamer::log2(mImpl.mWords[i]) + i * sBitsPerWord;
            }
        }
        return Gossamer::log2(0);
    }

    bool operator==(const BigInteger& pRhs) const
    {
#ifdef GOSS_HAVE_SIMD128
        if constexpr (Words == 2) {
            using namespace Gossamer::simd;
            auto x = load_aligned_128(&mImpl.mWords[0]);
            auto y = load_aligned_128(&pRhs.mImpl.mWords[0]);
            return test_equal(x, y);
        }
#endif

        bool equal = mImpl.mWords[0] == pRhs.mImpl.mWords[0];
#pragma unroll
        for (int i = 1; i < Words; ++i) {
            equal = mImpl.mWords[i] == pRhs.mImpl.mWords[i] && equal;
        }

        return equal;
    }

    bool operator!=(const BigInteger& pRhs) const
    {
        return !(*this == pRhs);
    }

    bool nonZero() const
    {
        word_type w = mImpl.mWords[0];
#pragma unroll
        for (int i = 1; i < Words; ++i) {
            w |= mImpl.mWords[i];
        }
        return w != 0;
    }

    bool operator<(const BigInteger& pRhs) const
    {
        if constexpr (Words == 1) {
            return mImpl.mWords[0] < pRhs.mImpl.mWords[0];
        }
        else if constexpr (Words == 2) {
            bool w1eq = mImpl.mWords[1] == pRhs.mImpl.mWords[1];
            bool w1lt = mImpl.mWords[1] < pRhs.mImpl.mWords[1];
            bool w0lt = mImpl.mWords[0] < pRhs.mImpl.mWords[0];
            return w1lt || (w1eq && w0lt);
        }
        else {
#pragma unroll
            for (int64_t i = Words - 1; i >= 0; --i)
            {
                if (mImpl.mWords[i] == pRhs.mImpl.mWords[i]) {
                    continue;
                }
                return mImpl.mWords[i] < pRhs.mImpl.mWords[i];
            }
            return false;
        }
    }

    static bool equalWithMask(const BigInteger& lhs, const BigInteger& rhs, const BigInteger& mask)
    {
        bool ok = true;
#pragma unroll
        for (int i = 0; i < Words; ++i) {
            ok = (lhs.mImpl.mWords[i] & mask.mImpl.mWords[i]) == (rhs.mImpl.mWords[i] & mask.mImpl.mWords[i]) && ok;
        }
        return ok;
    }

    static bool testAgainstMask(const BigInteger& lhs, const BigInteger& mask)
    {
        bool ok = false;
#pragma unroll
        for (int i = 0; i < Words; ++i) {
            ok = (lhs.mImpl.mWords[i] & mask.mImpl.mWords[i]) != 0 || ok;
        }
        return ok;
    }

    uint64_t hash() const
    {
        uint64_t hash = Gossamer::detail::sPhi7;

#pragma unroll
        for (int i = 0; i < Words; ++i) {
            hash = Gossamer::hash_word(hash, mImpl.mWords[i]);
        }
        return hash;
    }

    static std::pair<uint64_t, uint64_t> hash2(const BigInteger& lhs, const BigInteger& rhs)
    {
        uint64_t hash0 = Gossamer::detail::sPhi7;
        uint64_t hash1 = Gossamer::detail::sPhi7;

#pragma unroll
        for (int i = 0; i < Words; ++i) {
            hash0 = Gossamer::hash_word(hash0, lhs.mImpl.mWords[i]);
            hash1 = Gossamer::hash_word(hash1, rhs.mImpl.mWords[i]);
        }
        return std::pair<uint64_t, uint64_t>(hash0, hash1);
    }

    std::pair<const uint64_t*,const uint64_t*> words() const
    {
        return std::pair<const uint64_t*,const uint64_t*>(mImpl.mWords, mImpl.mWords + Words);
    }
    
    std::pair<uint64_t*,uint64_t*> words()
    {
        return std::pair<uint64_t*,uint64_t*>(mImpl.mWords, mImpl.mWords + Words);
    }
    
    friend std::ostream&
    operator<<(std::ostream& pStream, const BigInteger& pRhs)
    {
        BigInteger toWrite(pRhs);
        toWrite.writeDecimal(pStream, toWrite.mImpl.mWords, BigInteger::sWords - 1);
        return pStream;
    }

#if 0
    friend class std::istream&
    operator>>(std::istream& pStream, BigInteger& pValue)
    {
        return pStream;
    }
#endif

private:
    representation_type mImpl;    ///< Storage.

    void clear()
    {
        std::memset(&mImpl, 0, sizeof(mImpl));
    }
};


template<int Words>
inline BigInteger<Words>
operator~(BigInteger<Words> pRhs)
{
#ifdef GOSS_HAVE_SIMD128
    if constexpr (Words == 2) {
        using namespace Gossamer::simd;
        auto x = load_aligned_128(&pRhs.words().first[0]);
        store_aligned_128(&pRhs.words().first[0], not_128(x));
        return pRhs;
    }
#endif

#pragma unroll
    for (int i = 0; i < Words; ++i) {
        pRhs.words().first[i] = ~pRhs.words().first[i];
    }
    return pRhs;
}



// std::numeric_limits
namespace std {
    template<int Words>
    struct numeric_limits< BigInteger<Words> >
        : public UnsignedIntegralNumericLimits<
                BigInteger<Words>, BigInteger<Words>::sBits
          >
    {
    };
}

// std::hash
namespace std {
    template<int Words>
    struct hash< BigInteger<Words> >
    {
        std::size_t operator()(const BigInteger<Words>& pValue)
        {
            return (std::size_t)pValue.hash();
        }
    };
}


#endif // BIGINTEGER_HH
