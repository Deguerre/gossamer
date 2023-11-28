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
#define GOSS_CODEC_FORCE_VBYTE

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
    template<typename Type>
    class InAdapter
    {
    };

    template<>
    class InAdapter<uint8_t>
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


    template<>
    class InAdapter<uint64_t>
    {
    public:
        uint64_t operator*()
        {
            return mEof ? 0 : mBuffer;
        }

        void operator++()
        {
            fillBuffer();
        }

        InAdapter(std::istream& pFile)
            : mFile(pFile)
        {
            fillBuffer();
        }

    private:
        std::istream& mFile;
        uint64_t mBuffer;
        bool mEof;

        void fillBuffer() {
            mFile.read((char*)&mBuffer, sizeof(mBuffer));
            mEof = !mFile.good();
        }
    };
}
// namespace anonymous


template<typename Item>
struct EdgeEncoder
{
};


template<typename Item>
struct EdgeDecoder
{
};


template<>
struct EdgeEncoder<Gossamer::EdgeAndCount>
{
        void encode(std::ostream& pOut,
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

        void flush(std::ostream& pOut)
        {
        }
};


template<>
struct EdgeDecoder<Gossamer::EdgeAndCount>
{
    void decode(Gossamer::EdgeAndCount& pItm)
    {
        Gossamer::position_type::value_type d = pItm.first.value();
        std::pair<uint64_t*, uint64_t*> ws = d.words();
        for (uint64_t* i = ws.first; i != ws.second; ++i)
        {
            *i = VByteCodec::decode(mAdapter);
        }
        d.add1(pItm.first.value());
        pItm.first = Gossamer::position_type(d);
        pItm.second = VByteCodec::decode(mAdapter);
    }

    EdgeDecoder(std::istream& pIn)
        : mIn(pIn), mAdapter(pIn)
    {
    }

    bool good() const {
        return mIn.good();
    }

private:
    std::istream& mIn;
    InAdapter<uint8_t> mAdapter;
};


template<>
struct EdgeEncoder<Gossamer::position_type>
{
    void encode(std::ostream& pOut,
        const Gossamer::position_type& pPrevEdge,
        const Gossamer::position_type & pItm)
    {
#ifdef GOSS_CODEC_RUNTIME_CHECKS
        BOOST_ASSERT(pPrevEdge == ~Gossamer::position_type(0) || pPrevEdge <= pItm);
#endif

#ifndef GOSS_CODEC_FORCE_VBYTE
        typedef uint64_t encoded_type;
#else
        typedef uint8_t encoded_type;
#endif

        TrivialVector<encoded_type, 20> v;
        Gossamer::position_type d = pItm;
        d.value().subtract1(pPrevEdge.value());
 
#ifndef GOSS_CODEC_FORCE_VBYTE
        mEncoder.encode(d, v);
#else
        std::pair<const uint64_t*, const uint64_t*> ws = d.words();
        for (const uint64_t* i = ws.first; i < ws.second; ++i)
        {
            VByteCodec::encode(*i, v);
        }
#endif
        pOut.write(reinterpret_cast<const char*>(&v[0]), v.size() * sizeof(encoded_type));
    }

    void flush(std::ostream& pOut)
    {
#ifndef GOSS_CODEC_FORCE_VBYTE
        TrivialVector<uint64_t, 20> v;
        mEncoder.flush(v);
        pOut.write(reinterpret_cast<const char*>(&v[0]), v.size() * sizeof(uint64_t));
#endif
    }

private:
#ifndef GOSS_CODEC_FORCE_VBYTE
    Simple8bEncode<Gossamer::position_type> mEncoder;
#endif
};


template<>
struct EdgeDecoder<Gossamer::position_type>
{
#ifndef GOSS_CODEC_FORCE_VBYTE
    typedef uint64_t encoded_type;
#else
    typedef uint8_t encoded_type;
#endif

    void decode(Gossamer::position_type& pItm)
    {
#ifndef GOSS_CODEC_FORCE_VBYTE
        auto dec = mDecoder.decode(mAdapter);
        pItm.value().add1(dec.value());
#else
        Gossamer::position_type::value_type d;
        std::pair<uint64_t*, uint64_t*> ws = d.words();
        for (uint64_t* i = ws.first; i != ws.second; ++i)
        {
            *i = VByteCodec::decode(mAdapter);
        }
        pItm.value().add1(d);
#endif
    }

    EdgeDecoder(std::istream& pIn)
        : mIn(pIn), mAdapter(pIn)
    {
    }

    bool good() const {
#ifndef GOSS_CODEC_FORCE_VBYTE
        return !mDecoder.empty() || mIn.good();
#else
        return mIn.good();
#endif
    }

private:
    std::istream& mIn;
    InAdapter<encoded_type> mAdapter;
#ifndef GOSS_CODEC_FORCE_VBYTE
    Simple8bDecode<Gossamer::position_type> mDecoder;
#endif
};


#endif // EDGEANDCOUNT_HH
