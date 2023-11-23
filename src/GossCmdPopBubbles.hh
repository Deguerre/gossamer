// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#ifndef GOSSCMDPOPBUBBLES_HH
#define GOSSCMDPOPBUBBLES_HH

#ifndef GOSSCMD_HH
#include "GossCmd.hh"
#endif

class GossCmdPopBubbles : public GossCmd
{
public:
    void operator()(const GossCmdContext& pCxt);

    GossCmdPopBubbles(const std::string& pIn, const std::string& pOut,
                      const std::optional<uint64_t>& pNumThreads,
                      const std::optional<uint64_t>& pMaxSequenceLength,
                      const std::optional<uint64_t>& pMaxEditDistance,
                      const std::optional<double>& pMaxRelativeErrors,
                      const std::optional<uint64_t>& pCutoff,
                      const std::optional<double>& pRelCutoff)
        : mIn(pIn), mOut(pOut),
          mNumThreads(pNumThreads),
          mMaxSequenceLength(pMaxSequenceLength),
          mMaxEditDistance(pMaxEditDistance),
          mMaxRelativeErrors(pMaxRelativeErrors),
          mCutoff(pCutoff), mRelCutoff(pRelCutoff)
    {
    }

private:
    const std::string mIn;
    const std::string mOut;
    std::optional<uint64_t> mNumThreads;
    std::optional<uint64_t> mMaxSequenceLength;
    std::optional<uint64_t> mMaxEditDistance;
    std::optional<double> mMaxRelativeErrors;
    std::optional<uint64_t> mCutoff;
    std::optional<double> mRelCutoff;
};


class GossCmdFactoryPopBubbles : public GossCmdFactory
{
public:
    GossCmdPtr create(App& pApp, const boost::program_options::variables_map& pOpts);

    GossCmdFactoryPopBubbles();
};

#endif // GOSSCMDPOPBUBBLES_HH
