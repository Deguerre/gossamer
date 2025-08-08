// Copyright (c) 2008-2016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
/**  \file
 * Testing Big Integers.
 *
 * The results for testing rely on results from the gmp R package or Haskell.
 */

#include "BigInteger.hh"
#include "RankSelect.hh"
#include <random>

using namespace boost;
using namespace std;

#define GOSS_TEST_MODULE TestBigInteger
#include "testBegin.hh"

BOOST_AUTO_TEST_CASE(test_static_assertions)
{
    BOOST_STATIC_ASSERT(BigInteger<1>::sBits == 64);
    BOOST_STATIC_ASSERT(BigInteger<2>::sBits == 128);
}

BOOST_AUTO_TEST_CASE(test_dynamic_assertions)
{
    uint64_t x = BigInteger<1>::sBits;
    BOOST_CHECK_EQUAL(x, 64);
    uint64_t y = BigInteger<2>::sBits;
    BOOST_CHECK_EQUAL(y, 128);
}

BOOST_AUTO_TEST_CASE(test_limits)
{
    std::numeric_limits< BigInteger<2> > bi_limits;
    BOOST_CHECK(bi_limits.is_specialized);
    BOOST_CHECK(bi_limits.is_integer);
    BOOST_CHECK(!bi_limits.is_signed);
    BOOST_CHECK(bi_limits.radix == 2);
    BOOST_CHECK(bi_limits.digits == 128);

    std::numeric_limits< Gossamer::position_type > pos_limits;
    BOOST_CHECK(pos_limits.is_specialized);
    BOOST_CHECK(pos_limits.is_integer);
    BOOST_CHECK(!pos_limits.is_signed);
    BOOST_CHECK(pos_limits.radix == 2);
    uint64_t d = pos_limits.digits;
    BOOST_CHECK_EQUAL(d, sizeof(Gossamer::position_type::value_type) * 8);
    BOOST_CHECK_EQUAL(d, sizeof(Gossamer::position_type) * 8);
    BOOST_CHECK_EQUAL(sizeof(Gossamer::position_type), sizeof(Gossamer::position_type::value_type));
}

BOOST_AUTO_TEST_CASE(test_basic_range_128)
{
    BigInteger<2> a(1);
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "1");
    a <<= 16;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "65536");
    a <<= 16;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "4294967296");
    a <<= 16;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "281474976710656");
    a <<= 16;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "18446744073709551616");
    a <<= 16;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "1208925819614629174706176");
    a <<= 16;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "79228162514264337593543950336");
    a <<= 16;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "5192296858534827628530496329220096");
    a <<= 16;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "0");

}

BOOST_AUTO_TEST_CASE(test_add)
{
    BigInteger<2> a(1);
    a <<= 124;
    BigInteger<2> b = a;
    a <<= 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(b), "21267647932558653966460912964485513216");
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "42535295865117307932921825928971026432");
    a += b;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "63802943797675961899382738893456539648");

    a = BigInteger<2>(1);
    a <<= 127;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "170141183460469231731687303715884105728");
    a +=1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "170141183460469231731687303715884105729");

    // 2^64 - 1 Use ULL to be compatible on 32 bit machines.
    b = BigInteger<2>(18446744073709551615ULL);  
    BOOST_CHECK_EQUAL(lexical_cast<string>(b), "18446744073709551615");

    // Check adding a uint_64.
    b = BigInteger<2>(0);
    b += 18446744073709551615ULL;

    a = b;

    a <<= 63;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "170141183460469231722463931679029329920");
    // Now shift one more so that all top 64 bits are 1.
    a <<= 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463444927863358058659840");

    // Result of a + b should be max value 2^128 - 1: all bits 1.
    a += b;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463463374607431768211455");

    a -= 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463463374607431768211454");

    a += 2;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "0");
}

BOOST_AUTO_TEST_CASE(test_subtract)
{
    uint64_t max64 = 18446744073709551615ULL;

    BigInteger<2> a(1);
    a <<= 124;
    BigInteger<2> b = a;
    a <<= 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(b), "21267647932558653966460912964485513216");
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "42535295865117307932921825928971026432");
    a -= b;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "21267647932558653966460912964485513216");

    a = BigInteger<2>(1);
    a <<= 127;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "170141183460469231731687303715884105728");
    a -= 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "170141183460469231731687303715884105727");

    // 2^64 - 1 Use ULL to be compatible on 32 bit machines.
    a = BigInteger<2>(18446744073709551615ULL);  
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "18446744073709551615");

    // Check adding beyond 64bits.
    a += 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "18446744073709551616");
    a -= 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "18446744073709551615");
    a += max64;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "36893488147419103230");
    a -= max64;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "18446744073709551615");
    
    // Check that -1 == max 128-bit integer.
    a = BigInteger<2>(0);
    a -= 1ULL;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463463374607431768211455");

    a = BigInteger<2>(0);
    a -= 2ULL;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463463374607431768211454");

}

BOOST_AUTO_TEST_CASE(test_shift_128)
{
    using namespace Gossamer::detail;

    BigInteger<2> a(1);
    a <<= 126;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "85070591730234615865843651857942052864");
    a >>= 126;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "1");
    a <<= 127;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "170141183460469231731687303715884105728");

    // Left Shift of 1 by 128 bits == 0.
    a <<= 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "0");

    a += 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "1");
    
    a >>= 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "0");
    
    a >>= 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "0");

    a += 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "1");
    
    a <<= 127;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "170141183460469231731687303715884105728");
    a >>= 127;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "1");

    BigInteger<2> b(sPhi0);
    b <<= 64;
    b |= BigInteger<2>(sPhi1);
    BOOST_CHECK_EQUAL(b.mostSigWord(), sPhi0);
    BOOST_CHECK_EQUAL(b.asUInt64(), sPhi1);
    {
        BigInteger<2> x = b;
        x <<= 0;
        BOOST_CHECK_EQUAL(x, b);
        x >>= 0;
        BOOST_CHECK_EQUAL(x, b);
    }
    for (unsigned i = 1; i < 64; ++i) {
        BigInteger<2> x = b;
        x <<= i;
        BOOST_CHECK_EQUAL(x.mostSigWord(), (sPhi0 << i) | (sPhi1 >> (64 - i)));
        BOOST_CHECK_EQUAL(x.asUInt64(), sPhi1 << i);
    }
    for (unsigned i = 64; i < 128; ++i) {
        BigInteger<2> x = b;
        x <<= i;
        BOOST_CHECK_EQUAL(x.mostSigWord(), sPhi1 << (i - 64));
        BOOST_CHECK_EQUAL(x.asUInt64(), 0);
    }
    for (unsigned i = 1; i < 64; ++i) {
        BigInteger<2> x = b;
        x >>= i;
        BOOST_CHECK_EQUAL(x.mostSigWord(), sPhi0 >> i);
        BOOST_CHECK_EQUAL(x.asUInt64(), (sPhi0 << (64 - i)) | (sPhi1 >> i));
    }
    for (unsigned i = 64; i < 128; ++i) {
        BigInteger<2> x = b;
        x >>= i;
        BOOST_CHECK_EQUAL(x.mostSigWord(), 0);
        BOOST_CHECK_EQUAL(x.asUInt64(), sPhi0 >> (i - 64));
    }
}

BOOST_AUTO_TEST_CASE(test_bitwise_128)
{
    uint64_t MAX64 = 18446744073709551615ULL;

    BigInteger<2> MAX_BIG_INTEGER(0);
    MAX_BIG_INTEGER -= 1ULL;
    BOOST_CHECK_EQUAL(lexical_cast<string>(MAX_BIG_INTEGER), "340282366920938463463374607431768211455");

    BigInteger<2> a = MAX_BIG_INTEGER;

    a = ~a;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "0");

    a = BigInteger<2>(MAX64);
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "18446744073709551615");


    // highBits will have all high bits set.
    BigInteger<2> highBits = a;
    highBits <<= 64;
    BOOST_CHECK_EQUAL(lexical_cast<string>(highBits), "340282366920938463444927863358058659840");

    a = ~a;
    BOOST_CHECK(a == highBits);

    a = ~a;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "18446744073709551615");

    // Check bitwise OR.
    a = highBits;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463444927863358058659840");
    a |= BigInteger<2>(MAX64);
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463463374607431768211455");

    // Check that bitwise OR works across a word boundary.
    a = highBits;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463444927863358058659840");

    BigInteger<2> c(1);
    c <<= 64;
    c = ~c;
    // Set bit 64 == 0.
    a &= c;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463426481119284349108224");
    
    BigInteger<2> d(MAX64);
    BOOST_CHECK_EQUAL(lexical_cast<string>(d), "18446744073709551615");
    d <<= 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(d), "36893488147419103230");
    d += 1;
    BOOST_CHECK_EQUAL(lexical_cast<string>(d), "36893488147419103231");
    a |= d;
    // Should be MAX_BIG_INTEGER.
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463463374607431768211455");

    // Check bitwise XOR.
    for (uint64_t i = 0; i < 128; ++i)
    {
        a = MAX_BIG_INTEGER;
        BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463463374607431768211455");
        BigInteger<2> b(1);
        b <<= i;
        a ^= b;
        a ^= b;

        // Should be MAX_BIG_INTEGER.
        BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463463374607431768211455");
    }

    // Check bitwise ops together.
    for (uint64_t i = 0; i < 128; ++i)
    {
        a = MAX_BIG_INTEGER;
        BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463463374607431768211455");

        BigInteger<2> b(1);
        b <<= i;
        BigInteger<2> c = a;
        c &= b;
        BigInteger<2> d = a;
        d &= ~b;

        d |= c;
        // Should be MAX_BIG_INTEGER.
        BOOST_CHECK_EQUAL(lexical_cast<string>(d), "340282366920938463463374607431768211455");
    }

}

BOOST_AUTO_TEST_CASE(test_unary_128)
{
    uint64_t MAX64 = 18446744073709551615ULL;

    BigInteger<2> MAX_BIG_INTEGER(0);
    MAX_BIG_INTEGER -= 1ULL;
    BOOST_CHECK_EQUAL(lexical_cast<string>(MAX_BIG_INTEGER), "340282366920938463463374607431768211455");

    BigInteger<2> a = MAX_BIG_INTEGER;

    ++a;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "0");
    BOOST_CHECK(a==BigInteger<2>(0));
    
    --a;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463463374607431768211455");
    BOOST_CHECK(a==MAX_BIG_INTEGER);
    
    --a;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "340282366920938463463374607431768211454");
    
    a = BigInteger<2>(MAX64);
    ++a;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "18446744073709551616");
    
    a = BigInteger<2>(1);
    --a;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "0");
    BOOST_CHECK(a==BigInteger<2>(0));

    a = BigInteger<2>(MAX64);
    --a;
    BOOST_CHECK_EQUAL(lexical_cast<string>(a), "18446744073709551614");
}

BOOST_AUTO_TEST_CASE(test_shift_word_size)
{
    BigInteger<2> w(1);
    w <<= 32;
    BOOST_CHECK_EQUAL(lexical_cast<string>(w), "4294967296");
    w >>= 32;
    BOOST_CHECK_EQUAL(lexical_cast<string>(w), "1");
    w <<= 64;
    BOOST_CHECK_EQUAL(lexical_cast<string>(w), "18446744073709551616");
    w >>= 64;
    BOOST_CHECK_EQUAL(lexical_cast<string>(w), "1");
}


BOOST_AUTO_TEST_CASE(test_hash)
{
    BigInteger<2> a(21324132ull), b(12352979213ull);
    a <<= 64;
    a += BigInteger<2>(120528743928ull);
    b <<= 64;
    b += BigInteger<2>(83496121239692ull);
    auto [ha1, hb1] = BigInteger<2>::hash2(a, b);
    auto ha2 = a.hash();
    auto hb2 = b.hash();
    BOOST_CHECK_EQUAL(ha1, ha2);
    BOOST_CHECK_EQUAL(hb1, hb2);
}


// Slow version of Wittler canoncal test.
template<int Words>
bool bigIntegerIsWittlerCanonical(const BigInteger<Words>& x, uint64_t pK)
{
    BigInteger<Words> y(x);
    y.reverseComplement(pK);
    uint64_t i = 0;
    auto xw = x.words();
    auto yw = y.words();
    for (; i <= pK / 2; ++i) {
        auto bit = (pK - i - 1) * 2;
        auto w = bit / BigInteger<Words>::sBitsPerWord;
        auto b = bit % BigInteger<Words>::sBitsPerWord;
        auto l = (xw.first[w] >> b) & 3;
        auto r = (yw.first[w] >> b) & 3;
        if (l < r) {
            return true;
        }
        else if (l > r) {
            return false;
        }
    }
    return true;
}


// Slow version of Wittler canonicalise.
template<int Words>
BigInteger<Words> bigIntegerWittlerCanonicalise(const BigInteger<Words>& x, uint64_t pK)
{
    BigInteger<Words> y(x);
    y.reverseComplement(pK);

    uint64_t i = 0;
    auto xw = x.words();
    auto yw = y.words();
    for (; i <= pK / 2; ++i) {
        auto bit = (pK - i - 1) * 2;
        auto w = bit / BigInteger<Words>::sBitsPerWord;
        auto b = bit % BigInteger<Words>::sBitsPerWord;
        auto l = (xw.first[w] >> b) & 3;
        auto r = (yw.first[w] >> b) & 3;
        if (l < r) {
            return x;
        }
        else if (l > r) {
            return y;
        }
    }
}


BOOST_AUTO_TEST_CASE(test_wittler_canon)
{
    unsigned testKs[] = { 11, 19, 31, 33, 63 };
    const unsigned TEST_VALUES = 2;
    std::mt19937 gen(2024111801);
    std::uniform_int_distribution<> distrib(0, 3);

    for (auto k : testKs) {
        for (unsigned test = 0; test < TEST_VALUES; ++test) {
            BigInteger<2> x;
            for (unsigned i = 0; i < k; ++i) {
                BigInteger<2> nt(distrib(gen));
                nt <<= (2 * i);
                x |= nt;
            }

            BOOST_CHECK_EQUAL(x.wittlerCanonical(k), bigIntegerIsWittlerCanonical(x, k));
            auto y1 = x;
            y1.wittlerCanonicalise(k);
            auto y2 = bigIntegerWittlerCanonicalise(x, k);
            BOOST_CHECK_EQUAL(y1, y2);
        }
    }
}


#if 0


BOOST_AUTO_TEST_CASE(test_whittler_3mers)
{
    static unsigned K = 3;

    uint16_t sCodes3[] = {
        16, 12, 8, 4, 17, 13, 9, 0,
        18, 14, 10, 19, 15, 11, 24, 20,
        5, 25, 21, 1, 26, 22, 27, 23,
        28, 6, 29, 2, 30, 31, 7, 3
    };

    unsigned i = 0;
    for (uint64_t v = 0; v < (1ull << (2 * K)); ++v) {
        BigInteger<1> kmer(v);

        if (!kmer.wittlerCanonical(K)) {
            continue;
        }

        auto code = kmer.wittlerEncode(K);
        BOOST_CHECK_EQUAL(code.asUInt64(), sCodes3[i]);

        auto kmer2 = code.wittlerDecode(K);
        BOOST_CHECK_EQUAL(kmer.asUInt64(), kmer2.asUInt64());

        ++i;
    }
}


BOOST_AUTO_TEST_CASE(test_whittler_4mers)
{
    static unsigned K = 4;

	uint16_t sCodes4[] = {
	72, 56, 40, 24, 73, 57, 41, 20,
	74, 58, 42, 16, 75, 59, 43, 0,
	76, 60, 44, 32, 77, 61, 45, 28,
	78, 62, 46, 4, 79, 63, 47, 80,
	64, 48, 36, 81, 65, 49, 8, 82,
	66, 50, 83, 67, 51, 84, 68, 52,
	12, 85, 69, 53, 86, 70, 54, 87,
	71, 55, 104, 88, 25, 105, 89, 21,
	106, 90, 17, 107, 91, 1, 108, 92,
	33, 109, 93, 29, 110, 94, 5, 111,
	95, 112, 96, 37, 113, 97, 9, 114,
	98, 115, 99, 116, 100, 13, 117, 101,
	118, 102, 119, 103, 120, 26, 121, 22,
	122, 18, 123, 2, 124, 34, 125, 30,
	126, 6, 127, 128, 38, 129, 10, 130,
	131, 132, 14, 133, 134, 135, 27, 23,
	19, 3, 35, 31, 7, 39, 11, 15
	};

    unsigned i = 0;
    for (uint64_t v = 1; v < (1ull << (2*K)); ++v) {
        BigInteger<1> kmer(v);

        if (!kmer.wittlerCanonical(K)) {
            continue;
        }

        std::cerr << "Testing kmer " << kmer.asUInt64() << '\n';
        auto code = kmer.wittlerEncode(K);
        // BOOST_CHECK_EQUAL(code.asUInt64(), sCodes4[i]);

        auto kmer2 = code.wittlerDecode(K);
        BOOST_CHECK_EQUAL(kmer.asUInt64(), kmer2.asUInt64());

        ++i;
    }
}
#endif


#include "testEnd.hh"
