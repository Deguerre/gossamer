// Copyright (c) 2008-2016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#include "IntegerCodecs.hh"
#include "Sample.hh"
#include <boost/dynamic_bitset.hpp>
#include <random>
#include <vector>

using namespace boost;
using namespace std;

#define GOSS_TEST_MODULE TestIntegerCodecs
#include "testBegin.hh"


void genIntegers(uint64_t pSeed, uint64_t pN, double pP, vector<uint64_t>& pItems)
{
    std::mt19937_64 rng(pSeed);
    std::exponential_distribution<> dist(pP);
    for (unsigned i = 0; i < pN; ++i) {
        pItems.push_back((uint64_t)dist(rng));
    }
}


void genIntegersBits(uint64_t pSeed, uint64_t pN, unsigned pBits, vector<uint64_t>& pItems)
{
    std::mt19937_64 rng(pSeed);
    std::uniform_int_distribution<uint64_t> dist(0, uint64_t(1) << pBits);
    for (unsigned i = 0; i < pN; ++i) {
        pItems.push_back(dist(rng));
    }
}


BOOST_AUTO_TEST_CASE(test1a)
{
    uint64_t x = 0;
    vector<uint8_t> bytes;

    VByteCodec::encode(x, bytes);
    BOOST_CHECK_EQUAL(bytes.size(), 1);
    BOOST_CHECK_EQUAL(bytes[0], 0);

    vector<uint8_t>::iterator itr(bytes.begin());
    uint64_t y;
    VByteCodec::decode(itr, y);
    BOOST_CHECK_EQUAL(x, y);
}

BOOST_AUTO_TEST_CASE(test1b)
{
    uint64_t x = 1;
    vector<uint8_t> bytes;

    VByteCodec::encode(x, bytes);
    BOOST_CHECK_EQUAL(bytes.size(), 1);
    BOOST_CHECK_EQUAL(bytes[0], 1);

    vector<uint8_t>::iterator itr(bytes.begin());
    uint64_t y;
    VByteCodec::decode(itr, y);
    BOOST_CHECK_EQUAL(x, y);
}

BOOST_AUTO_TEST_CASE(test1c)
{
    uint64_t x = 128;
    vector<uint8_t> bytes;

    VByteCodec::encode(x, bytes);
    BOOST_CHECK_EQUAL(bytes.size(), 2);
    BOOST_CHECK_EQUAL(bytes[0], 0x80);
    BOOST_CHECK_EQUAL(bytes[1], 0x80);

    vector<uint8_t>::iterator itr(bytes.begin());
    uint64_t y;
    VByteCodec::decode(itr, y);
    BOOST_CHECK_EQUAL(x, y);
}

        
BOOST_AUTO_TEST_CASE(test1d_alt)
{           
    for (uint64_t i = 0; i < 64; ++i)
    {
        uint64_t x = 1ull << i;
        vector<uint8_t> bytes;
            
        VByteCodec::encode(x, bytes);
        vector<uint8_t>::iterator itr(bytes.begin());
        uint64_t y;
        VByteCodec::decode(itr, y);
        BOOST_CHECK_EQUAL(x, y);
    }
}               
             

BOOST_AUTO_TEST_CASE(test2)
{
    for (uint64_t x = 0; x < 1024 * 1024; ++x)
    {
        vector<uint8_t> bytes;

        VByteCodec::encode(x, bytes);
        vector<uint8_t>::iterator itr(bytes.begin());
        uint64_t y;
        VByteCodec::decode(itr, y);
        BOOST_CHECK_EQUAL(x, y);
    }
}

BOOST_AUTO_TEST_CASE(tryme)
{
    vector<uint8_t> bytes;
    VByteCodec::encode(1051466, bytes);
    VByteCodec::encode(1089606, bytes);
    VByteCodec::encode(1082820, bytes);
    VByteCodec::encode(1070359, bytes);
    VByteCodec::encode(1097879, bytes);
    VByteCodec::encode(3, bytes);
    VByteCodec::encode(30, bytes);
    VByteCodec::encode(226534, bytes);
    VByteCodec::encode(503445, bytes);
    VByteCodec::encode(19, bytes);
    VByteCodec::encode(21778, bytes);
    VByteCodec::encode(1101788, bytes);
    VByteCodec::encode(0, bytes);
}


void testSimple8b(uint64_t pSeed, uint64_t pN, double pProb)
{
    vector<uint64_t> input;
    vector<uint64_t> output;
    vector<uint64_t> decoded;
    genIntegers(pSeed, pN, pProb, input);

    Simple8bEncode<uint64_t> enc;
    for (auto x : input) {
        enc.encode(x, output);
    }
    enc.encodeEof(output);

    Simple8bDecode<uint64_t> dec;
    auto it = output.begin();
    uint64_t codeword;
    while (dec.decode(it, codeword)) {
        decoded.push_back(codeword);
    }

    BOOST_CHECK_EQUAL(input.size(), decoded.size());
    for (unsigned i = 0; i < input.size(); ++i) {
        // std::cerr << "At " << i << '\n';
        BOOST_CHECK_EQUAL(input[i], decoded[i]);
    }
}

BOOST_AUTO_TEST_CASE(test_simple8b_basic)
{
    testSimple8b(3, 1000, 1.0 / 1024.0 / 1024.0 / 1024.0);
    testSimple8b(5, 1000, 1.0 / 1024.0 / 1024.0);
    testSimple8b(7, 1000, 1.0 / 1024.0);
    testSimple8b(11, 1000, 1.0 / 512.0);
    testSimple8b(13, 1000, 1.0 / 256.0);
    testSimple8b(17, 1000, 1.0 / 128.0);
    testSimple8b(19, 1000, 1.0 / 64.0);
    testSimple8b(23, 1000, 1.0 / 32.0);
    testSimple8b(29, 1000, 1.0 / 16.0);
    testSimple8b(31, 1000, 1.0 / 8.0);
    testSimple8b(37, 1000, 1.0 / 4.0);
    testSimple8b(41, 1000, 1.0 / 2.0);
    testSimple8b(43, 1000, 1.0 / 1.0);
}


void testSimple8bBit(uint64_t pSeed, uint64_t pN, uint64_t pBits)
{
    vector<uint64_t> input;
    vector<uint64_t> output;
    vector<uint64_t> decoded;
    genIntegersBits(pSeed, pN, pBits, input);

    Simple8bEncode<uint64_t> enc;
    for (auto x : input) {
        enc.encode(x, output);
    }
    enc.encodeEof(output);

    Simple8bDecode<uint64_t> dec;
    auto it = output.begin();
    uint64_t codeword;
    while (dec.decode(it, codeword)) {
        decoded.push_back(codeword);
    }

    BOOST_CHECK_EQUAL(input.size(), decoded.size());
    for (unsigned i = 0; i < input.size(); ++i) {
        BOOST_CHECK_EQUAL(input[i], decoded[i]);
    }
}


BOOST_AUTO_TEST_CASE(test_simple8b_specific)
{
    testSimple8bBit(100, 1000, 1);
    testSimple8bBit(101, 1000, 2);
    testSimple8bBit(102, 1000, 3);
    testSimple8bBit(103, 1000, 4);
    testSimple8bBit(104, 1000, 5);
    testSimple8bBit(105, 1000, 6);
    testSimple8bBit(106, 1000, 7);
    testSimple8bBit(107, 1000, 8);
    testSimple8bBit(108, 1000, 9);
    testSimple8bBit(109, 1000, 10);
    testSimple8bBit(110, 1000, 11);
    testSimple8bBit(111, 1000, 12);
    testSimple8bBit(112, 1000, 13);
    testSimple8bBit(113, 1000, 14);
    testSimple8bBit(114, 1000, 15);
    testSimple8bBit(115, 1000, 63);
}


void genKmers(uint64_t pSeed, uint64_t pN, double pP, vector<Gossamer::position_type>& pItems)
{
    std::mt19937_64 rng(pSeed);
    std::vector<uint64_t> draw;
    std::exponential_distribution<> dist(pP);

    Gossamer::position_type item = ~Gossamer::position_type(0);

    for (auto i = 0; i < pN; ++i) {
        item.value().add1(dist(rng));
        pItems.push_back(item);
    }
}


void testSimple8bKmers(uint64_t pSeed, uint64_t pN, double pP)
{
    vector<Gossamer::position_type> input;
    vector<uint64_t> output;
    vector<Gossamer::position_type> decoded;
    genKmers(pSeed, pN, pP, input);

    Simple8bEncode<Gossamer::position_type> enc;
    Gossamer::position_type prev = ~Gossamer::position_type(0);
    for (auto& item : input) {
        auto encode = item;
        encode.value().subtract1(prev.value());
        enc.encode(encode, output);
        prev = item;
    }
    enc.encodeEof(output);

    Simple8bDecode<Gossamer::position_type> dec;
    Gossamer::position_type item = ~Gossamer::position_type(0);
    auto it = output.begin();
    Gossamer::position_type codeword;
    while (dec.decode(it, codeword)) {
        item.value().add1(codeword.value());
        decoded.push_back(item);
    }

    BOOST_CHECK_EQUAL(input.size(), decoded.size());
    for (unsigned i = 0; i < input.size(); ++i) {
        BOOST_CHECK_EQUAL(input[i], decoded[i]);
    }
}


BOOST_AUTO_TEST_CASE(test_simple8b_kmers)
{
    testSimple8bKmers(3, 1000, 1.0 / 1024.0 / 1024.0 / 1024.0);
    testSimple8bKmers(5, 1000, 1.0 / 1024.0 / 1024.0);
    testSimple8bKmers(7, 1000, 1.0 / 1024.0);
    testSimple8bKmers(11, 1000, 1.0 / 512.0);
    testSimple8bKmers(13, 1000, 1.0 / 256.0);
    testSimple8bKmers(17, 1000, 1.0 / 128.0);
}


#include "testEnd.hh"
