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

#ifndef UTILS_HH
#include "Utils.hh"
#endif

#ifndef SIMPLEHASHSET_HH
#include "SimpleHashSet.hh"
#endif

namespace Gossamer {
    template<typename RNG, typename Container>
    void sampleWithoutReplacement(RNG& rng, uint32_t n, uint64_t k, Container& container)
    {
        switch (k) {
        case 0:
            return;
        case 1:
        {
            container.reserve(1);
            std::uniform_int_distribution<uint64_t> unif(0, n - 1);
            container.push_back(unif(rng));
            return;
        }
        case 2:
        {
            container.reserve(2);
            std::uniform_int_distribution<uint64_t> unif1(0, n - 1);
            uint64_t x1 = unif1(rng);
            std::uniform_int_distribution<uint64_t> unif2(0, n - 2);
            uint64_t x2 = unif2(rng);
            if (x1 == x2) x2 = n - 1;
            container.push_back(x1);
            container.push_back(x2);
            return;
        }
        }


#if 0
        // Sample without replacement
        //
        // Teuhola, J. and Nevalainen, O. (1982)
        // "Two efficient algorithms for random sampling without replacement."
        // IJCM, 11(2): 127–140. DOI: 10.1080/00207168208803304
        //

        uint64_t hashTableSize = std::max<uint64_t>((uint64_t)1 << (Gossamer::log2(uint64_t(k) + ((k + 1) >> 1)) + 1), 32);
        uint64_t hashTableMask = hashTableSize - 1;

        struct entry { uint64_t value; uint32_t link; };
        std::vector<entry> hashTable;
        hashTable.resize(hashTableSize);
        auto containerBase = container.size();
        container.resize(containerBase + k);

        for (uint32_t i = 0; i < k; ++i) {
            std::uniform_int_distribution<uint64_t> unif(0, n - i - 1);
            uint64_t j = unif(rng);
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
#else
        // Robert Floyd's method
        SimpleHashSet<uint64_t> ht(k + (k >> 1));
        container.reserve(k);
        for (auto j = n - k; j < n; ++j) {
            std::uniform_int_distribution<uint64_t> unif(0, j);
            uint64_t t = unif(rng);
            auto e = ht.count(t) ? j : t;
            ht.insert(e);
            container.push_back(e);
        }
#endif
    }
}

#endif