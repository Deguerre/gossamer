// Copyright (c) 2008-2016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#include "Utils.hh"

#include <boost/dynamic_bitset.hpp>
#include <boost/timer/timer.hpp>
#include <random>


using namespace boost;
using namespace std;

#define GOSS_TEST_MODULE TestUtils
#include "testBegin.hh"


BOOST_AUTO_TEST_CASE(test1)
{
    BOOST_CHECK_EQUAL(Gossamer::log2(1), 0);
    BOOST_CHECK_EQUAL(Gossamer::log2(2), 1);
    BOOST_CHECK_EQUAL(Gossamer::log2(3), 2);
    BOOST_CHECK_EQUAL(Gossamer::log2(4), 2);
    BOOST_CHECK_EQUAL(Gossamer::log2(5), 3);
    BOOST_CHECK_EQUAL(Gossamer::log2(6), 3);
    BOOST_CHECK_EQUAL(Gossamer::log2(7), 3);
    BOOST_CHECK_EQUAL(Gossamer::log2(8), 3);
    BOOST_CHECK_EQUAL(Gossamer::log2(9), 4);
}

BOOST_AUTO_TEST_CASE(test2)
{
    uint64_t w = 0x5;
    BOOST_CHECK_EQUAL(Gossamer::select1(w, 0), 0);
    BOOST_CHECK_EQUAL(Gossamer::select1(w, 1), 2);
    for (uint64_t i = 0; i < 64; ++i)
    {
        BOOST_CHECK_EQUAL(Gossamer::select1(0xFFFFFFFFFFFFFFFFull, i), i);
    }
    for (uint64_t i = 0; i < 32; ++i)
    {
        BOOST_CHECK_EQUAL(Gossamer::select1(0x5555555555555555ull, i), 2*i+0);
        BOOST_CHECK_EQUAL(Gossamer::select1(0xAAAAAAAAAAAAAAAAull, i), 2*i+1);
    }
    for (uint64_t i = 0; i < 16; ++i)
    {
        BOOST_CHECK_EQUAL(Gossamer::select1(0x1111111111111111ull, i), 4*i+0);
        BOOST_CHECK_EQUAL(Gossamer::select1(0x2222222222222222ull, i), 4*i+1);
        BOOST_CHECK_EQUAL(Gossamer::select1(0x4444444444444444ull, i), 4*i+2);
        BOOST_CHECK_EQUAL(Gossamer::select1(0x8888888888888888ull, i), 4*i+3);
    }
}

BOOST_AUTO_TEST_CASE(testClz)
{
    uint64_t x = 1ull << 63;
    for (uint64_t i = 0; i <= 64; ++i)
    {
        BOOST_CHECK_EQUAL(Gossamer::count_leading_zeroes(x), i);
        x >>= 1;
    }
}


static void
check_bounds(int* b, int* e, int vl, int vu)
{
    // The simplest way to test the binary search operations is to
    // see if they return the same thing as the platinum-iridium
    // implementation. Hopefully the C++ standard library is bug-free.

    auto lb = std::lower_bound(b, e, vl);
    auto lb16 = Gossamer::lower_bound_on_pointers<int,16>(b, e, vl);
    auto lb200 = Gossamer::lower_bound_on_pointers<int,200>(b, e, vl);
    auto tlb16 = Gossamer::tuned_lower_bound<int*,int,std::less<int>,16>(b, e, vl, std::less<int>());
    auto tlb200 = Gossamer::tuned_lower_bound<int*,int,std::less<int>,200>(b, e, vl, std::less<int>());
    BOOST_CHECK_EQUAL(lb, lb16);
    BOOST_CHECK_EQUAL(lb, lb200);
    BOOST_CHECK_EQUAL(lb, tlb16);
    BOOST_CHECK_EQUAL(lb, tlb200);

    auto ub = std::upper_bound(b, e, vu);
    auto ub16 = Gossamer::upper_bound_on_pointers<int,16>(b, e, vu);
    auto ub200 = Gossamer::upper_bound_on_pointers<int,200>(b, e, vu);
    auto tub16 = Gossamer::tuned_upper_bound<int*,int,std::less<int>,16>(b, e, vu, std::less<int>());
    auto tub200 = Gossamer::tuned_upper_bound<int*,int,std::less<int>,200>(b, e, vu, std::less<int>());
    BOOST_CHECK_EQUAL(ub, ub16);
    BOOST_CHECK_EQUAL(ub, ub200);
    BOOST_CHECK_EQUAL(ub, tub16);
    BOOST_CHECK_EQUAL(ub, tub200);

    decltype(lb) lb16c, ub16c, lb200c, ub200c;
    Gossamer::tuned_lower_and_upper_bound<int*, int, std::less<int>, 16>(b, e, vl, vu, lb16c, ub16c, std::less<int>());
    Gossamer::tuned_lower_and_upper_bound<int*, int, std::less<int>, 200>(b, e, vl, vu, lb200c, ub200c, std::less<int>());
    auto ulb16 = Gossamer::lower_and_upper_bound_on_pointers<int, 16>(b, e, vl, vu);
    auto ulb200 = Gossamer::lower_and_upper_bound_on_pointers<int, 200>(b, e, vl, vu);
    BOOST_CHECK_EQUAL(lb, lb16c);
    BOOST_CHECK_EQUAL(lb, lb200c);
    BOOST_CHECK_EQUAL(lb, ulb16.first);
    BOOST_CHECK_EQUAL(lb, ulb200.first);
    BOOST_CHECK_EQUAL(ub, ub16c);
    BOOST_CHECK_EQUAL(ub, ub200c);
    BOOST_CHECK_EQUAL(ub, ulb16.second);
    BOOST_CHECK_EQUAL(ub, ulb200.second);
}

BOOST_AUTO_TEST_CASE(testUpperLowerBound)
{
    int c[100];
    for (unsigned i = 0; i < 100; ++i)
    {
        c[i] = i;
    }

    static int test_values[] = { -1, 0, 1, 2, 3, 29, 49, 50, 51, 52, 97, 99, 100, 101 };
    for (auto l : test_values) {
        for (auto u : test_values) {
            if (l > u) continue;
            check_bounds(&c[0], &c[100], l, u);
        }
    }

    for (unsigned i = 0; i < 100; ++i)
    {
        c[i] = (i & ~(int)1) + 1;
    }

    for (auto l : test_values) {
        for (auto u : test_values) {
            if (l > u) continue;
            check_bounds(&c[0], &c[100], l, u);
        }
    }
}


BOOST_AUTO_TEST_CASE(timeSelectMethods)
{
    const unsigned test_count = 100000000;
    struct test_case {
        uint64_t word;
        uint32_t r, res;
        test_case(uint64_t pWord, uint32_t pR, uint32_t pRes)
            : word(pWord), r(pR), res(pRes)
        {
        }
    };
    std::vector<test_case> tests;
    tests.reserve(test_count);
    for (unsigned i = 0; i < test_count; ++i) {
        uint64_t word = 0xcbf29ce484222325ull;
        uint32_t r = 0x811c9dc5;
        uint64_t ww = i;
        for (unsigned j = 0; j < 8; ++j) {
            word *= 1099511628211ull;
            word ^= ww & 0xFF;
            r *= 16777619;
            r ^= ww & 0xFF;
            ww >>= 8;
        }
        if (!word) continue;
        r = r % Gossamer::popcnt(word);
        tests.emplace_back(word, r, Gossamer::select_by_ffs(word, r));
    }

    {
        std::cerr << "Testing vigna method\n";
        boost::timer::auto_cpu_timer timer;
        bool ok = true;
        for (auto& t : tests) {
            ok &= Gossamer::select_by_vigna(t.word, t.r) == t.res;
        }
        BOOST_CHECK(ok);
    }

    {
        std::cerr << "Testing mask method\n";
        boost::timer::auto_cpu_timer timer;
        bool ok = true;
        for (auto& t : tests) {
            ok &= Gossamer::select_by_mask(t.word, t.r) == t.res;
        }
        BOOST_CHECK(ok);
    }

    {
        std::cerr << "Testing pdep method\n";
        boost::timer::auto_cpu_timer timer;
        bool ok = true;
        for (auto& t : tests) {
            ok &= Gossamer::select_by_pdep(t.word, t.r) == t.res;
        }
        BOOST_CHECK(ok);
    }
}


#include "testEnd.hh"
