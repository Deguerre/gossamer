// Copyright (c) 2023 Andrew J. Bromage.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#include "Sample.hh"
#include <vector>
#include <algorithm>
#include <random>
#include <boost/timer/timer.hpp>


#define GOSS_TEST_MODULE TestSample
#include "testBegin.hh"


void
testSample(uint64_t n, uint32_t k)
{
    std::vector<uint64_t> data;
    std::mt19937 rng(n ^ k);
    Gossamer::sampleWithoutReplacement(rng, n, k, data);
    std::sort(data.begin(), data.end());
    BOOST_CHECK_EQUAL(data.size(), k);
    for (auto i : data) {
        BOOST_CHECK(i < n);
    }
    for (auto i = 0; i + 1 < k; ++i) {
        BOOST_CHECK(data[i] != data[i + 1]);
    }
}


BOOST_AUTO_TEST_CASE(test_sample_without_replacement)
{
    testSample(0, 0);
    testSample(1, 1);
    testSample(2, 2);
    testSample(3, 3);
    testSample(10, 10);
    testSample(100, 100);
    testSample(1000, 1000);
    testSample(1000000, 10);
}


#include "testEnd.hh"
