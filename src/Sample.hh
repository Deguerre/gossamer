// Copyright (c) 2023 Andrew J. Bromage.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#ifndef SAMPLE_HH
#define SAMPLE_HH

#ifndef STD_RANDOM
#include <random>
#define STD_RANDOM
#endif

#ifndef STD_ITERATOR
#include <random>
#define STD_ITERATOR
#endif

#ifndef UTILS_HH
#include "Utils.hh"
#endif

namespace Gossamer {
    // Sample without replacement
    //
    // Teuhola, J. and Nevalainen, O. (1982)
    // "Two efficient algorithms for random sampling without replacement."
    // IJCM, 11(2): 127–140. DOI: 10.1080/00207168208803304
    //
    template<typename RNG, typename Container>
    void sampleWithoutReplacement(RNG& rng, uint32_t n, uint64_t k, Container& container)
    {
        std::uniform_real_distribution<double> unif(0, 1);

        switch (k) {
        case 0:
            return;
        case 1:
            container.reserve(1);
            container.push_back((uint64_t)(n * unif(rng)));
            return;
        case 2:
            container.reserve(2);
            uint64_t x1 = n * unif(rng);
            uint64_t x2 = (n - 1) * unif(rng);
            if (x1 == x2) x2 = n - 1;
            container.push_back(x1);
            container.push_back(x2);
            return;
        }

        uint64_t hashTableSize = std::max<uint64_t>((uint64_t)1 << (Gossamer::log2(uint64_t(k) + ((k+1) >> 1)) + 1), 32);
        uint64_t hashTableMask = hashTableSize - 1;

        struct entry { uint64_t value; uint32_t link; };
        std::vector<entry> hashTable;
        hashTable.resize(hashTableSize);

        auto containerBase = container.size();
        container.resize(containerBase + k);

        for (uint32_t i = 0; i < k; ++i) {
            uint64_t j = (uint64_t)((n - i) * unif(rng));
            uint64_t h;
            for (h = j & hashTableMask; ; h = (h + 1) & hashTableMask) {
                if (!hashTable[h].link) {
                    container[containerBase + i] = j;
                    break;
                }
                if (hashTable[h].link == j + 1) {
                    container[containerBase + i] = hashTable[h].value;
                    break;
                }
            }
            for (auto h2 = (n - i - 1) & hashTableMask; ; h2 = (h2 + 1) & hashTableMask) {
                if (!hashTable[h2].link) {
                    hashTable[h].value = n - i - 1;
                    break;
                }
                if (hashTable[h2].link == n - i) {
                    hashTable[h].value = hashTable[h2].value;
                    break;
                }
            }
            hashTable[h].link = j + 1;
        }
    }
}

#endif