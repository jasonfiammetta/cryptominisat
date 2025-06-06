/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#ifndef __DISTILLERWITHBIN_H__
#define __DISTILLERWITHBIN_H__

#include <vector>
#include "clause.h"
#include "constants.h"
#include "solvertypes.h"
#include "cloffset.h"
#include "watcharray.h"

namespace CMSat {

using std::vector;

class Solver;
class Clause;

class DistillerLongWithImpl {
    public:
        DistillerLongWithImpl(Solver* solver);
        bool distill_long_with_implicit(bool also_strengthen);

        struct Stats
        {
            void clear()
            {
                Stats tmp;
                *this = tmp;
            }

            Stats& operator+=(const Stats& other);
            void print_short(const Solver* solver) const;
            void print() const;

            struct WatchBased
            {
                double cpu_time = 0.0;
                uint64_t numLitsRem = 0;
                uint64_t numClSubsumed = 0;
                uint64_t triedCls = 0;
                uint64_t shrinked = 0;
                uint64_t totalCls = 0;
                uint64_t totalLits = 0;
                uint64_t ranOutOfTime = 0;
                uint64_t numCalled = 0;

                void clear()
                {
                    WatchBased tmp;
                    *this = tmp;
                }

                void print_short(const string& type, const Solver* solver) const;
                void print() const;

                WatchBased& operator+=(const WatchBased& other)
                {
                    cpu_time += other.cpu_time;
                    numLitsRem += other.numLitsRem;
                    numClSubsumed += other.numClSubsumed;
                    triedCls += other.triedCls;
                    shrinked += other.shrinked;
                    totalCls += other.totalCls;
                    totalLits += other.totalLits;
                    ranOutOfTime += other.ranOutOfTime;
                    numCalled += other.numCalled;

                    return  *this;
                }
            };

            WatchBased irredWatchBased;
            WatchBased redWatchBased;
        };

        const Stats& get_stats() const;
        double mem_used() const;

    private:

        bool remove_or_shrink_clause(Clause& cl, ClOffset& offset);
        void strsub_with_watch(
            bool also_strengthen
            , Clause& cl
        );
        void dump_stats_for_sub_str_all_cl_with_watch(
            bool red
            , bool also_strengthen
            , double my_time
            , double orig_time_available
        );

        struct WatchBasedData
        {
            size_t remLitBin = 0;
            size_t subBin = 0;
            void clear();
            size_t get_cl_subsumed() const;
            size_t get_lits_rem() const;
            void print(const char* prefix) const;
        };
        WatchBasedData watch_based_data;
        bool isSubsumed;
        size_t thisremLitBin;
        void str_and_sub_using_watch(
            Clause& cl
            , const Lit lit
            , const bool also_strengthen
        );
        void strengthen_clause_with_watch(
            const Lit lit
            , const Watched* wit
        );
        bool subsume_clause_with_watch(
           const Lit lit
            , Watched* wit
            , const Clause& cl
        );
        Stats::WatchBased tmpStats;
        //bool needToFinish;
        bool sub_str_cl_with_watch(
            ClOffset& offset
            , const bool also_strengthen
        );
        void randomise_order_of_clauses(vector<ClOffset>& clauses);
        uint64_t calc_time_available(
            const bool also_strengthen
            , const bool red
        ) const;

        bool sub_str_all_cl_with_watch(
            vector<ClOffset>& clauses
            , bool red
            , bool also_strengthen
        );
        int64_t timeAvailable;

        //Working set
        Solver* solver;
        vector<Lit> lits;
        vector<Lit> lits2;
        vector<uint32_t>& seen;
        vector<uint8_t>& seen2;

        //Global status
        Stats runStats;
        Stats globalStats;
        size_t numCalls = 0;

};

inline const DistillerLongWithImpl::Stats& DistillerLongWithImpl::get_stats() const
{
    return globalStats;
}

} //end namespace

#endif //__DISTILLERWITHBIN_H__
