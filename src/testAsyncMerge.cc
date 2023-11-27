// Copyright (c) 2008-2016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#include "AsyncMerge.hh"
#include "EdgeAndCount.hh"
#include "Graph.hh"
#include "KmerSet.hh"
#include "StringFileFactory.hh"
#include "Sample.hh"

#include <vector>
#include <iostream>
#include <string>
#include <boost/dynamic_bitset.hpp>
#include <random>


#define GOSS_TEST_MODULE TestAsyncMerge
#include "testBegin.hh"

using namespace boost;
using namespace std;

void genPairs(uint64_t pSeed, uint64_t pN, vector<Gossamer::EdgeAndCount>& pItems)
{
    static const double p = 1.0 / 1024.0;
    std::mt19937_64 rng(pSeed);
    std::exponential_distribution<> dist(p);
    std::vector<uint64_t> draw;
    Gossamer::sampleWithoutReplacement(rng, pN * 1024, pN, draw);
    std::sort(draw.begin(), draw.end());

    for (auto edge : draw) {
        Gossamer::EdgeAndCount itm;
        itm.first = Gossamer::position_type(edge);
        itm.second = dist(rng) + 1;
        pItems.push_back(itm);
    }
}

void genEdges(uint64_t pSeed, uint64_t pN, vector<Gossamer::position_type>& pItems)
{
    static const double p = 1.0 / 1024.0;
    std::mt19937_64 rng(pSeed);
    std::vector<uint64_t> draw;
    Gossamer::sampleWithoutReplacement(rng, pN * 1024, pN, draw);
    std::sort(draw.begin(), draw.end());

    for (auto edge : draw) {
        pItems.push_back(Gossamer::position_type(edge));
    }
}

template<typename Item>
void writeFile(const string& pFn, const vector<Item>& pItems, FileFactory& pFactory)
{
    FileFactory::OutHolderPtr outp(pFactory.out(pFn));
    ostream& out(**outp);
    Gossamer::position_type prev = ~Gossamer::position_type(0);
    EdgeEncoder<Item> encoder;
    for (uint64_t i = 0; i < pItems.size(); ++i)
    {
        encoder.encode(out, prev, pItems[i]);
        prev = Gossamer::EdgeItemTraits<Item>::edge(pItems[i]);
    }
    encoder.flush(out);
}

BOOST_AUTO_TEST_CASE(test1_eac)
{
    StringFileFactory fac;

    vector<Gossamer::EdgeAndCount> x1;
    genPairs(17, 65536, x1);
    writeFile<Gossamer::EdgeAndCount>("x1", x1, fac);

    {
        FileFactory::InHolderPtr inp(fac.in("x1"));
        istream& in(**inp);
        Gossamer::EdgeAndCount itm(~Gossamer::position_type(0), 0);
        uint64_t i = 0;
        EdgeDecoder<Gossamer::EdgeAndCount> decoder(in);
        while (in.good())
        {
            decoder.decode(itm);
            if (!in.good())
            {
                break;
            }
            BOOST_CHECK(i < x1.size());
            BOOST_CHECK(x1[i] == itm);
            ++i;
        }
    }
}

BOOST_AUTO_TEST_CASE(test1_pt)
{
    StringFileFactory fac;

    vector<Gossamer::position_type> x1;
    genEdges(17, 65536, x1);
    writeFile<Gossamer::position_type>("x1", x1, fac);

    {
        FileFactory::InHolderPtr inp(fac.in("x1"));
        istream& in(**inp);
        Gossamer::position_type itm = ~Gossamer::position_type(0);
        uint64_t i = 0;
        EdgeDecoder<Gossamer::position_type> decoder(in);
        while (in.good())
        {
            decoder.decode(itm);
            if (!in.good())
            {
                break;
            }
            BOOST_CHECK(i < x1.size());
            BOOST_CHECK(x1[i] == itm);
            ++i;
        }
    }
}


BOOST_AUTO_TEST_CASE(test2)
{
    StringFileFactory fac;

    vector<Gossamer::EdgeAndCount> x1;
    genPairs(17, 65536, x1);
    writeFile<Gossamer::EdgeAndCount>("x1", x1, fac);

    vector<Gossamer::EdgeAndCount> x2;
    genPairs(18, 65536, x2);
    writeFile<Gossamer::EdgeAndCount>("x2", x2, fac);

    vector<Gossamer::EdgeAndCount> x3;
    {
        uint64_t i = 0;
        uint64_t j = 0;
        while (i < x1.size() && j < x2.size())
        {
            if (x1[i].first < x2[j].first)
            {
                x3.push_back(x1[i]);
                ++i;
                continue;
            }
            if (x1[i].first > x2[j].first)
            {
                x3.push_back(x2[j]);
                ++j;
                continue;
            }
            x3.push_back(x1[i]);
            x3.back().second += x2[j].second;
            ++i;
            ++j;
        }
        while (i < x1.size())
        {
            x3.push_back(x1[i]);
            ++i;
        }
        while (j < x2.size())
        {
            x3.push_back(x2[j]);
            ++j;
        }
    }

    vector<string> parts;
    parts.push_back("x1");
    parts.push_back("x2");
    vector<uint64_t> sizes;
    sizes.push_back(x1.size());
    sizes.push_back(x2.size());
    AsyncMerge::merge<Graph>(parts, sizes, "x3", 25, x1.size() + x2.size(), 2, 1024, fac);

#if 0
    for (map<string,string>::const_iterator i = fac.files.begin(); i != fac.files.end(); ++i)
    {
        cout << i->first << '\t' << i->second.size() << endl;
    }
#endif

    {
        Graph::LazyIterator itr("x3", fac);
        uint64_t i = 0;
        while (itr.valid())
        {
            BOOST_CHECK(i < x3.size());
            BOOST_CHECK((*itr).first.value() == x3[i].first);
            BOOST_CHECK((*itr).second == x3[i].second);
            ++itr;
            ++i;
        }
        BOOST_CHECK_EQUAL(i, x3.size());
    }
}

BOOST_AUTO_TEST_CASE(test3)
{
    StringFileFactory fac;

    vector<Gossamer::EdgeAndCount> x1;
    genPairs(17, 65533, x1);
    writeFile("x1", x1, fac);

    vector<Gossamer::EdgeAndCount> x2;
    genPairs(18, 65576, x2);
    writeFile("x2", x2, fac);

    vector<Gossamer::EdgeAndCount> x3;
    genPairs(19, 65936, x3);
    writeFile("x3", x3, fac);

    vector<Gossamer::EdgeAndCount> x4;
    genPairs(20, 32757, x4);
    writeFile("x4", x4, fac);

    vector<string> parts;
    parts.push_back("x1");
    parts.push_back("x2");
    parts.push_back("x3");
    parts.push_back("x4");
    vector<uint64_t> sizes;
    sizes.push_back(x1.size());
    sizes.push_back(x2.size());
    sizes.push_back(x3.size());
    sizes.push_back(x4.size());
    AsyncMerge::merge<Graph>(parts, sizes, "x0", 25, x1.size() + x2.size() + x3.size() + x4.size(), 1, 256, fac);

#if 0
    for (map<string,string>::const_iterator i = fac.files.begin(); i != fac.files.end(); ++i)
    {
        cout << i->first << '\t' << i->second.size() << endl;
    }
#endif
}

#include "testEnd.hh"
