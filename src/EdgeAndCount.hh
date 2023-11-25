// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#ifndef EDGEANDCOUNT_HH
#define EDGEANDCOUNT_HH

#ifndef RANKSELECT_HH
#include "RankSelect.hh"
#endif

#ifndef TRIVIALVECTOR_HH
#include "TrivialVector.hh"
#endif

#ifndef INTEGERCODECS_HH
#include "IntegerCodecs.hh"
#endif

#ifndef STD_ISTREAM
#include <istream>
#define STD_ISTREAM
#endif

#ifndef STD_OSTREAM
#include <ostream>
#define STD_OSTREAM
#endif

#ifndef STD_UTILITY
#include <utility>
#define STD_UTILITY
#endif

#ifndef STDIO_H
#include <stdio.h>
#define STDIO_H
#endif


#undef GOSS_CODEC_RUNTIME_CHECKS

namespace Gossamer
{
    typedef std::pair<position_type,uint64_t> EdgeAndCount;


    template<typename Item>
    struct EdgeItemTraits
    {
    };

    template<>
    struct EdgeItemTraits<EdgeAndCount>
    {
        static inline void combine(EdgeAndCount& pLhs, const EdgeAndCount& pRhs)
        {
            pLhs.second += pRhs.second;
        }

        static inline const position_type& edge(const EdgeAndCount& pEC)
        {
            return pEC.first;
        }

        static inline position_type& edge(EdgeAndCount& pEC)
        {
            return pEC.first;
        }
    };

    template<>
    struct EdgeItemTraits<position_type>
    {
        static inline void combine(position_type& pLhs, const position_type& pRhs)
        {
        }

        static inline const position_type& edge(const position_type& pEdge)
        {
            return pEdge;
        }

        static inline position_type& edge(position_type& pEdge)
        {
            return pEdge;
        }
    };


}
// namespace Gossamer

namespace // anonymous
{
    class InAdapter
    {
    public:
        uint8_t operator*()
        {
            int c = mFile.peek();
            return (c != EOF ? c : 0);
        }

        void operator++()
        {
            mFile.get();
            return;
        }

        InAdapter(std::istream& pFile)
            : mFile(pFile)
        {
        }

    private:
        std::istream& mFile;
    };
}
// namespace anonymous


template<typename Item>
struct EdgeCodec
{
    static void encode(std::ostream& pOut, const Gossamer::position_type& pPrevEdge, const Item& pItm);
    static void decode(std::istream& pIn, Item& pItm);
};


template<>
struct EdgeCodec<Gossamer::EdgeAndCount>
{
        static void encode(std::ostream& pOut,
                           const Gossamer::position_type& pPrevEdge,
                           const Gossamer::EdgeAndCount& pItm)
        {
#ifdef GOSS_CODEC_RUNTIME_CHECKS
            BOOST_ASSERT(pPrevEdge == ~Gossamer::position_type(0) ||  pPrevEdge <= pItm.first);
#endif
            TrivialVector<uint8_t,sizeof(Gossamer::EdgeAndCount) * 9 / 8 + 1> v;
            Gossamer::position_type::value_type d = pItm.first.value();
            d.subtract1(pPrevEdge.value());

            std::pair<const uint64_t*,const uint64_t*> ws = d.words();
            for (const uint64_t* i = ws.first; i < ws.second; ++i)
            {
                VByteCodec::encode(*i, v);
            }
            VByteCodec::encode(pItm.second, v);
            pOut.write(reinterpret_cast<const char*>(&v[0]), v.size());
        }

        static void decode(std::istream& pIn, Gossamer::EdgeAndCount& pItm)
        {
            InAdapter adapter(pIn);
            Gossamer::position_type::value_type d = pItm.first.value();
            std::pair<uint64_t*,uint64_t*> ws = d.words();
            for (uint64_t* i = ws.first; i != ws.second; ++i)
            {
                *i = VByteCodec::decode(adapter);
            }
            d.add1(pItm.first.value());
            pItm.first = Gossamer::position_type(d);
            pItm.second = VByteCodec::decode(adapter);
        }
};

template<>
struct EdgeCodec<Gossamer::position_type>
{
    static void encode(std::ostream& pOut,
        const Gossamer::position_type& pPrevEdge,
        const Gossamer::position_type & pItm)
    {
#ifdef GOSS_CODEC_RUNTIME_CHECKS
        BOOST_ASSERT(pPrevEdge == ~Gossamer::position_type(0) || pPrevEdge <= pItm);
#endif
        TrivialVector<uint8_t, sizeof(Gossamer::position_type) * 9 / 8 + 1> v;
        Gossamer::position_type::value_type d = pItm.value();
        d -= pPrevEdge.value();

        std::pair<const uint64_t*, const uint64_t*> ws = d.words();
        for (const uint64_t* i = ws.first; i < ws.second; ++i)
        {
            VByteCodec::encode(*i, v);
        }
        pOut.write(reinterpret_cast<const char*>(&v[0]), v.size());
    }

    static void decode(std::istream& pIn, Gossamer::position_type& pItm)
    {
        InAdapter adapter(pIn);
        Gossamer::position_type::value_type d = pItm.value();
        std::pair<uint64_t*, uint64_t*> ws = d.words();
        for (uint64_t* i = ws.first; i != ws.second; ++i)
        {
            *i = VByteCodec::decode(adapter);
        }
        d += pItm.value();
        pItm = Gossamer::position_type(d);
    }
};


#endif // EDGEANDCOUNT_HH
