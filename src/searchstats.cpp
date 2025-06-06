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

#include "searchstats.h"

using namespace CMSat;

SearchStats& SearchStats::operator+=(const SearchStats& other)
{
    numRestarts += other.numRestarts;
    blocked_restart += other.blocked_restart;
    blocked_restart_same += other.blocked_restart_same;

    //Decisions
    decisions += other.decisions;
    decisionsAssump += other.decisionsAssump;
    decisionsRand += other.decisionsRand;
    decisionFlippedPolar += other.decisionFlippedPolar;

    //Conflict minimisation stats
    litsRedNonMin += other.litsRedNonMin;
    litsRedFinal += other.litsRedFinal;
    recMinCl += other.recMinCl;
    recMinLitRem += other.recMinLitRem;

    permDiff_attempt  += other.permDiff_attempt;
    permDiff_rem_lits += other.permDiff_rem_lits;
    permDiff_success += other.permDiff_success;

    furtherShrinkAttempt  += other.furtherShrinkAttempt;
    binTriShrinkedClause += other.binTriShrinkedClause;
    furtherShrinkedSuccess += other.furtherShrinkedSuccess;


    moreMinimLitsStart += other.moreMinimLitsStart;
    moreMinimLitsEnd += other.moreMinimLitsEnd;
    recMinimCost += other.recMinimCost;

    //Red stats
    learntUnits += other.learntUnits;
    learntBins += other.learntBins;
    learntLongs += other.learntLongs;
    otfSubsumed += other.otfSubsumed;
    otfSubsumedImplicit += other.otfSubsumedImplicit;
    otfSubsumedLong += other.otfSubsumedLong;
    otfSubsumedRed += other.otfSubsumedRed;
    otfSubsumedLitsGained += other.otfSubsumedLitsGained;
    red_cl_in_which0 += other.red_cl_in_which0;

    //Hyper-bin & transitive reduction
    advancedPropCalled += other.advancedPropCalled;
    hyperBinAdded += other.hyperBinAdded;
    transReduRemIrred += other.transReduRemIrred;
    transReduRemRed += other.transReduRemRed;

    //Stat structs
    resolvs += other.resolvs;
    conflicts += other.conflicts;

    //Time
    cpu_time += other.cpu_time;

    return *this;
}

SearchStats& SearchStats::operator-=(const SearchStats& other)
{
    numRestarts -= other.numRestarts;
    blocked_restart -= other.blocked_restart;
    blocked_restart_same -= other.blocked_restart_same;

    //Decisions
    decisions -= other.decisions;
    decisionsAssump -= other.decisionsAssump;
    decisionsRand -= other.decisionsRand;
    decisionFlippedPolar -= other.decisionFlippedPolar;

    //Conflict minimisation stats
    litsRedNonMin -= other.litsRedNonMin;
    litsRedFinal -= other.litsRedFinal;
    recMinCl -= other.recMinCl;
    recMinLitRem -= other.recMinLitRem;

    permDiff_attempt  -= other.permDiff_attempt;
    permDiff_rem_lits -= other.permDiff_rem_lits;
    permDiff_success -= other.permDiff_success;

    furtherShrinkAttempt  -= other.furtherShrinkAttempt;
    binTriShrinkedClause -= other.binTriShrinkedClause;
    furtherShrinkedSuccess -= other.furtherShrinkedSuccess;

    moreMinimLitsStart -= other.moreMinimLitsStart;
    moreMinimLitsEnd -= other.moreMinimLitsEnd;
    recMinimCost -= other.recMinimCost;

    //Red stats
    learntUnits -= other.learntUnits;
    learntBins -= other.learntBins;
    learntLongs -= other.learntLongs;
    otfSubsumed -= other.otfSubsumed;
    otfSubsumedImplicit -= other.otfSubsumedImplicit;
    otfSubsumedLong -= other.otfSubsumedLong;
    otfSubsumedRed -= other.otfSubsumedRed;
    otfSubsumedLitsGained -= other.otfSubsumedLitsGained;
    red_cl_in_which0 -= other.red_cl_in_which0;

    //Hyper-bin & transitive reduction
    advancedPropCalled -= other.advancedPropCalled;
    hyperBinAdded -= other.hyperBinAdded;
    transReduRemIrred -= other.transReduRemIrred;
    transReduRemRed -= other.transReduRemRed;

    //Stat structs
    resolvs -= other.resolvs;
    conflicts -= other.conflicts;

    //Time
    cpu_time -= other.cpu_time;

    return *this;
}

SearchStats SearchStats::operator-(const SearchStats& other) const
{
    SearchStats result = *this;
    result -= other;
    return result;
}

void SearchStats::printCommon(uint64_t props, bool do_print_times, const string& prefix) const
{
    print_stats_line(prefix + "restarts"
        , numRestarts
        , float_div(conflicts, numRestarts)
        , "confls per restart"

    );
    print_stats_line(prefix + "blocked restarts"
        , blocked_restart
        , float_div(blocked_restart, numRestarts)
        , "per normal restart"

    );
    if (do_print_times)
    print_stats_line(prefix + "time", cpu_time);
    print_stats_line(prefix + "decisions", decisions
        , stats_line_percent(decisionsRand, decisions)
        , "% random"
    );

    print_stats_line(prefix + "propagations"
                     , print_value_kilo_mega(props, false)
                     , print_value_kilo_mega(ratio_for_stat(props, cpu_time), false),
                     "props/s");

    print_stats_line(prefix + "decisions/conflicts"
        , float_div(decisions, conflicts)
    );
}

void SearchStats::print_short(uint64_t props, bool do_print_times, const string& prefix) const
{
    //Restarts stats
    printCommon(props, do_print_times, prefix);
    print_stats_line(prefix + "conflicts", conflicts);
    print_stats_line(prefix + "conf lits non-minim"
        , litsRedNonMin
        , float_div(litsRedNonMin, conflicts)
        , "lit/confl"
    );

    print_stats_line(prefix + "conf lits final"
        , float_div(litsRedFinal, conflicts)
    );

    print_stats_line(prefix + "red which0"
        , red_cl_in_which0
        , stats_line_percent(red_cl_in_which0, conflicts)
        , "% of confl"
    );
}

void SearchStats::print(uint64_t props, bool do_print_times, const string& prefix) const
{
    printCommon(props, do_print_times, prefix);
    print_stats_line(prefix + "conflicts", conflicts);

    /*assert(numConflicts
        == conflsBin + conflsTri + conflsLongIrred + conflsLongRed);*/

    cout << "c LEARNT stats" << endl;
    print_stats_line(prefix + "units learnt"
        , learntUnits
        , stats_line_percent(learntUnits, conflicts)
        , "% of conflicts");

    print_stats_line(prefix + "bins learnt"
        , learntBins
        , stats_line_percent(learntBins, conflicts)
        , "% of conflicts");

    print_stats_line(prefix + "long learnt"
        , learntLongs
        , stats_line_percent(learntLongs, conflicts)
        , "% of conflicts"
    );

    print_stats_line(prefix + "red which0"
        , red_cl_in_which0
        , stats_line_percent(red_cl_in_which0, conflicts)
        , "% of confl"
    );

    cout << "c SEAMLESS HYPERBIN&TRANS-RED stats" << endl;
    print_stats_line(prefix + "advProp called"
        , advancedPropCalled
    );
    print_stats_line(prefix + "hyper-bin add bin"
        , hyperBinAdded
        , ratio_for_stat(hyperBinAdded, advancedPropCalled)
        , "bin/call"
    );
    print_stats_line(prefix + "trans-red rem irred bin"
        , transReduRemIrred
        , ratio_for_stat(transReduRemIrred, advancedPropCalled)
        , "bin/call"
    );
    print_stats_line(prefix + "trans-red rem red bin"
        , transReduRemRed
        , ratio_for_stat(transReduRemRed, advancedPropCalled)
        , "bin/call"
    );

    cout << "c CONFL LITS stats" << endl;
    print_stats_line(prefix + "orig "
        , litsRedNonMin
        , ratio_for_stat(litsRedNonMin, conflicts)
        , "lit/confl"
    );

    print_stats_line(prefix + "recurs-min effective"
        , recMinCl
        , stats_line_percent(recMinCl, conflicts)
        , "% attempt successful"
    );

    print_stats_line(prefix + "recurs-min lits"
        , recMinLitRem
        , stats_line_percent(recMinLitRem, litsRedNonMin)
        , "% less overall"
    );

    print_stats_line(prefix + "permDiff call%"
        , stats_line_percent(permDiff_attempt, conflicts)
        , stats_line_percent(permDiff_success, permDiff_attempt)
        , "% attempt successful"
    );

    print_stats_line(prefix + "permDiff lits-rem"
        , permDiff_rem_lits
        , ratio_for_stat(permDiff_rem_lits, permDiff_attempt)
        , "less lits/cl on attempts"
     );


    print_stats_line(prefix + "further-min call%"
        , stats_line_percent(furtherShrinkAttempt, conflicts)
        , stats_line_percent(furtherShrinkedSuccess, furtherShrinkAttempt)
        , "% attempt successful"
    );

    print_stats_line(prefix + "bintri-min lits"
        , binTriShrinkedClause
        , stats_line_percent(binTriShrinkedClause, litsRedNonMin)
        , "% less overall"
    );

    print_stats_line(prefix + "final avg"
        , ratio_for_stat(litsRedFinal, conflicts)
    );

    //General stats
    //print_stats_line(prefix + "Memory used", (double)mem_used / 1048576.0, " MB");
    #if !defined(_MSC_VER) && defined(RUSAGE_THREAD)
    print_stats_line(prefix + "single-thread CPU time", cpu_time, " s");
    #else
    print_stats_line(prefix + "all-threads sum CPU time", cpu_time, " s");
    #endif
}
