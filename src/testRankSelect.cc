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
#include <chrono>


#define GOSS_TEST_MODULE TestRankSelect
#include "testBegin.hh"

#undef BENCHMARK_TEST
#undef ALSO_BENCHMARK_STDSORT
#define BENCHMARK_REPS 1000

void
testKmerSort(uint64_t k, uint64_t N)
{
    std::mt19937 rng(k * N);
    std::uniform_int_distribution<> dist(0, 3);

    std::vector<Gossamer::position_type> vec_data;
    std::vector<Gossamer::position_type> vec_gosssort;
    std::vector<Gossamer::position_type> vec_stdsort;
    vec_data.reserve(N);

    for (auto i = 0; i < N; ++i) {
        Gossamer::position_type kmer;
        for (auto j = 0; j < k; ++j) {
            kmer <<= 2;
            kmer |= dist(rng);
        }
        vec_data.push_back(kmer);
    }
    vec_stdsort = vec_data;
    {
#if defined(BENCHMARK_TEST) && defined(ALSO_BENCHMARK_STDSORT)
        std::cerr << "Testing std::sort k=" << k << ", N=" << N << "\n";
#endif
        const auto start = std::chrono::high_resolution_clock::now();
        std::sort(vec_stdsort.begin(), vec_stdsort.end());
        const auto end = std::chrono::high_resolution_clock::now();
#if defined(BENCHMARK_TEST) && defined(ALSO_BENCHMARK_STDSORT)
        std::cerr << "Elapsed time: " << std::chrono::duration<double>(end - start).count() << "s\n";
#endif
    }

#ifdef BENCHMARK_TEST
    std::cerr << "Testing Gossamer sort k=" << k << ", N=" << N << "\n";
    std::chrono::duration<double> time;
    for (unsigned i = 0; i < BENCHMARK_REPS; ++i)
#endif
    {
        vec_gosssort = vec_data;
        const auto start = std::chrono::high_resolution_clock::now();
        Gossamer::sortKmers(k, vec_gosssort);
        const auto end = std::chrono::high_resolution_clock::now();
#ifdef BENCHMARK_TEST
        time += std::chrono::duration<double>(end - start);
#endif
    }
#ifdef BENCHMARK_TEST
    std::cerr << "Average elapsed time: " << (time.count() / BENCHMARK_REPS) << "s\n";
#endif
    for (int i = 0; i < N; ++i) {
        BOOST_CHECK_EQUAL(vec_stdsort[i], vec_gosssort[i]);
    }
}

BOOST_AUTO_TEST_CASE(test_kmer_sort)
{
#ifdef BENCHMARK_TEST
    static const size_t sizes[] = { 1000000, 10000000 };
#else
    static const size_t sizes[] = { 10, 20, 1000, 10000, 1000000 };
#endif
    for (auto size : sizes) {
        testKmerSort(27, size);
        testKmerSort(32, size);
        testKmerSort(34, size);
    }
}

#include "testEnd.hh"
