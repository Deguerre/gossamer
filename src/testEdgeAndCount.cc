// Copyright (c) 2008-2016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#include "EdgeAndCount.hh"
#include "StringFileFactory.hh"
#include "GossamerException.hh"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <random>

using namespace boost;
using namespace std;
using namespace Gossamer;

#define GOSS_TEST_MODULE TestEdgeAndCount
#include "testBegin.hh"

namespace {

    position_type parse(const string& pKmer)
    {
        const uint64_t len = pKmer.size();
        position_type e(0);
        for (uint64_t i = 0; i < len; ++i)
        {
            e <<= 2;
            switch (pKmer[i])
            {
                case 'C':
                    e |= 1;
                    break;
                case 'G':
                    e |= 2;
                    break;
                case 'T':
                    e |= 3;
                    break;
                default:
                    break;
            }
        }
        return e;
    }

}

#if 0
BOOST_AUTO_TEST_CASE(test1)
{
    typedef Gossamer::EdgeAndCount Item;
    vector<Item> items;

    items.push_back(Item(parse("AAAAAAAAAAAAAAAAAAAAAACTTTTTTTTTTTACGTGAAGGGAACGTTCATAGG"), 1));
    items.push_back(Item(parse("AAAAAAAAAAAAAAAAAAAAAAGAAAAAAAAAAAAAAGAAAAGAAAAAAAAAGAAA"), 1));

    StringFileFactory fac;
    {
        FileFactory::OutHolderPtr outP(fac.out("tmp"));
        ostream& out(**outP);

        EdgeEncoder<Gossamer::EdgeAndCount> encoder;
        position_type prev(0ULL);
        for (uint64_t i = 0; i < items.size(); ++i)
        {
            const Item& item(items[i]);
            encoder.encode(out, prev, item);
            prev = item.first;
        }
        encoder.flush(out);
    }

    {
        FileFactory::InHolderPtr inP(fac.in("tmp"));
        EdgeAndCount item;

        EdgeDecoder<Gossamer::EdgeAndCount> decoder(**inP);
        for (uint64_t i = 0; i < items.size(); ++i)
        {
            decoder.decode(item);
            BOOST_ASSERT(item.first == items[i].first);
            BOOST_ASSERT(item.second == items[i].second);
        }
 
    }
}
#endif


BOOST_AUTO_TEST_CASE(test2)
{
    typedef Gossamer::position_type Item;
    vector<Item> items;

    items.push_back(Item(parse("AAAAAAAAAAAAAAAAAAAAAACTTTTTTTTTTTACGTGAAGGGAACGTTCATAGG")));
    items.push_back(Item(parse("AAAAAAAAAAAAAAAAAAAAAAGAAAAAAAAAAAAAAGAAAAGAAAAAAAAAGAAA")));

    StringFileFactory fac;
    {
        FileFactory::OutHolderPtr outP(fac.out("tmp"));
        ostream& out(**outP);

        position_type prev = ~position_type(0);
        EdgeEncoder<Gossamer::position_type> encoder;
        for (uint64_t i = 0; i < items.size(); ++i)
        {
            const Item& item(items[i]);
            encoder.encode(out, prev, item);
            prev = item;
        }
        encoder.flush(out);
    }

    {
        FileFactory::InHolderPtr inP(fac.in("tmp"));
        position_type item = ~position_type(0);
        EdgeDecoder<Gossamer::position_type> decoder(**inP);

        for (uint64_t i = 0; i < items.size(); ++i)
        {
            decoder.decode(item);
            BOOST_CHECK_EQUAL(item, items[i]);
        }

    }
}


#include "testEnd.hh"
