// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#ifndef INTEGERCODECS_HH
#define INTEGERCODECS_HH

#ifndef UTILS_HH
#include "Utils.hh"
#endif

#ifndef GOSSAMER_HH
#include "Gossamer.hh"
#endif

#ifndef TRIVIALVECTOR_HH
#include "TrivialVector.hh"
#endif

#ifndef BOOST_ASSERT_HPP
#include <boost/assert.hpp>
#define BOOST_ASSERT_HPP
#endif

namespace VByteCodec
{
    template <typename Dest>
    void encode(const uint64_t& pItem, Dest& pDest)
    {
        if (pItem < 0x80)
        {
            pDest.push_back(static_cast<uint8_t>(pItem));
            return;
        }
        uint64_t x = pItem;
        uint64_t b = 64 - Gossamer::count_leading_zeroes(x);
        uint64_t v = b / 8; // number of whole bytes of payload
        uint64_t l = b % 8; // number of bits in the most significant partial byte of payload
        if (v + l + 1 <= 8)
        {
            // We can fit the l bits of the msb in the header byte
            uint8_t z = (x >> (8 * v)) | ~(static_cast<uint8_t>(-1) >> v);
            pDest.push_back(z);
        }
        else
        {
            if (l != 0)
            {
                // There is no room for the l bits of the msb
                // in the header, so we need an extra byte.
                ++v;
            }
            uint8_t z = ~(static_cast<uint8_t>(-1) >> v);
            pDest.push_back(z);
        }
        switch (v)
        {
        case 8:
        {
            uint8_t y = x >> 56;
            pDest.push_back(y);
        }
        case 7:
        {
            uint8_t y = x >> 48;
            pDest.push_back(y);
        }
        case 6:
        {
            uint8_t y = x >> 40;
            pDest.push_back(y);
        }
        case 5:
        {
            uint8_t y = x >> 32;
            pDest.push_back(y);
        }
        case 4:
        {
            uint8_t y = x >> 24;
            pDest.push_back(y);
        }
        case 3:
        {
            uint8_t y = x >> 16;
            pDest.push_back(y);
        }
        case 2:
        {
            uint8_t y = x >> 8;
            pDest.push_back(y);
        }
        case 1:
        {
            uint8_t y = x >> 0;
            pDest.push_back(y);
        }
        case 0:
        {
            break;
        }
        default:
        {
            BOOST_ASSERT(false);
        }
        }
    }

    template <typename Itr>
    uint64_t decode(Itr& pItr, const Itr& pEnd)
    {
        BOOST_ASSERT(pItr != pEnd);

        uint8_t z = *pItr;
        ++pItr;
        if (z < 0x80)
        {
            return z;
        }

        // Count the number of leading 1s by sign-extending,
        // taking bitwise complement and counting leading zeros.
        int64_t x = static_cast<int8_t>(z);
        uint64_t n = Gossamer::count_leading_zeroes(~x);
        uint64_t r = static_cast<uint8_t>(x) & ((~0ULL) >> n);
        n -= 56;
        switch (n)
        {
            case 8:
            {
                BOOST_ASSERT(pItr != pEnd);
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 7:
            {
                BOOST_ASSERT(pItr != pEnd);
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 6:
            {
                BOOST_ASSERT(pItr != pEnd);
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 5:
            {
                BOOST_ASSERT(pItr != pEnd);
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 4:
            {
                BOOST_ASSERT(pItr != pEnd);
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 3:
            {
                BOOST_ASSERT(pItr != pEnd);
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 2:
            {
                BOOST_ASSERT(pItr != pEnd);
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 1:
            {
                BOOST_ASSERT(pItr != pEnd);
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 0:
            {
                break;
            }
            default:
            {
                BOOST_ASSERT(false);
            }
        }
        return r;
    }

    template <typename Itr>
    uint64_t decode(Itr& pItr)
    {
        uint8_t z = *pItr;
        ++pItr;
        if (z < 0x80)
        {
            return z;
        }

        // Count the number of leading 1s by sign-extending,
        // taking bitwise complement and counting leading zeros.
        int64_t x = static_cast<int8_t>(z);
        uint64_t n = Gossamer::count_leading_zeroes(~x);
        uint64_t r = static_cast<uint8_t>(x) & ((~0ULL) >> n);
        n -= 56;
        switch (n)
        {
            case 8:
            {
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 7:
            {
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 6:
            {
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 5:
            {
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 4:
            {
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 3:
            {
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 2:
            {
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 1:
            {
                r = (r << 8) | *pItr;
                ++pItr;
            }
            case 0:
            {
                break;
            }
            default:
            {
                BOOST_ASSERT(false);
            }
        }
        return r;
    }
}

namespace VWord32Codec
{
    inline uint64_t encodingLength(uint64_t pX)
    {
        if (pX < (1ULL << 31))
        {
            return 1;
        }
        if (pX < (1ULL << 62))
        {
            return 2;
        }
        return 3;
    }

    template <typename Vec>
    void encode(uint64_t pX, Vec& pVec)
    {
        if (pX < (1ULL << 31))
        {
            pVec.push_back(static_cast<uint32_t>(pX) << 1);
            return;
        }
        if (pX < (1ULL << 62))
        {
            pVec.push_back((static_cast<uint32_t>(pX >> 31) << 1) | 1);
            pVec.push_back(static_cast<uint32_t>(pX) << 1);
        }
        pVec.push_back((static_cast<uint32_t>(pX >> 62) << 1) | 1);
        pVec.push_back((static_cast<uint32_t>(pX >> 31) << 1) | 1);
        pVec.push_back(static_cast<uint32_t>(pX) << 1);
    }

    template <typename Itr>
    uint64_t decode(Itr& pItr)
    {
        uint32_t w = *pItr;
        ++pItr;
        uint64_t r = w >> 1;
        while (w & 1)
        {
            w = *pItr;
            ++pItr;
            r = (r << 31) | (w >> 1);
        }
        return r;
    }
}
// namespace VWord32Codec

class Simple8bBase
{
public:
    static constexpr uint64_t sSelectorMask = 0xF000000000000000ull;
    static constexpr uint64_t sStorageMask = ~sSelectorMask;
    static constexpr unsigned sStorageBits = 60;
    static constexpr uint64_t s2ToThe60 = 1ull << 60;
    static constexpr uint64_t sContinuationBit = 1ull << 63;
    static constexpr unsigned sUncompressedBufferCapacity = 256;
    static constexpr unsigned sCompressedBufferCapacity = 4;
};


template<typename Item>
struct Simple8bItemTraits
{
};


template<>
struct Simple8bItemTraits<uint64_t>
{
    static bool largeValue(uint64_t pValue)
    {
        return pValue & Simple8bBase::sSelectorMask;
    }

    static uint64_t getWord(uint64_t pValue)
    {
        return pValue & ~Simple8bBase::sSelectorMask;
    }

    static constexpr unsigned maxContinuationEntries() {
        return 1;
    }
};


template<>
struct Simple8bItemTraits<Gossamer::position_type>
{
    static bool largeValue(const Gossamer::position_type& pValue)
    {
        return (pValue & Simple8bBase::sSelectorMask) || !pValue.fitsIn64Bits();
    }

    static uint64_t getWord(const Gossamer::position_type& pValue)
    {
        return pValue.asUInt64() & ~Simple8bBase::sSelectorMask;
    }

    static constexpr unsigned maxContinuationEntries() {
        return Gossamer::position_type::value_type::sBits / 60;
    }
};


class Simple8bEncodeBase : public Simple8bBase
{
protected:
    void resetInputBuffer();
    bool encodeOnce();
    void encodeSelector(uint8_t pSelector);
    void encodeWord(uint64_t pItem);

    template<typename Dest>
    void flushOutput(Dest& pDest)
    {
        if (mOutput.size()) {
            for (auto o : mOutput) {
                pDest.push_back(o);
            }
            mOutput.clear();
        }
    }

    uint64_t mInputPos = 0, mLargestSoFar = 0;
    TrivialVector<uint64_t, sCompressedBufferCapacity> mOutput;
    TrivialVector<uint64_t, sUncompressedBufferCapacity> mInput;
};


template<typename Item>
class Simple8bEncode : public Simple8bEncodeBase
{
    void encodeLargeItem(Item pItem);

public:
    template <typename Dest>
    void encode(const Item& pItem, Dest& pDest)
    {
        if (Simple8bItemTraits<Item>::largeValue(pItem)) {
            flush(pDest);
            encodeLargeItem(pItem);
            flushOutput(pDest);
            return;
        }

        encodeWord(Simple8bItemTraits<Item>::getWord(pItem));
        flushOutput(pDest);
    }

    template <typename Dest>
    void flush(Dest& pDest)
    {
        while (mInputPos < mInput.size()) {
            encodeOnce();
        }
        resetInputBuffer();
        flushOutput(pDest);
    }
};


template<typename Item>
void Simple8bEncode<Item>::encodeLargeItem(Item pItem)
{
    uint64_t continuation[Simple8bItemTraits<Item>::maxContinuationEntries()];
    auto finalWord = Simple8bItemTraits<Item>::getWord(pItem) & sStorageMask;
    for (unsigned i = 0; i < Simple8bItemTraits<Item>::maxContinuationEntries(); ++i) {
        pItem >>= sStorageBits;
        continuation[i] = Simple8bItemTraits<Item>::getWord(pItem) & sStorageMask;
    }
    int i = Simple8bItemTraits<Item>::maxContinuationEntries() - 1;
    while (i >= 0 && !continuation[i]) --i;
    while (i >= 0) {
        mOutput.push_back((uint64_t(15) << sStorageBits) | continuation[i--]);
    }
    mOutput.push_back((uint64_t(14) << sStorageBits) | finalWord);
}


class Simple8bDecodeBase : public Simple8bBase
{
protected:
    uint64_t decodeWord(uint64_t pWord);

    uint64_t mOutputPos = 0;
    TrivialVector<uint64_t, sUncompressedBufferCapacity> mOutput;

public:
    bool empty() const {
        return mOutputPos >= mOutput.size();
    }
};


template<typename Item>
class Simple8bDecode : public Simple8bDecodeBase
{
public:
    template <typename Itr>
    Item decode(Itr& pItr)
    {
        if (mOutputPos < mOutput.size()) {
            return Item(mOutput[mOutputPos++]);
        }

        mOutput.clear();
        uint64_t decword = decodeWord(*pItr++);
        if (decword & sContinuationBit) {
            Item item(decword & sStorageMask);
            do {
                item <<= sStorageBits;
                decword = decodeWord(*pItr++);
                item |= decword & sStorageMask;
            } while (decword & sContinuationBit);
            mOutputPos = 0;
            return item;
        }
        else {
            mOutputPos = 0;
            return Item(decword);
        }
    }
};

#endif // INTEGERCODECS_HH
