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

#if 0
class Simple8Base
{
protected:
    static constexpr uint64_t sSelectorMask = 0xF000000000000000ull;
    static constexpr unsigned sStorageBits = 60;
    static constexpr uint64_t s2ToThe60 = 1ull << 60;
    static constexpr uint64_t sToThe63 = 1ull << 63;
    static constexpr uint64_t sMask60m1 = s2ToThe60 - 1;
    static constexpr unsigned sBufferCapacity = 256;

    static const uint8_t sSelectorBySize[64] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 9, 10, 10, 11, 11, 11, 12,
        12, 12, 12, 12, 12, 12, 13, 13, 13,
        13, 13, 13, 13, 13, 13, 13, 13, 14,
        14, 14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 15, 15, 15, 15
    };
    static constexpr uint8_t sSelectorByCount[256] = {
        15, 14, 13, 12, 11, 10, 9, 8, 7, 7, 6, 6, 5, 5, 5, 4,
        4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    };
};

class Simpl8Encode : public Simple8Base
{
public:
    template <typename Dest>
    void encode(const uint64_t& pItem, Dest& pDest)
    {
        if (pItem >= s2ToThe60) {
            // Variable word code. First, flush the buffer, because this starts a code word.
            flushWholeBuffer(pDest);

            // Variable word encode.
            mData.push_back((pItem >> 60) | s2toThe63);
            mData.push_back(pItem & sMask60m1);

            // Encode the high word.
            encode(0xF);
            return;
        }

        unsigned itemSize = 64 - Gossamer::count_leading_zeroes(pItem);
        while (itemSize * (mData.size() + 1) > sStorageBits) {
            writeSelector(sSelectorBySize[mLargestSoFar], pDest);
            resetBuffer();
        }
        mData.push_back(pItem);
        mLargestSoFar = std::max<unsigned>(mLargestSoFar, itemSize);
        if (mData.size() == sBufferCapacity) {
            flushWholeBuffer
        }
    }

private:
    unsigned mDataPos = 0, mLargestSoFar = 0;
    TrivialVector<uint64_t, sBufferCapacity> mData;

    void resetBuffer()
    {
        auto newSize = mData.size() - mDataPos;
        std::memmove(&mData[0], &mData[mDataPos], newSize * sizeof(uint64_t));
        mData.resize(newSize);

        auto newItemSize = 0;
        for (unsigned i = 0; i < newSize; ++i) {
            newItemSize = std::max<unsigned>(newItemSize, 64 - Gossamer::count_leading_zeroes(mData[i]));
        }
        mLargestSoFar = newItemSize;
    }

    template<typename Dest>
    void writeSelector(uint8_t selector, Dest& pDest)
    {
        switch (selector) {
        case 0:
        {
            uint64_t zeroes;
            for (zeroes = 0; zeroes + mDataPos < mData.size(); ++zeroes) {
                if (mData[i]) {
                    break;
                }
            }
            uint64_t word = zeroes;
            pDest.push_back(word);
            pDataPos += zeroes;
            break;
        }
        case 1:
        {
            uint64_t word = (0x1 << sStorageBits)
                | (mData[mDataPos] << 59)
                | (mData[mDataPos + 1] << 58)
                | (mData[mDataPos + 2] << 57)
                | (mData[mDataPos + 3] << 56)
                | (mData[mDataPos + 4] << 55)
                | (mData[mDataPos + 5] << 54)
                | (mData[mDataPos + 6] << 53)
                | (mData[mDataPos + 7] << 52)
                | (mData[mDataPos + 8] << 51)
                | (mData[mDataPos + 9] << 50)
                | (mData[mDataPos + 10] << 49)
                | (mData[mDataPos + 11] << 48)
                | (mData[mDataPos + 12] << 47)
                | (mData[mDataPos + 13] << 46)
                | (mData[mDataPos + 14] << 45)
                | (mData[mDataPos + 15] << 44)
                | (mData[mDataPos + 16] << 43)
                | (mData[mDataPos + 17] << 42)
                | (mData[mDataPos + 18] << 41)
                | (mData[mDataPos + 19] << 40)
                | (mData[mDataPos + 20] << 39)
                | (mData[mDataPos + 21] << 38)
                | (mData[mDataPos + 22] << 37)
                | (mData[mDataPos + 23] << 36)
                | (mData[mDataPos + 24] << 35)
                | (mData[mDataPos + 25] << 34)
                | (mData[mDataPos + 26] << 33)
                | (mData[mDataPos + 27] << 32)
                | (mData[mDataPos + 28] << 31)
                | (mData[mDataPos + 29] << 30)
                | (mData[mDataPos + 30] << 29)
                | (mData[mDataPos + 31] << 28)
                | (mData[mDataPos + 32] << 27)
                | (mData[mDataPos + 33] << 26)
                | (mData[mDataPos + 34] << 25)
                | (mData[mDataPos + 35] << 24)
                | (mData[mDataPos + 36] << 23)
                | (mData[mDataPos + 37] << 22)
                | (mData[mDataPos + 38] << 21)
                | (mData[mDataPos + 39] << 20)
                | (mData[mDataPos + 40] << 19)
                | (mData[mDataPos + 41] << 18)
                | (mData[mDataPos + 42] << 17)
                | (mData[mDataPos + 43] << 16)
                | (mData[mDataPos + 44] << 15)
                | (mData[mDataPos + 45] << 14)
                | (mData[mDataPos + 46] << 13)
                | (mData[mDataPos + 47] << 12)
                | (mData[mDataPos + 48] << 11)
                | (mData[mDataPos + 49] << 10)
                | (mData[mDataPos + 50] << 9)
                | (mData[mDataPos + 51] << 8)
                | (mData[mDataPos + 52] << 7)
                | (mData[mDataPos + 53] << 6)
                | (mData[mDataPos + 54] << 5)
                | (mData[mDataPos + 55] << 4)
                | (mData[mDataPos + 56] << 3)
                | (mData[mDataPos + 57] << 2)
                | (mData[mDataPos + 58] << 1)
                | mData[mDataPos + 59];
            pDest.push_back(word);
            mDataPos += 60;
            break;
        }
        case 2:
        {
            uint64_t word = (0x2 << sStorageBits)
                | (mData[mDataPos] << 58)
                | (mData[mDataPos + 1] << 56)
                | (mData[mDataPos + 2] << 54)
                | (mData[mDataPos + 3] << 52)
                | (mData[mDataPos + 4] << 50)
                | (mData[mDataPos + 5] << 48)
                | (mData[mDataPos + 6] << 46)
                | (mData[mDataPos + 7] << 44)
                | (mData[mDataPos + 8] << 42)
                | (mData[mDataPos + 9] << 40)
                | (mData[mDataPos + 10] << 38)
                | (mData[mDataPos + 11] << 36)
                | (mData[mDataPos + 12] << 34)
                | (mData[mDataPos + 13] << 32)
                | (mData[mDataPos + 14] << 30)
                | (mData[mDataPos + 15] << 28)
                | (mData[mDataPos + 16] << 26)
                | (mData[mDataPos + 17] << 24)
                | (mData[mDataPos + 18] << 22)
                | (mData[mDataPos + 19] << 20)
                | (mData[mDataPos + 20] << 18)
                | (mData[mDataPos + 21] << 16)
                | (mData[mDataPos + 22] << 14)
                | (mData[mDataPos + 23] << 12)
                | (mData[mDataPos + 24] << 10)
                | (mData[mDataPos + 25] << 8)
                | (mData[mDataPos + 26] << 6)
                | (mData[mDataPos + 27] << 4)
                | (mData[mDataPos + 28] << 2)
                | mData[mDataPos + 29];
            pDest.push_back(word);
            mDataPos += 30;
            break;
        }
        case 3:
        {
            uint64_t word = (0x3 << sStorageBits)
                | (mData[mDataPos] << 57)
                | (mData[mDataPos + 1] << 54)
                | (mData[mDataPos + 2] << 51)
                | (mData[mDataPos + 3] << 48)
                | (mData[mDataPos + 4] << 45)
                | (mData[mDataPos + 5] << 42)
                | (mData[mDataPos + 6] << 39)
                | (mData[mDataPos + 7] << 36)
                | (mData[mDataPos + 8] << 33)
                | (mData[mDataPos + 9] << 30)
                | (mData[mDataPos + 10] << 27)
                | (mData[mDataPos + 11] << 24)
                | (mData[mDataPos + 12] << 21)
                | (mData[mDataPos + 13] << 18)
                | (mData[mDataPos + 14] << 15)
                | (mData[mDataPos + 15] << 12)
                | (mData[mDataPos + 16] << 9)
                | (mData[mDataPos + 17] << 6)
                | (mData[mDataPos + 18] << 3)
                | mData[mDataPos + 19];
            pDest.push_back(word);
            mDataPos += 20;
            break;
        }
        case 4:
        {
            uint64_t word = (0x4 << sStorageBits)
                | (mData[mDataPos] << 56)
                | (mData[mDataPos + 1] << 52)
                | (mData[mDataPos + 2] << 48)
                | (mData[mDataPos + 3] << 44)
                | (mData[mDataPos + 4] << 40)
                | (mData[mDataPos + 5] << 36)
                | (mData[mDataPos + 6] << 32)
                | (mData[mDataPos + 7] << 28)
                | (mData[mDataPos + 8] << 24)
                | (mData[mDataPos + 9] << 20)
                | (mData[mDataPos + 10] << 16)
                | (mData[mDataPos + 11] << 12)
                | (mData[mDataPos + 12] << 8)
                | (mData[mDataPos + 13] << 4)
                | mData[mDataPos + 14];
            pDest.push_back(word);
            mDataPos += 15;
            break;
        }
        case 5:
        {
            uint64_t word = (0x5 << sStorageBits)
                | (mData[mDataPos] << 55)
                | (mData[mDataPos + 1] << 50)
                | (mData[mDataPos + 2] << 45)
                | (mData[mDataPos + 3] << 40)
                | (mData[mDataPos + 4] << 35)
                | (mData[mDataPos + 5] << 30)
                | (mData[mDataPos + 6] << 25)
                | (mData[mDataPos + 7] << 20)
                | (mData[mDataPos + 8] << 15)
                | (mData[mDataPos + 9] << 10)
                | (mData[mDataPos + 10] << 5)
                | mData[mDataPos + 11];
            pDest.push_back(word);
            mDataPos += 12;
            break;
        }
        case 6:
        {
            uint64_t word = (0x6 << sStorageBits)
                | (mData[mDataPos] << 52)
                | (mData[mDataPos + 1] << 48)
                | (mData[mDataPos + 2] << 42)
                | (mData[mDataPos + 3] << 36)
                | (mData[mDataPos + 4] << 30)
                | (mData[mDataPos + 5] << 24)
                | (mData[mDataPos + 6] << 18)
                | (mData[mDataPos + 7] << 12)
                | (mData[mDataPos + 8] << 6)
                | mData[mDataPos + 9];
            pDest.push_back(word);
            mDataPos += 10;
            break;
        }
        case 7:
        {
            uint64_t word = (0x7 << sStorageBits)
                | (mData[mDataPos] << 49)
                | (mData[mDataPos + 1] << 42)
                | (mData[mDataPos + 2] << 35)
                | (mData[mDataPos + 3] << 28)
                | (mData[mDataPos + 4] << 21)
                | (mData[mDataPos + 5] << 14)
                | (mData[mDataPos + 6] << 7)
                | mData[mDataPos + 7];
            pDest.push_back(word);
            mDataPos += 8;
            break;
        }
        case 8:
        {
            uint64_t word = (0x8 << sStorageBits)
                | (mData[mDataPos] << 48)
                | (mData[mDataPos + 1] << 40)
                | (mData[mDataPos + 2] << 32)
                | (mData[mDataPos + 3] << 24)
                | (mData[mDataPos + 4] << 16)
                | (mData[mDataPos + 5] << 8)
                | mData[mDataPos + 6];
            pDest.push_back(word);
            mDataPos += 7;
            break;
        }
        case 9:
        {
            uint64_t word = (0x9 << sStorageBits)
                | (mData[mDataPos] << 50)
                | (mData[mDataPos + 1] << 40)
                | (mData[mDataPos + 2] << 30)
                | (mData[mDataPos + 3] << 20)
                | (mData[mDataPos + 4] << 10)
                | mData[mDataPos + 5];
            pDest.push_back(word);
            mDataPos += 6;
            break;
        }
        case 10:
        {
            uint64_t word = (0xA << sStorageBits)
                | (mData[mDataPos] << 48)
                | (mData[mDataPos + 1] << 36)
                | (mData[mDataPos + 2] << 24)
                | (mData[mDataPos + 3] << 12)
                | mData[mDataPos + 4];
            pDest.push_back(word);
            mDataPos += 5;
            break;
        }
        case 11:
        {
            uint64_t word = (0xB << sStorageBits) | (mData[mDataPos] << 45) | (mData[mDataPos + 1] << 30) | (mData[mDataPos + 2] << 15) | mData[mDataPos + 3];
            pDest.push_back(word);
            mDataPos += 4;
            break;
        }
        case 12:
        {
            uint64_t word = (0xC << sSelectorShift) | (mData[mDataPos] << 40) | (mData[mDataPos + 1] << 20) | mData[mDataPos + 2];
            pDest.push_back(word);
            mDataPos += 3;
            break;
        }
        case 13:
        {
            uint64_t word = (0xD << sStorageBits) | (mData[mDataPos] << 30) | mData[mDataPos + 1];
            pDest.push_back(word);
            mDataPos += 2;
            break;
        }
        case 14:
        {
            uint64_t word = (0xE << sStorageBits) | (mData[mDataPos] & ~sSelectorMask);
            pDest.push_back(word);
            ++mDataPos;
            break;
        }
        case 15:
        {
            uint64_t word = (0xF << sStorageBits) | (mData[mDataPos] & ~sSelectorMask);
            pDest.push_back(word);
            ++mDataPos;
            break;
        }
        }
   }
};

#endif

#endif // INTEGERCODECS_HH
