// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#ifndef ASYNCMERGE_HH
#define ASYNCMERGE_HH

#ifndef FILEFACTORY_HH
#include "FileFactory.hh"
#endif

#ifndef STD_VECTOR
#include <vector>
#define STD_VECTOR
#endif

class AsyncMerge
{
public:
    struct Part {
        uint64_t mNumber;
        std::string mFname;
        uint64_t mSize;

        Part(uint64_t pNumber, std::string pFname, uint64_t pSize)
            : mNumber(pNumber), mFname(pFname), mSize(pSize)
        {
        }
    };
    template <typename Kind, typename Item = Gossamer::EdgeAndCount>
    static void merge(const std::vector<Part>& pParts,
                      const std::string& pGraphName,
                      uint64_t pK, uint64_t pN, uint64_t pNumThreads, uint64_t pBufferSize,
                      FileFactory& pFactory);
};

#include "AsyncMerge.tcc"

#endif // ASYNCMERGE_HH
