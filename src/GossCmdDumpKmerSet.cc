// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#include "GossCmdDumpKmerSet.hh"

#include "GossCmdReg.hh"
#include "GossOptionChecker.hh"
#include "KmerSet.hh"
#include "Timer.hh"

#include <string>
#include <boost/lexical_cast.hpp>

using namespace boost;
using namespace boost::program_options;
using namespace std;

typedef vector<string> strings;

namespace // anonymous
{


} // namespace anonymous

void
GossCmdDumpKmerSet::operator()(const GossCmdContext& pCxt)
{
    Logger& log(pCxt.log);
    FileFactory& fac(pCxt.fac);
    Timer t;

    FileFactory::OutHolderPtr outPtr(fac.out(mOut));
    ostream& out(**outPtr);

    KmerSet::LazyIterator itr(mIn, fac);

    out << '#' << KmerSet::version << endl;
    out << itr.K() << '\t' << itr.count() << endl;

    SmallBaseVector v;
    string s;
    out << std::hex << std::setprecision(8);
    for (; itr.valid(); ++itr)
    {
        s.clear();
        auto kmer1 = (*itr).first.value();
        auto kmer2 = kmer1;
        kmer2.reverseComplement(itr.K());
        kmerToString(itr.K(), (*itr).first.value(), s);
        out << s << '\n';
        auto k1 = kmer1.asUInt64();
        auto k2 = kmer2.asUInt64();
        out << "! " << std::setfill('0') << std::setw(16) << std::min(k1,k2) << ' ' << std::setfill('0') << std::setw(16) << std::max(k1,k2) << '\n';
    }
    log(info, "total elapsed time: " + lexical_cast<string>(t.check()));
}


GossCmdPtr
GossCmdFactoryDumpKmerSet::create(App& pApp, const variables_map& pOpts)
{
    GossOptionChecker chk(pOpts);
    FileFactory& fac(pApp.fileFactory());

    string in;
    chk.getRepeatingOnce("graph-in", in);

    string out = "-";
    chk.getOptional("output-file", out, GossOptionChecker::FileCreateCheck(fac, false));

    chk.throwIfNecessary(pApp);

    return make_goss_cmd<GossCmdDumpKmerSet>(in, out);
}

GossCmdFactoryDumpKmerSet::GossCmdFactoryDumpKmerSet()
    : GossCmdFactory("write out the graph in a robust text representation.")
{
    mCommonOptions.insert("graph-in");
    mCommonOptions.insert("output-file");
}
