// Copyright (c) 2023 Andrew J. Bromage.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#include "IntegerCodecs.hh"
#include "Utils.hh"

namespace {
	const alignas(16) uint8_t sSimple8bCountTable[16] = {
        255, 60, 30, 20,
        15, 12, 10, 8,
        7, 6, 5, 4,
        3, 2, 1, 0
	};

    const alignas(16) uint8_t sSimple8bCountTableSigned[16] = {
        255 - 128, 60 - 128, 30 - 128, 20 - 128,
        15 - 128, 12 - 128, 10 - 128, 8 - 128,
        7 - 128, 6 - 128, 5 - 128, 4 - 128,
        3 - 128, 2 - 128, 1 - 128, 0 - 128
    };

#if 0
    const alignas(16) uint8_t sSimple8bBitsTable[16] = {
        0, 1, 2, 3,
        4, 5, 6, 7,
        8, 10, 12, 15,
        20, 30, 60, 255
    };

    inline uint8_t findSelectorByBits(uint8_t bits)
    {
        uint8_t i = 0;
        i += (bits >= sSimple8bBitsTable[i + (1 << 3)]) << 3;
        i += (bits >= sSimple8bBitsTable[i + (1 << 2)]) << 2;
        i += (bits >= sSimple8bBitsTable[i + (1 << 1)]) << 1;
        i += (bits >= sSimple8bBitsTable[i + (1 << 0)]) << 0;
        return i;
    }
#endif

    inline uint8_t findSelectorByCount(uint8_t count)
    {
#ifdef GOSS_HAVE_SIMD128
        using namespace Gossamer::simd;
        auto countTable = load_aligned_128(sSimple8bCountTableSigned);
        auto index = load1_8x8_128(count - 128);
        auto gtcount = compare_gt8_128(countTable, index);
        auto mask = movemask_8_128(gtcount);
        return Gossamer::find_first_set(~mask) - 1;
#else
        uint8_t i = 0;
        i += (count >= sSimple8bCountTable[15 - (i + (1 << 3))]) << 3;
        i += (count >= sSimple8bCountTable[15 - (i + (1 << 2))]) << 2;
        i += (count >= sSimple8bCountTable[15 - (i + (1 << 1))]) << 1;
        i += (count >= sSimple8bCountTable[15 - (i + (1 << 0))]) << 0;
        return 15 - i;
#endif
    }

}


uint64_t
Simple8bDecodeBase::decodeWord(uint64_t pWord)
{
    uint64_t retval = 0;
    switch (pWord >> sStorageBits) {
    case 0:
    {
        for (uint64_t zeroes = 1; zeroes < (pWord & sStorageMask); ++zeroes) {
            mOutput.push_back(0);
        }
        break;
    }
    case 1:
    {
        retval = (pWord >> 59) & 1;
        mOutput.push_back((pWord >> 58) & 1);
        mOutput.push_back((pWord >> 57) & 1);
        mOutput.push_back((pWord >> 56) & 1);
        mOutput.push_back((pWord >> 55) & 1);
        mOutput.push_back((pWord >> 54) & 1);
        mOutput.push_back((pWord >> 53) & 1);
        mOutput.push_back((pWord >> 52) & 1);
        mOutput.push_back((pWord >> 51) & 1);
        mOutput.push_back((pWord >> 50) & 1);
        mOutput.push_back((pWord >> 49) & 1);
        mOutput.push_back((pWord >> 48) & 1);
        mOutput.push_back((pWord >> 47) & 1);
        mOutput.push_back((pWord >> 46) & 1);
        mOutput.push_back((pWord >> 45) & 1);
        mOutput.push_back((pWord >> 44) & 1);
        mOutput.push_back((pWord >> 43) & 1);
        mOutput.push_back((pWord >> 42) & 1);
        mOutput.push_back((pWord >> 41) & 1);
        mOutput.push_back((pWord >> 40) & 1);
        mOutput.push_back((pWord >> 39) & 1);
        mOutput.push_back((pWord >> 38) & 1);
        mOutput.push_back((pWord >> 37) & 1);
        mOutput.push_back((pWord >> 36) & 1);
        mOutput.push_back((pWord >> 35) & 1);
        mOutput.push_back((pWord >> 34) & 1);
        mOutput.push_back((pWord >> 33) & 1);
        mOutput.push_back((pWord >> 32) & 1);
        mOutput.push_back((pWord >> 31) & 1);
        mOutput.push_back((pWord >> 30) & 1);
        mOutput.push_back((pWord >> 29) & 1);
        mOutput.push_back((pWord >> 28) & 1);
        mOutput.push_back((pWord >> 27) & 1);
        mOutput.push_back((pWord >> 26) & 1);
        mOutput.push_back((pWord >> 25) & 1);
        mOutput.push_back((pWord >> 24) & 1);
        mOutput.push_back((pWord >> 23) & 1);
        mOutput.push_back((pWord >> 22) & 1);
        mOutput.push_back((pWord >> 21) & 1);
        mOutput.push_back((pWord >> 20) & 1);
        mOutput.push_back((pWord >> 19) & 1);
        mOutput.push_back((pWord >> 18) & 1);
        mOutput.push_back((pWord >> 17) & 1);
        mOutput.push_back((pWord >> 16) & 1);
        mOutput.push_back((pWord >> 15) & 1);
        mOutput.push_back((pWord >> 14) & 1);
        mOutput.push_back((pWord >> 13) & 1);
        mOutput.push_back((pWord >> 12) & 1);
        mOutput.push_back((pWord >> 11) & 1);
        mOutput.push_back((pWord >> 10) & 1);
        mOutput.push_back((pWord >> 9) & 1);
        mOutput.push_back((pWord >> 8) & 1);
        mOutput.push_back((pWord >> 7) & 1);
        mOutput.push_back((pWord >> 6) & 1);
        mOutput.push_back((pWord >> 5) & 1);
        mOutput.push_back((pWord >> 4) & 1);
        mOutput.push_back((pWord >> 3) & 1);
        mOutput.push_back((pWord >> 2) & 1);
        mOutput.push_back((pWord >> 1) & 1);
        mOutput.push_back((pWord >> 0) & 1);
        break;
    }
    case 2:
    {
        retval = (pWord >> 58) & 3;
        mOutput.push_back((pWord >> 56) & 3);
        mOutput.push_back((pWord >> 54) & 3);
        mOutput.push_back((pWord >> 52) & 3);
        mOutput.push_back((pWord >> 50) & 3);
        mOutput.push_back((pWord >> 48) & 3);
        mOutput.push_back((pWord >> 46) & 3);
        mOutput.push_back((pWord >> 44) & 3);
        mOutput.push_back((pWord >> 42) & 3);
        mOutput.push_back((pWord >> 40) & 3);
        mOutput.push_back((pWord >> 38) & 3);
        mOutput.push_back((pWord >> 36) & 3);
        mOutput.push_back((pWord >> 34) & 3);
        mOutput.push_back((pWord >> 32) & 3);
        mOutput.push_back((pWord >> 30) & 3);
        mOutput.push_back((pWord >> 28) & 3);
        mOutput.push_back((pWord >> 26) & 3);
        mOutput.push_back((pWord >> 24) & 3);
        mOutput.push_back((pWord >> 22) & 3);
        mOutput.push_back((pWord >> 20) & 3);
        mOutput.push_back((pWord >> 18) & 3);
        mOutput.push_back((pWord >> 16) & 3);
        mOutput.push_back((pWord >> 14) & 3);
        mOutput.push_back((pWord >> 12) & 3);
        mOutput.push_back((pWord >> 10) & 3);
        mOutput.push_back((pWord >> 8) & 3);
        mOutput.push_back((pWord >> 6) & 3);
        mOutput.push_back((pWord >> 4) & 3);
        mOutput.push_back((pWord >> 2) & 3);
        mOutput.push_back((pWord >> 0) & 3);
        break;
    }
    case 3:
    {
        retval = (pWord >> 57) & 7;
        mOutput.push_back((pWord >> 54) & 7);
        mOutput.push_back((pWord >> 51) & 7);
        mOutput.push_back((pWord >> 48) & 7);
        mOutput.push_back((pWord >> 45) & 7);
        mOutput.push_back((pWord >> 42) & 7);
        mOutput.push_back((pWord >> 39) & 7);
        mOutput.push_back((pWord >> 36) & 7);
        mOutput.push_back((pWord >> 33) & 7);
        mOutput.push_back((pWord >> 30) & 7);
        mOutput.push_back((pWord >> 27) & 7);
        mOutput.push_back((pWord >> 24) & 7);
        mOutput.push_back((pWord >> 21) & 7);
        mOutput.push_back((pWord >> 18) & 7);
        mOutput.push_back((pWord >> 15) & 7);
        mOutput.push_back((pWord >> 12) & 7);
        mOutput.push_back((pWord >> 9) & 7);
        mOutput.push_back((pWord >> 6) & 7);
        mOutput.push_back((pWord >> 3) & 7);
        mOutput.push_back((pWord >> 0) & 7);
        break;
    }
    case 4:
    {
        retval = (pWord >> 56) & 0xF;
        mOutput.push_back((pWord >> 52) & 0xF);
        mOutput.push_back((pWord >> 48) & 0xF);
        mOutput.push_back((pWord >> 44) & 0xF);
        mOutput.push_back((pWord >> 40) & 0xF);
        mOutput.push_back((pWord >> 36) & 0xF);
        mOutput.push_back((pWord >> 32) & 0xF);
        mOutput.push_back((pWord >> 28) & 0xF);
        mOutput.push_back((pWord >> 24) & 0xF);
        mOutput.push_back((pWord >> 20) & 0xF);
        mOutput.push_back((pWord >> 16) & 0xF);
        mOutput.push_back((pWord >> 12) & 0xF);
        mOutput.push_back((pWord >> 8) & 0xF);
        mOutput.push_back((pWord >> 4) & 0xF);
        mOutput.push_back((pWord >> 0) & 0xF);
        break;
    }
    case 5:
    {
        retval = (pWord >> 55) & 0x1F;
        mOutput.push_back((pWord >> 50) & 0x1F);
        mOutput.push_back((pWord >> 45) & 0x1F);
        mOutput.push_back((pWord >> 40) & 0x1F);
        mOutput.push_back((pWord >> 35) & 0x1F);
        mOutput.push_back((pWord >> 30) & 0x1F);
        mOutput.push_back((pWord >> 25) & 0x1F);
        mOutput.push_back((pWord >> 20) & 0x1F);
        mOutput.push_back((pWord >> 15) & 0x1F);
        mOutput.push_back((pWord >> 10) & 0x1F);
        mOutput.push_back((pWord >> 5) & 0x1F);
        mOutput.push_back((pWord >> 0) & 0x1F);
        break;
    }
    case 6:
    {
        retval = (pWord >> 54) & 0x3F;
        mOutput.push_back((pWord >> 48) & 0x3F);
        mOutput.push_back((pWord >> 42) & 0x3F);
        mOutput.push_back((pWord >> 36) & 0x3F);
        mOutput.push_back((pWord >> 30) & 0x3F);
        mOutput.push_back((pWord >> 24) & 0x3F);
        mOutput.push_back((pWord >> 18) & 0x3F);
        mOutput.push_back((pWord >> 12) & 0x3F);
        mOutput.push_back((pWord >> 6) & 0x3F);
        mOutput.push_back((pWord >> 0) & 0x3F);
        break;
    }
    case 7:
    {
        retval = (pWord >> 49) & 0x7F;
        mOutput.push_back((pWord >> 42) & 0x7F);
        mOutput.push_back((pWord >> 35) & 0x7F);
        mOutput.push_back((pWord >> 28) & 0x7F);
        mOutput.push_back((pWord >> 21) & 0x7F);
        mOutput.push_back((pWord >> 14) & 0x7F);
        mOutput.push_back((pWord >> 7) & 0x7F);
        mOutput.push_back((pWord >> 0) & 0x7F);
        break;
    }
    case 8:
    {
        retval = (pWord >> 48) & 0xFF;
        mOutput.push_back((pWord >> 40) & 0xFF);
        mOutput.push_back((pWord >> 32) & 0xFF);
        mOutput.push_back((pWord >> 24) & 0xFF);
        mOutput.push_back((pWord >> 16) & 0xFF);
        mOutput.push_back((pWord >> 8) & 0xFF);
        mOutput.push_back((pWord >> 0) & 0xFF);
        break;
    }
    case 9:
    {
        retval = (pWord >> 50) & 0x3FF;
        mOutput.push_back((pWord >> 40) & 0x3FF);
        mOutput.push_back((pWord >> 30) & 0x3FF);
        mOutput.push_back((pWord >> 20) & 0x3FF);
        mOutput.push_back((pWord >> 10) & 0x3FF);
        mOutput.push_back((pWord >> 0) & 0x3FF);
        break;
    }
    case 10:
    {
        retval = (pWord >> 48) & 0xFFF;
        mOutput.push_back((pWord >> 36) & 0xFFF);
        mOutput.push_back((pWord >> 24) & 0xFFF);
        mOutput.push_back((pWord >> 12) & 0xFFF);
        mOutput.push_back((pWord >> 0) & 0xFFF);
        break;
    }
    case 11:
    {
        retval = (pWord >> 45) & 0x7FFF;
        mOutput.push_back((pWord >> 30) & 0x7FFF);
        mOutput.push_back((pWord >> 15) & 0x7FFF);
        mOutput.push_back((pWord >> 0) & 0x7FFF);
        break;
    }
    case 12:
    {
        retval = (pWord >> 40) & 0xFFFFF;
        mOutput.push_back((pWord >> 20) & 0xFFFFF);
        mOutput.push_back((pWord >> 0) & 0xFFFFF);
        break;
    }
    case 13:
    {
        retval = (pWord >> 30) & 0x3FFFFFFF;
        mOutput.push_back((pWord >> 0) & 0x3FFFFFFF);
        break;
    }
    case 14:
    {
        retval = pWord & sStorageMask;
        break;
    }
    case 15:
    {
        retval = (pWord & sStorageMask) | sContinuationBit;
        break;
    }
    }
    return retval;
}


void
Simple8bEncodeBase::encodeSelector(uint8_t selector)
{
    switch (selector) {
    case 0:
    {
        uint64_t zeroes;
        for (zeroes = 0; zeroes + mInputPos < mInput.size(); ++zeroes) {
            if (mInput[zeroes + mInputPos]) {
                break;
            }
        }
        uint64_t word = zeroes;
        mOutput.push_back(word);
        mInputPos += zeroes;
        break;
    }
    case 1:
    {
        uint64_t word = (uint64_t(1) << sStorageBits)
            | (mInput[mInputPos] << 59)
            | (mInput[mInputPos + 1] << 58)
            | (mInput[mInputPos + 2] << 57)
            | (mInput[mInputPos + 3] << 56)
            | (mInput[mInputPos + 4] << 55)
            | (mInput[mInputPos + 5] << 54)
            | (mInput[mInputPos + 6] << 53)
            | (mInput[mInputPos + 7] << 52)
            | (mInput[mInputPos + 8] << 51)
            | (mInput[mInputPos + 9] << 50)
            | (mInput[mInputPos + 10] << 49)
            | (mInput[mInputPos + 11] << 48)
            | (mInput[mInputPos + 12] << 47)
            | (mInput[mInputPos + 13] << 46)
            | (mInput[mInputPos + 14] << 45)
            | (mInput[mInputPos + 15] << 44)
            | (mInput[mInputPos + 16] << 43)
            | (mInput[mInputPos + 17] << 42)
            | (mInput[mInputPos + 18] << 41)
            | (mInput[mInputPos + 19] << 40)
            | (mInput[mInputPos + 20] << 39)
            | (mInput[mInputPos + 21] << 38)
            | (mInput[mInputPos + 22] << 37)
            | (mInput[mInputPos + 23] << 36)
            | (mInput[mInputPos + 24] << 35)
            | (mInput[mInputPos + 25] << 34)
            | (mInput[mInputPos + 26] << 33)
            | (mInput[mInputPos + 27] << 32)
            | (mInput[mInputPos + 28] << 31)
            | (mInput[mInputPos + 29] << 30)
            | (mInput[mInputPos + 30] << 29)
            | (mInput[mInputPos + 31] << 28)
            | (mInput[mInputPos + 32] << 27)
            | (mInput[mInputPos + 33] << 26)
            | (mInput[mInputPos + 34] << 25)
            | (mInput[mInputPos + 35] << 24)
            | (mInput[mInputPos + 36] << 23)
            | (mInput[mInputPos + 37] << 22)
            | (mInput[mInputPos + 38] << 21)
            | (mInput[mInputPos + 39] << 20)
            | (mInput[mInputPos + 40] << 19)
            | (mInput[mInputPos + 41] << 18)
            | (mInput[mInputPos + 42] << 17)
            | (mInput[mInputPos + 43] << 16)
            | (mInput[mInputPos + 44] << 15)
            | (mInput[mInputPos + 45] << 14)
            | (mInput[mInputPos + 46] << 13)
            | (mInput[mInputPos + 47] << 12)
            | (mInput[mInputPos + 48] << 11)
            | (mInput[mInputPos + 49] << 10)
            | (mInput[mInputPos + 50] << 9)
            | (mInput[mInputPos + 51] << 8)
            | (mInput[mInputPos + 52] << 7)
            | (mInput[mInputPos + 53] << 6)
            | (mInput[mInputPos + 54] << 5)
            | (mInput[mInputPos + 55] << 4)
            | (mInput[mInputPos + 56] << 3)
            | (mInput[mInputPos + 57] << 2)
            | (mInput[mInputPos + 58] << 1)
            | mInput[mInputPos + 59];
        mOutput.push_back(word);
        mInputPos += 60;
        break;
    }
    case 2:
    {
        uint64_t word = (uint64_t(2) << sStorageBits)
            | (mInput[mInputPos] << 58)
            | (mInput[mInputPos + 1] << 56)
            | (mInput[mInputPos + 2] << 54)
            | (mInput[mInputPos + 3] << 52)
            | (mInput[mInputPos + 4] << 50)
            | (mInput[mInputPos + 5] << 48)
            | (mInput[mInputPos + 6] << 46)
            | (mInput[mInputPos + 7] << 44)
            | (mInput[mInputPos + 8] << 42)
            | (mInput[mInputPos + 9] << 40)
            | (mInput[mInputPos + 10] << 38)
            | (mInput[mInputPos + 11] << 36)
            | (mInput[mInputPos + 12] << 34)
            | (mInput[mInputPos + 13] << 32)
            | (mInput[mInputPos + 14] << 30)
            | (mInput[mInputPos + 15] << 28)
            | (mInput[mInputPos + 16] << 26)
            | (mInput[mInputPos + 17] << 24)
            | (mInput[mInputPos + 18] << 22)
            | (mInput[mInputPos + 19] << 20)
            | (mInput[mInputPos + 20] << 18)
            | (mInput[mInputPos + 21] << 16)
            | (mInput[mInputPos + 22] << 14)
            | (mInput[mInputPos + 23] << 12)
            | (mInput[mInputPos + 24] << 10)
            | (mInput[mInputPos + 25] << 8)
            | (mInput[mInputPos + 26] << 6)
            | (mInput[mInputPos + 27] << 4)
            | (mInput[mInputPos + 28] << 2)
            | mInput[mInputPos + 29];
        mOutput.push_back(word);
        mInputPos += 30;
        break;
    }
    case 3:
    {
        uint64_t word = (uint64_t(3) << sStorageBits)
            | (mInput[mInputPos] << 57)
            | (mInput[mInputPos + 1] << 54)
            | (mInput[mInputPos + 2] << 51)
            | (mInput[mInputPos + 3] << 48)
            | (mInput[mInputPos + 4] << 45)
            | (mInput[mInputPos + 5] << 42)
            | (mInput[mInputPos + 6] << 39)
            | (mInput[mInputPos + 7] << 36)
            | (mInput[mInputPos + 8] << 33)
            | (mInput[mInputPos + 9] << 30)
            | (mInput[mInputPos + 10] << 27)
            | (mInput[mInputPos + 11] << 24)
            | (mInput[mInputPos + 12] << 21)
            | (mInput[mInputPos + 13] << 18)
            | (mInput[mInputPos + 14] << 15)
            | (mInput[mInputPos + 15] << 12)
            | (mInput[mInputPos + 16] << 9)
            | (mInput[mInputPos + 17] << 6)
            | (mInput[mInputPos + 18] << 3)
            | mInput[mInputPos + 19];
        mOutput.push_back(word);
        mInputPos += 20;
        break;
    }
    case 4:
    {
        uint64_t word = (uint64_t(4) << sStorageBits)
            | (mInput[mInputPos] << 56)
            | (mInput[mInputPos + 1] << 52)
            | (mInput[mInputPos + 2] << 48)
            | (mInput[mInputPos + 3] << 44)
            | (mInput[mInputPos + 4] << 40)
            | (mInput[mInputPos + 5] << 36)
            | (mInput[mInputPos + 6] << 32)
            | (mInput[mInputPos + 7] << 28)
            | (mInput[mInputPos + 8] << 24)
            | (mInput[mInputPos + 9] << 20)
            | (mInput[mInputPos + 10] << 16)
            | (mInput[mInputPos + 11] << 12)
            | (mInput[mInputPos + 12] << 8)
            | (mInput[mInputPos + 13] << 4)
            | mInput[mInputPos + 14];
        mOutput.push_back(word);
        mInputPos += 15;
        break;
    }
    case 5:
    {
        uint64_t word = (uint64_t(5) << sStorageBits)
            | (mInput[mInputPos] << 55)
            | (mInput[mInputPos + 1] << 50)
            | (mInput[mInputPos + 2] << 45)
            | (mInput[mInputPos + 3] << 40)
            | (mInput[mInputPos + 4] << 35)
            | (mInput[mInputPos + 5] << 30)
            | (mInput[mInputPos + 6] << 25)
            | (mInput[mInputPos + 7] << 20)
            | (mInput[mInputPos + 8] << 15)
            | (mInput[mInputPos + 9] << 10)
            | (mInput[mInputPos + 10] << 5)
            | mInput[mInputPos + 11];
        mOutput.push_back(word);
        mInputPos += 12;
        break;
    }
    case 6:
    {
        uint64_t word = (uint64_t(6) << sStorageBits)
            | (mInput[mInputPos] << 54)
            | (mInput[mInputPos + 1] << 48)
            | (mInput[mInputPos + 2] << 42)
            | (mInput[mInputPos + 3] << 36)
            | (mInput[mInputPos + 4] << 30)
            | (mInput[mInputPos + 5] << 24)
            | (mInput[mInputPos + 6] << 18)
            | (mInput[mInputPos + 7] << 12)
            | (mInput[mInputPos + 8] << 6)
            | mInput[mInputPos + 9];
        mOutput.push_back(word);
        mInputPos += 10;
        break;
    }
    case 7:
    {
        uint64_t word = (uint64_t(7) << sStorageBits)
            | (mInput[mInputPos] << 49)
            | (mInput[mInputPos + 1] << 42)
            | (mInput[mInputPos + 2] << 35)
            | (mInput[mInputPos + 3] << 28)
            | (mInput[mInputPos + 4] << 21)
            | (mInput[mInputPos + 5] << 14)
            | (mInput[mInputPos + 6] << 7)
            | mInput[mInputPos + 7];
        mOutput.push_back(word);
        mInputPos += 8;
        break;
    }
    case 8:
    {
        uint64_t word = (uint64_t(8) << sStorageBits)
            | (mInput[mInputPos] << 48)
            | (mInput[mInputPos + 1] << 40)
            | (mInput[mInputPos + 2] << 32)
            | (mInput[mInputPos + 3] << 24)
            | (mInput[mInputPos + 4] << 16)
            | (mInput[mInputPos + 5] << 8)
            | mInput[mInputPos + 6];
        mOutput.push_back(word);
        mInputPos += 7;
        break;
    }
    case 9:
    {
        uint64_t word = (uint64_t(9) << sStorageBits)
            | (mInput[mInputPos] << 50)
            | (mInput[mInputPos + 1] << 40)
            | (mInput[mInputPos + 2] << 30)
            | (mInput[mInputPos + 3] << 20)
            | (mInput[mInputPos + 4] << 10)
            | mInput[mInputPos + 5];
        mOutput.push_back(word);
        mInputPos += 6;
        break;
    }
    case 10:
    {
        uint64_t word = (uint64_t(10) << sStorageBits)
            | (mInput[mInputPos] << 48)
            | (mInput[mInputPos + 1] << 36)
            | (mInput[mInputPos + 2] << 24)
            | (mInput[mInputPos + 3] << 12)
            | mInput[mInputPos + 4];
        mOutput.push_back(word);
        mInputPos += 5;
        break;
    }
    case 11:
    {
        uint64_t word = (uint64_t(11) << sStorageBits)
            | (mInput[mInputPos] << 45)
            | (mInput[mInputPos + 1] << 30)
            | (mInput[mInputPos + 2] << 15)
            | mInput[mInputPos + 3];
        mOutput.push_back(word);
        mInputPos += 4;
        break;
    }
    case 12:
    {
        uint64_t word = (uint64_t(12) << sStorageBits)
            | (mInput[mInputPos] << 40)
            | (mInput[mInputPos + 1] << 20)
            | mInput[mInputPos + 2];
        mOutput.push_back(word);
        mInputPos += 3;
        break;
    }
    case 13:
    {
        uint64_t word = (uint64_t(13) << sStorageBits)
            | (mInput[mInputPos] << 30)
            | mInput[mInputPos + 1];
        mOutput.push_back(word);
        mInputPos += 2;
        break;
    }
    case 14:
    {
        uint64_t word = (uint64_t(14) << sStorageBits)
            | mInput[mInputPos];
        mOutput.push_back(word);
        ++mInputPos;
        break;
    }
    case 15:
    {
        uint64_t word = (uint64_t(15) << sStorageBits)
            | (mInput[mInputPos] & ~sSelectorMask);
        mOutput.push_back(word);
        ++mInputPos;
        break;
    }
    }
}


void
Simple8bEncodeBase::resetInputBuffer()
{
    auto newSize = mInput.size() - mInputPos;
    uint64_t largest = 0;
    for (auto i = 0; i < newSize; ++i) {
        mInput[i] = mInput[i + mInputPos];
        largest = std::max<uint64_t>(largest, 64 - Gossamer::count_leading_zeroes(mInput[i]));
    }
    mInput.resize(newSize);
    mLargestSoFar = largest;
    mInputPos = 0;
}


bool
Simple8bEncodeBase::encodeOnce()
{
    uint64_t inputLength = mInput.size() - mInputPos;
    if (inputLength) {
        if (!mLargestSoFar) {
            // Special case: Run of zeroes.
            encodeSelector(0);
            return true;
        }
        uint64_t viableLength = 1;
        uint64_t maxSize = 64 - Gossamer::count_leading_zeroes(mInput[mInputPos]);
        for (uint64_t i = 1; i < inputLength; ++i) {
            uint64_t itemSize = std::max(maxSize, 64 - Gossamer::count_leading_zeroes(mInput[i + mInputPos]));
            if ((i+1)*itemSize > sStorageBits) {
                break;
            }
            maxSize = itemSize;
            viableLength = i+1;
        }
        auto selector = findSelectorByCount(viableLength);
        encodeSelector(selector);
        inputLength = mInput.size() - mInputPos;
    }
    return inputLength > 0;
}


void
Simple8bEncodeBase::encodeWord(const uint64_t pItem)
{
    if (pItem & sContinuationBit) {
        // If we have a continuation bit, flush the buffer first.
        while (encodeOnce()) {
        }
        resetInputBuffer();

        // Encode selector 15.
        mOutput[0] = (uint64_t(15) << sStorageBits) | (pItem & ~sSelectorMask);
        return;
    }

    auto itemSize = 64 - Gossamer::count_leading_zeroes(pItem);
    auto largestIncludingThis = std::max<uint64_t>(mLargestSoFar, itemSize);
    while (largestIncludingThis * (mInput.size() + 1) > sStorageBits || mInput.size() + 1 >= sUncompressedBufferCapacity) {
        encodeOnce();
        resetInputBuffer();
        largestIncludingThis = std::max<uint64_t>(mLargestSoFar, itemSize);
    }

    mInput.push_back(pItem);
    mLargestSoFar = largestIncludingThis;
}

