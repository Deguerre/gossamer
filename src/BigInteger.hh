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

#undef GOSS_NO_SIMD

#if defined(GOSS_EXT_SSE2)
#include "MachDepSSE.hh"
#elif defined(GOSS_EXT_NEON)
#include "MachDepNeon.hh"
#else
#include "MachDepNoSimd.hh"
#endif


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
        Gossamer::unrolled_loop<int, Words>([&](auto i) {
            test |= mImpl.mWords[i];
        });
        return test != 0;
    }

    bool fitsIn64Bits() const
    {
        word_type test(0);
        for (int64_t i = 1; i < Words; ++i)
        {
            test |= mImpl.mWords[i];
        }
        return !test;
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
        Gossamer::unrolled_loop<int, Words>([&](auto j) {
            auto i = Words - 1 - j;
            value = value * scale + mImpl.mWords[i];
        });
        return value;
    }

    /// Reverse two-bit sequences.
    void reverse()
    {
        Gossamer::unrolled_loop<int, Words / 2>([&](auto i) {
            word_type tmp = Gossamer::reverseBase4(mImpl.mWords[i]);
            mImpl.mWords[i] = Gossamer::reverseBase4(mImpl.mWords[Words - i - 1]);
            mImpl.mWords[Words - i - 1] = tmp;
        });
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

        Gossamer::unrolled_loop<int, Words/2>([&](auto i) {
            word_type tmp = Gossamer::reverseBase4(~mImpl.mWords[i]);
            mImpl.mWords[i] = Gossamer::reverseBase4(~mImpl.mWords[Words - i - 1]);
            mImpl.mWords[Words - i - 1] = tmp;
        });
        if (Words & 1)
        {
            mImpl.mWords[Words/2] = Gossamer::reverseBase4(~mImpl.mWords[Words/2]);
        }
        *this >>= sBits - 2*pK;
    }
    
    BigInteger& operator+=(uint64_t pRhs)
    {
        bool carry = Gossamer::add64(mImpl.mWords[0], pRhs, false, mImpl.mWords[0]);
        Gossamer::unrolled_loop<int, Words - 1>([&](auto j) {
            auto i = j + 1;
            carry = Gossamer::add64(mImpl.mWords[i], 0, carry, mImpl.mWords[i]);
        });
        return *this;
    }

    BigInteger& operator+=(const BigInteger& pRhs)
    {
        bool carry = Gossamer::add64(mImpl.mWords[0], pRhs.mImpl.mWords[0], false, mImpl.mWords[0]);
        Gossamer::unrolled_loop<int, Words - 1>([&](auto j) {
            auto i = j + 1;
            carry = Gossamer::add64(mImpl.mWords[i], pRhs.mImpl.mWords[i], carry, mImpl.mWords[i]);
        });
        return *this;
    }

    // Optimised add-plus-1.
    BigInteger& add1(uint64_t pRhs)
    {
        bool carry = Gossamer::add64(mImpl.mWords[0], pRhs, true, mImpl.mWords[0]);
        Gossamer::unrolled_loop<int, Words - 1>([&](auto j) {
            auto i = j + 1;
            carry = Gossamer::add64(mImpl.mWords[i], 0, carry, mImpl.mWords[i]);
        });
        return *this;
    }

    // Optimised add-plus-1.
    BigInteger& add1(const BigInteger& pRhs)
    {
        bool carry = Gossamer::add64(mImpl.mWords[0], pRhs.mImpl.mWords[0], true, mImpl.mWords[0]);
        Gossamer::unrolled_loop<int, Words - 1>([&](auto j) {
            auto i = j + 1;
            carry = Gossamer::add64(mImpl.mWords[i], pRhs.mImpl.mWords[i], carry, mImpl.mWords[i]);
            });
        return *this;
    }

    BigInteger& operator-=(uint64_t pRhs)
    {
        bool carry = Gossamer::sub64(mImpl.mWords[0], pRhs, false, mImpl.mWords[0]);
        Gossamer::unrolled_loop<int, Words - 1>([&](auto j) {
            auto i = j + 1;
            carry = Gossamer::sub64(mImpl.mWords[i], 0, carry, mImpl.mWords[i]);
        });
        return *this;
    }

    BigInteger& operator-=(const BigInteger& pRhs)
    {
        bool carry = Gossamer::sub64(mImpl.mWords[0], pRhs.mImpl.mWords[0], false, mImpl.mWords[0]);
        Gossamer::unrolled_loop<int, Words - 1>([&](auto j) {
            auto i = j + 1;
            carry = Gossamer::sub64(mImpl.mWords[i], pRhs.mImpl.mWords[i], carry, mImpl.mWords[i]);
        });
        return *this;
    }

    // Optimised subtract-minus-1.
    BigInteger& subtract1(const BigInteger& pRhs)
    {
        bool carry = Gossamer::sub64(mImpl.mWords[0], pRhs.mImpl.mWords[0], true, mImpl.mWords[0]);
        Gossamer::unrolled_loop<int, Words - 1>([&](auto j) {
            auto i = j + 1;
            carry = Gossamer::sub64(mImpl.mWords[i], pRhs.mImpl.mWords[i], carry, mImpl.mWords[i]);
            });
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
        Gossamer::unrolled_loop<int, Words>([&](auto j) {
            auto i = Words - 1 - j;
            mImpl.mWords[i] = i >= wordShift ? mImpl.mWords[i - wordShift] : word_type(0);
        });
        pShift -= wordShift * sBitsPerWord;

        if (pShift > 0) {
            word_type carry = 0;
            Gossamer::unrolled_loop<int, Words>([&](auto i) {
                word_type nextCarry = mImpl.mWords[i] >> (sBitsPerWord - pShift);
                mImpl.mWords[i] = (mImpl.mWords[i] << pShift) | carry;
                carry = nextCarry;
            });
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
        Gossamer::unrolled_loop<int, Words>([&](auto i) {
            mImpl.mWords[i] = i + wordShift < Words ? mImpl.mWords[i + wordShift] : word_type(0);
        });
        pShift -= wordShift * sBitsPerWord;

        if (pShift > 0)
        {
            word_type carry = 0;
            Gossamer::unrolled_loop<int, Words>([&](auto j) {
                auto i = Words - 1 - j;
                word_type nextCarry = mImpl.mWords[i] << (sBitsPerWord - pShift);
                mImpl.mWords[i] = (mImpl.mWords[i] >> pShift) | carry;
                carry = nextCarry;
            });
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

        Gossamer::unrolled_loop<int, Words>([&](auto i) {
            mImpl.mWords[i] |= pRhs.mImpl.mWords[i];
        });
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

        Gossamer::unrolled_loop<int, Words>([&](auto i) {
            mImpl.mWords[i] &= pRhs.mImpl.mWords[i];
        });
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
        Gossamer::unrolled_loop<int, Words>([&](auto i) {
            mImpl.mWords[i] ^= pRhs.mImpl.mWords[i];
        });
        return *this;
    }

    BigInteger& operator++()
    {
        bool carry = Gossamer::add64(mImpl.mWords[0], 0, true, mImpl.mWords[0]);
        Gossamer::unrolled_loop<int, Words - 1>([&](auto j) {
            auto i = j + 1;
            carry = Gossamer::add64(mImpl.mWords[i], 0, carry, mImpl.mWords[i]);
        });
        return *this;
    }

    BigInteger& operator--()
    {
        bool carry = Gossamer::sub64(mImpl.mWords[0], 0, true, mImpl.mWords[0]);
        Gossamer::unrolled_loop<int, Words - 1>([&](auto j) {
            auto i = j + 1;
            carry = Gossamer::sub64(mImpl.mWords[i], 0, carry, mImpl.mWords[i]);
        });
        return *this;
    }

    uint64_t log2() const
    {
        // XXX TODO SIMD implementation
        for (int64_t i = Words - 1; i >= 0; --i)
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
        bool equal = mImpl.mWords[0] == pRhs.mImpl.mWords[0];
        Gossamer::unrolled_loop<int, Words-1>([&](auto i) {
            equal = mImpl.mWords[i+1] == pRhs.mImpl.mWords[i+1] && equal;
        });

        return equal;
    }

    bool nonZero() const
    {
        word_type w = mImpl.mWords[0];
        unrolled_loop<int, Words - 1>([&](auto i) {
            w |= mImpl.mWords[i + 1];
        });
        return w != 0;
    }

    bool operator<(const BigInteger& pRhs) const
    {
        if constexpr (Words == 1) {
            return mImpl.mWords[0] < pRhs.mImpl.mWords[0];
        }

        if constexpr (Words == 2) {
            bool w1eq = mImpl.mWords[1] == pRhs.mImpl.mWords[1];
            bool w1lt = mImpl.mWords[1] < pRhs.mImpl.mWords[1];
            bool w0lt = mImpl.mWords[0] < pRhs.mImpl.mWords[0];
            return w1lt || (w1eq && w0lt);
        }

        for (int64_t i = Words-1; i >= 0; --i)
        {
            if (mImpl.mWords[i] == pRhs.mImpl.mWords[i]) {
                continue;
            }
            return mImpl.mWords[i] < pRhs.mImpl.mWords[i];
        }

        return false;
    }

    static bool equalWithMask(const BigInteger& lhs, const BigInteger& rhs, const BigInteger& mask)
    {
        bool ok = true;
        Gossamer::unrolled_loop<int, Words>([&](auto i) {
            ok = (lhs.mImpl.mWords[i] & mask.mImpl.mWords[i]) == (rhs.mImpl.mWords[i] & mask.mImpl.mWords[i]) && ok;
        });
        return ok;
    }

    static bool testAgainstMask(const BigInteger& lhs, const BigInteger& mask)
    {
        bool ok = false;
        Gossamer::unrolled_loop<int, Words>([&](auto i) {
            ok = (lhs.mImpl.mWords[i] & mask.mImpl.mWords[i]) != 0 || ok;
        });
        return ok;
    }

    uint64_t hash() const
    {
        uint64_t hash = Gossamer::sPhi0;

        Gossamer::unrolled_loop<int, Words>([&](auto i) {
            uint64_t w = mImpl.mWords[i];
            uint64_t whi = w >> 32;
            uint64_t wlo = w & 0xFFFFFFFF;

            uint64_t hashlo = (Gossamer::sPhi1 * whi + Gossamer::sPhi2 * wlo + Gossamer::sPhi3 * hash) >> 32;
            uint64_t hashhi = (Gossamer::sPhi4 * whi + Gossamer::sPhi5 * wlo + Gossamer::sPhi6 * hash) >> 32;
            hash = hashlo | (hashhi << 32);
        });
        return hash;
    }

    static std::pair<uint64_t, uint64_t> hash2(const BigInteger& lhs, const BigInteger& rhs)
    {
        uint64_t hash0 = Gossamer::sPhi0;
        uint64_t hash1 = Gossamer::sPhi0;

        Gossamer::unrolled_loop<int, Words>([&](auto i) {
            uint64_t w0 = lhs.mImpl.mWords[i];
            uint64_t w1 = rhs.mImpl.mWords[i];
            uint64_t w0hi = w0 >> 32;
            uint64_t w1hi = w1 >> 32;
            uint64_t w0lo = w0 & 0xFFFFFFFF;
            uint64_t w1lo = w1 & 0xFFFFFFFF;

            uint64_t hash0lo = (Gossamer::sPhi1 * w0hi + Gossamer::sPhi2 * w0lo + Gossamer::sPhi3 * hash0) >> 32;
            uint64_t hash1lo = (Gossamer::sPhi1 * w1hi + Gossamer::sPhi2 * w1lo + Gossamer::sPhi3 * hash1) >> 32;
            uint64_t hash0hi = (Gossamer::sPhi4 * w0hi + Gossamer::sPhi5 * w0lo + Gossamer::sPhi6 * hash0) >> 32;
            uint64_t hash1hi = (Gossamer::sPhi4 * w1hi + Gossamer::sPhi5 * w1lo + Gossamer::sPhi6 * hash1) >> 32;
            hash0 = hash0lo | (hash0hi << 32);
            hash1 = hash1lo | (hash1hi << 32);
        });
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
    Gossamer::unrolled_loop<int, Words>([&](auto i) {
        pRhs.words().first[i] = ~pRhs.words().first[i];
    });
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
