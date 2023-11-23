// Copyright (c) 2023 Andrew J. Bromage.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#include "RankSelect.hh"
#include <vector>
#include <algorithm>
#include <random>
#include <boost/timer/timer.hpp>


#define GOSS_TEST_MODULE TestRankSelect
#include "testBegin.hh"


#undef TIMING_TEST

void
testKmerSort(uint64_t k, uint64_t N)
{
    std::mt19937 rng(k * N);
    std::uniform_int_distribution<> dist(0, 3);

    std::vector<Gossamer::position_type> vec1;
    std::vector<Gossamer::position_type> vec2;
    vec1.reserve(N);
    vec2.reserve(N);

    for (auto i = 0; i < N; ++i) {
        Gossamer::position_type kmer;
        for (auto j = 0; j < k; ++j) {
            kmer <<= 2;
            kmer |= dist(rng);
        }
        vec1.push_back(kmer);
        vec2.push_back(kmer);
    }
    {
#ifdef TIMING_TEST
        std::cerr << "Testing Gossamer sort k=" << k << ", N=" << N << "\n";
        boost::timer::auto_cpu_timer timer;
#endif
        Gossamer::sortKmers(k, vec1);
    }
    {
#ifdef TIMING_TEST
        std::cerr << "Testing std::sort k=" << k << ", N=" << N << "\n";
        boost::timer::auto_cpu_timer timer;
#endif
        std::sort(vec2.begin(), vec2.end());
    }
    for (int i = 0; i < N; ++i) {
        BOOST_CHECK_EQUAL(vec1[i], vec2[i]);
    }
}

BOOST_AUTO_TEST_CASE(test_kmer_sort)
{
#ifdef TIMING_TEST
    static const size_t sizes[] = { 10, 1000, 10000, 1000000, 100000000 };
#else
    static const size_t sizes[] = { 10, 1000, 10000, 1000000 };
#endif
    for (auto size : sizes) {
        testKmerSort(27, size);
        testKmerSort(32, size);
        testKmerSort(36, size);
    }
}

#include "testEnd.hh"
