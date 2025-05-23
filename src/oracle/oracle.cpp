// SharpSAT-TD is a modification of SharpSAT (MIT License, 2019 Marc Thurley).
//
// SharpSAT-TD -- Copyright (c) 2021 Tuukka Korhonen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#include "oracle.h"
#include "constants.h"

#include <iostream>
#include "utils.h"
using std::cout;
using std::endl;
using std::max;
using std::pair;
using std::swap;

namespace sspp {
namespace oracle {
namespace {
constexpr const double EPS = 1e-150;
}

void Stats::Print() const {
    cout <<"Decisions/Propagations "<<decisions<<"/"<<mems<<endl;
    cout <<"Conflicts: "<<conflicts<<endl;
    cout <<"Learned clauses/bin/unit: "<<learned_clauses<<"/"<<learned_bin_clauses<<"/"<<learned_units<<endl;
    cout <<"Forgot clauses: "<<forgot_clauses<<endl;
    cout <<"Nontriv redu: "<<nontriv_redu<<endl;
    cout <<"Restarts "<<restarts<<endl;
}

void Oracle::AddSolToCache() {
    for (Var i = 1; i <= vars; i++) {
        assert(vs[i].phase == 0 || vs[i].phase == 1);
        sol_cache[i].push_back(vs[i].phase);
    }
    stats.cache_added++;
}

void Oracle::ClearSolCache() {
    if (sol_cache[1].empty()) return;
    for (Var v = 1; v <= vars; v++) sol_cache[v].clear();
}

bool Oracle::SatByCache(const vector<Lit>& assumps) const {
    // 1st variable's cache size is the same as all the rest
    int cs = sol_cache[1].size();

    // Try all cache lines
    for (int i = 0; i < cs; i++) {
        bool ok = true;
        // all our assumptions must be in the solution
        for (const Lit& l : assumps) {
            if (IsPos(l)) {
                if (sol_cache[VarOf(l)][i] == 0) { ok = false; break; }
            } else {
                if (sol_cache[VarOf(l)][i] == 1) { ok = false; break; }
            }
        }
        if (ok) return true;
    }
    // Not in the cache
    return false;
}

void Oracle::ResizeClauseDb() {
    std::sort(cla_info.begin(), cla_info.end(), [](const CInfo& a, const CInfo& b){
        if (a.glue == -1 || b.glue == -1) return a.glue < b.glue;
        if (a.used != b.used) return a.used > b.used;
        return a.total_used > b.total_used;
    });
    {
        vector<size_t> new_reason(vars+2);
        for (Var v = 1; v <= vars; v++) { new_reason[v] = vs[v].reason; }
        size_t prev_orig_clauses_size = orig_clauses_size;
        vector<Lit> new_clauses(orig_clauses_size);
        for (size_t i = 0; i < orig_clauses_size; i++) { new_clauses[i] = clauses[i]; }
        vector<CInfo> new_cla_info;
        // This sets new_clauses and fixes reasons
        const size_t good_size = 10000;
        num_lbd2_red_cls = 0;
        num_used_red_cls = 0;
        for (size_t i = 0; i < cla_info.size(); i++) {
            if (i+1 < cla_info.size()) {
                cmsat_prefetch(clauses.data() + cla_info[i+1].pt);
            }
            stats.mems++;
            Lit impll = 0;
            size_t cls = cla_info[i].pt;
            if (vs[VarOf(clauses[cls+1])].reason == cls) {
                swap(clauses[cls], clauses[cls+1]);
                // Can only happen in binary clause
                assert(clauses[cls+2] == 0);
            }
            if (vs[VarOf(clauses[cls])].reason == cls) {
                impll = clauses[cls];
                assert(LitVal(impll) == 1);
                for (size_t k = cls+1; clauses[k] != 0; k++) {
                    assert(LitVal(clauses[k]) == -1);
                }
            }
            bool added = false;
            if (cla_info[i].glue == -1) {
                assert(cla_info[i].used <= -1);
                added = true;
            }
            size_t len = 0;
            bool frozen_sat = false;
            while (clauses[cls+len]) {
                if (!frozen_sat && LitVal(clauses[cls+len]) == 1) {
                    Var v = VarOf(clauses[cls+len]);
                    if (vs[v].level == 1) frozen_sat = true;
                }
                len++;
            }
            assert(len >= 2);
            if (frozen_sat) assert(!impll);
            if (cla_info[i].glue <= 2) num_lbd2_red_cls++;
            else if (cla_info[i].used) num_used_red_cls++;
            if (frozen_sat || (impll == 0 && !added && !cla_info[i].Keep()
                        && i > good_size+num_lbd2_red_cls)) {
                stats.forgot_clauses++;
                clauses[cls] = 0;
                continue;
            }
            size_t new_pt = new_clauses.size();
            if (impll) new_reason[VarOf(impll)] = new_pt;
            if (added) assert(new_clauses.size() == orig_clauses_size);
            else new_cla_info.push_back({new_clauses.size(), cla_info[i].glue, cla_info[i].used-1, cla_info[i].total_used});
            for (size_t k = cls; clauses[k]; k++) new_clauses.push_back(clauses[k]);
            new_clauses.push_back(0);
            if (added) orig_clauses_size = new_clauses.size();
            clauses[cls] = new_pt;
        }
        for (Var v = 1; v <= vars; v++) vs[v].reason = new_reason[v];
        for (Lit l = 2; l <= vars*2+1; l++) {
            size_t pos = 0;
            for (size_t i = 0; i < watches[l].size(); i++) {
                watches[l][pos++] = watches[l][i];
                if (watches[l][pos-1].cls >= prev_orig_clauses_size) {
                    if (clauses[watches[l][pos-1].cls] == 0) pos--;
                    else watches[l][pos-1].cls = clauses[watches[l][pos-1].cls];
                }
            }
            watches[l].resize(pos);
        }
        clauses = new_clauses;
        cla_info = new_cla_info;
#ifdef SLOW_DEBUG
        for (Lit l = 2; l <= vars*2+1; l++) {
            for (const auto& w : watches[l]) assert(clauses[w.cls] == l || clauses[w.cls+1] == l);
        }
#endif
    }
    for (Var v = 1; v <= vars; v++) {
        if (vs[v].reason) {
            size_t cl = vs[v].reason;
            assert(clauses[cl-1] == 0);
            if (cl < orig_clauses_size) continue;
            assert(VarOf(clauses[cl]) == v);
            assert(LitVal(clauses[cl]) == 1);
            for (size_t k = cl+1; clauses[k]; k++) {
                assert(LitVal(clauses[k]) == -1);
            }
        }
    }
}

void Oracle::BumpClause(size_t cls) {
    if (cls < orig_clauses_size) return;
    assert(cla_info.size() > 0);
    size_t i = 0;
    for (size_t b = cla_info.size()/2; b >= 1; b /= 2) {
        while (i + b < cla_info.size() && cla_info[i+b].pt <= cls) {
            i += b;
        }
    }
    assert(cla_info[i].pt == cls);
    if (cla_info[i].glue == -1) {
        // Special added clause
        assert(cla_info[i].used == -1);
        return;
    }
    lvl_it++;
    int glue = 0;
    for (;clauses[cls] != 0; cls++) {
        if (lvl_seen[vs[VarOf(clauses[cls])].level] != lvl_it) {
            lvl_seen[vs[VarOf(clauses[cls])].level] = lvl_it;
            glue++;
        }
    }
    cla_info[i].glue = glue;
    cla_info[i].used = 1;
    cla_info[i].total_used++;
    return;
}

void Oracle::InitLuby() {
    luby.clear();
}

int Oracle::NextLuby() {
    luby.push_back(1);
    while (luby.size() >= 2 && luby[luby.size()-1] == luby[luby.size()-2]) {
        luby.pop_back();
        luby.back() *= 2;
    }
    return luby.back();
}

Var Oracle::PopVarHeap() {
    if (var_act_heap[1] <= 0) {
        return 0;
    }
    size_t i = 1;
    while (i < heap_N) {
        if (var_act_heap[i*2] == var_act_heap[i]) {
            i = i*2;
        } else {
            i = i*2+1;
        }
    }
    assert(var_act_heap[i] == var_act_heap[1]);
    assert(i > heap_N);
    Var ret = i - heap_N;
    var_act_heap[i] = -var_act_heap[i];
    for (i/=2;i;i/=2) {
        var_act_heap[i] = max(var_act_heap[i*2], var_act_heap[i*2+1]);
    }
    return ret;
}

void Oracle::ActivateActivity(Var v) {
    if (var_act_heap[heap_N + v] > 0) return;
    assert(var_act_heap[heap_N + v] < 0);
    var_act_heap[heap_N + v] = -var_act_heap[heap_N + v];
    for (size_t i = (heap_N + v)/2; i >= 1; i/=2) {
        var_act_heap[i] = max(var_act_heap[i*2], var_act_heap[i*2+1]);
    }
}

void Oracle::BumpVar(Var v) {
    stats.mems++;
    if (var_act_heap[heap_N + v] < 0) {
        var_act_heap[heap_N + v] -= var_inc;
    } else {
        assert(var_act_heap[heap_N + v] > 0);
        var_act_heap[heap_N + v] += var_inc;
        for (size_t i = (heap_N + v)/2; i >= 1; i/=2) {
            var_act_heap[i] = max(var_act_heap[i*2], var_act_heap[i*2+1]);
        }
    }
    var_inc = var_inc * var_fact;
    if (var_inc > 10000.0) {
        stats.mems+=10;
        var_inc /= 10000.0;
        for (Var i = 1; i <= vars; i++) {
            double& act = var_act_heap[heap_N + i];
            act /= 10000.0;
            if (-EPS < act && act < EPS) {
                assert(act != 0);
                if (act < 0) {
                    act = -EPS;
                } else {
                    act = EPS;
                }
            }
        }
        for (size_t i = heap_N-1; i >= 1; i--) {
            var_act_heap[i] = max(var_act_heap[i*2], var_act_heap[i*2+1]);
        }
    }
}

void Oracle::SetAssumpLit(Lit lit, bool freeze) {
    assert(CurLevel() == 1);
    Var v = VarOf(lit);
    assert(vs[v].reason == 0);
    assert(vs[v].level != 1);
    for (Lit tl : {PosLit(v), NegLit(v)}) {
        for (const Watch w : watches[tl]) {
            stats.mems++;
            assert(w.size > 2);
            size_t pos = w.cls;
            size_t opos = w.cls+1;
            if (clauses[pos] != tl) {
                pos++;
                opos--;
            }
            assert(clauses[pos] == tl);
            size_t f = 0;
            for (size_t i = w.cls+2; clauses[i]; i++) {
                if (LitVal(clauses[i]) == 0) {
                    f = i;
                }
            }
            assert(f);
            swap(clauses[f], clauses[pos]);
            watches[clauses[pos]].push_back({w.cls, clauses[opos], w.size});
        }
        watches[tl].clear();
    }
    assert(watches[lit].empty());
    assert(watches[Neg(lit)].empty());
    assert(prop_q.empty());
    if (freeze) { Assign(lit, 0, 1); }
    else { Assign(lit, 0, 2); }
    assert(decided.back() == VarOf(lit));
    decided.pop_back();
    assert(prop_q.back() == Neg(lit));
    prop_q.pop_back();
}

void Oracle::Assign(Lit dec, size_t reason_clause, int level) {
    if (level <= 1) reason_clause = 0;
    Var v = VarOf(dec);
    lit_val[dec] = 1;
    lit_val[Neg(dec)] = -1;
    vs[v].phase = IsPos(dec) ? 1 : 0;
    vs[v].reason = reason_clause;
    vs[v].level = level;
    oclv("Assigning " << v << " to: " << IsPos(dec) << " at level: " << level << " reason: " << reason_clause);
    decided.push_back(v);
    prop_q.push_back(Neg(dec));
    cmsat_prefetch(watches[Neg(dec)].data());
}

void Oracle::UnDecide(int level) {
    while (!decided.empty() && vs[decided.back()].level >= level) {
        stats.mems++;
        Var v = decided.back();
        decided.pop_back();
        lit_val[PosLit(v)] = 0;
        lit_val[NegLit(v)] = 0;
        vs[v].reason = 0;
        vs[v].level = 0;
        ActivateActivity(v);
        oclv("UNAss " << v);
    }
    assert(prop_q.empty());
}

size_t Oracle::AddLearnedClause(const vector<Lit>& clause) {
    stats.learned_clauses++;
    if (clause.size() == 2) stats.learned_bin_clauses++;
    assert(clause.size() >= 2);

    // Compute LBD. Notice that the clause has been sorted already in terms of decision
    // levels of its literals, before this function is called.
    int glue = 2;
    assert(!LitAssigned(clause[0]));
    for (size_t i = 1; i < clause.size(); i++) {
        assert(LitAssigned(clause[i]) && !LitSat(clause[i]));
        if (i >= 2) {
            assert(vs[VarOf(clause[i])].level <= vs[VarOf(clause[i-1])].level);
            if (vs[VarOf(clause[i])].level < vs[VarOf(clause[i-1])].level) {
                glue++;
            }
        }
    }

    // Attach
    size_t pt = clauses.size();
    watches[clause[0]].push_back({pt, clause[1], (int)clause.size()});
    watches[clause[1]].push_back({pt, clause[0], (int)clause.size()});
    for (Lit lit : clause) clauses.push_back(lit);
    clauses.push_back(0);
    cla_info.push_back({pt, glue, 1, 0});
    return pt;
}

// Check if lit can be removed from the learned clause
bool Oracle::LitReduntant(Lit lit) {
    // TODO: this can be optimized a lot
    assert(redu_s.empty());
    redu_it++;
    redu_s.push_back(lit);
    int its = 0;
    while (!redu_s.empty()) {
        its++;
        stats.mems++;
        lit = redu_s.back();
        redu_s.pop_back();
        Var v = VarOf(lit);
        assert(vs[v].reason);
        size_t rc = vs[v].reason;
        if (clauses[rc] != Neg(lit)) {
            swap(clauses[rc], clauses[rc+1]);
        }
        assert(LitVal(lit) == -1);
        assert(clauses[rc] == Neg(lit));
        for (size_t k = rc+1; clauses[k]; k++) {
            if (!in_cc[clauses[k]] && vs[VarOf(clauses[k])].level > 1) {
                if (vs[VarOf(clauses[k])].reason == 0) {
                    redu_s.clear();
                    return false;
                } else {
                    if (redu_seen[clauses[k]] != redu_it) {
                        redu_seen[clauses[k]] = redu_it;
                        redu_s.push_back(clauses[k]);
                    }
                }
            }
        }
    }
    if (its >= 2) {
        stats.nontriv_redu++;
    }
    return true;
}

vector<Lit> Oracle::LearnUip(size_t conflict_clause) {
    assert(conflict_clause > 0);
    BumpClause(conflict_clause);
#ifdef DEBUG_ORACLE_VERB
    oclv2("Conflict clause NUM: " << conflict_clause << " cl:");
    for(size_t i = conflict_clause; clauses[i]; i++) {
        oclv2(" v: " << VarOf(clauses[i]) << " val: " << LitVal(clauses[i]));
    }
    oclv2(endl);
#endif

    vector<Lit> clause = {0};
    int level = vs[VarOf(clauses[conflict_clause])].level;
    int open = 0;
    oclv("---");
    for (size_t i = conflict_clause; clauses[i]; i++) {
        assert(LitVal(clauses[i]) == -1);
        oclv("clauses[i]: " << VarOf(clauses[i]) << " val: " << LitVal(clauses[i])
                << " vs[VarOf(clauses[i])].level: " << vs[VarOf(clauses[i])].level
                << " level: " << level);
        assert(vs[VarOf(clauses[i])].level <= level);
        BumpVar(VarOf(clauses[i]));
        if (vs[VarOf(clauses[i])].level == level) {
            open++;
            seen[VarOf(clauses[i])] = true;
        } else if (vs[VarOf(clauses[i])].level > 1) {
            clause.push_back(clauses[i]);
            in_cc[clauses[i]] = true;
        }
    }
    assert(open > 0);
    for (size_t i = decided.size()-1; open; i--) {
        Var v = decided[i];
        if (!seen[v]) continue;
        assert(vs[v].level == level);
        open--;
        if (open) {
            stats.mems++;
            BumpClause(vs[v].reason);
            for (size_t j = vs[v].reason; clauses[j]; j++) {
                Var tv = VarOf(clauses[j]);
                if (seen[tv]) continue;
                BumpVar(tv);
                if (vs[tv].level == level) {
                    open++;
                    seen[tv] = true;
                } else if (!in_cc[clauses[j]] && vs[tv].level > 1) {
                    clause.push_back(clauses[j]);
                    in_cc[clauses[j]] = true;
                }
            }
        } else {
            clause[0] = Neg(MkLit(v, vs[v].phase));
        }
        seen[v] = false;
    }
    for (size_t i = 1; i < clause.size(); i++) {
        assert(VarOf(clause[i]) != VarOf(clause[i-1]));
    }


    // Conflict minimization
    for (size_t i = 1; i < clause.size(); i++) {
        if (vs[VarOf(clause[i])].reason) {
            stats.mems++;
            if (LitReduntant(clause[i])) {
                assert(in_cc[clause[i]]);
                in_cc[clause[i]] = false;
                SwapDel(clause, i);
                i--;
            }
        }
    }
    std::sort(clause.begin(), clause.end(), [&](Lit l1, Lit l2) {
        int d1 = vs[VarOf(l1)].level;
        int d2 = vs[VarOf(l2)].level;
        if (d1 == d2) {
            return l1 < l2;
        }
        return d1 > d2;
    });
    for (size_t i = 1; i < clause.size(); i++) {
        assert(in_cc[clause[i]]);
        in_cc[clause[i]] = false;
    }
    return clause;
}

// Returns id of conflict clause or 0 if no conflict
size_t Oracle::Propagate(int level) {
    size_t conflict = 0;
    for (size_t i = 0; i < prop_q.size(); i++) {
        stats.mems++;
        // ff short for falsified
        const Lit ff = prop_q[i];
        assert(vs[VarOf(ff)].level == level);
        vector<Watch>& wt = watches[ff];
        vector<Watch>::const_iterator j1 = wt.begin();
        vector<Watch>::iterator j2 = wt.begin();
        while (j1 != wt.end()) {
            // This code is inspired by cadical propagation code
            const Watch w = *j2++ = *j1++;
            int bv = LitVal(w.blit);
            if (bv > 0) continue; // SAT by blit
            if (w.size == 2) {
                if (bv < 0) {
                    // CONFLICT
                    conflict = w.cls;
                }    else {
                    // UNIT
                    Assign(w.blit, w.cls, level);
                }
                continue;
            }
            if (conflict) break;
            // Check if satisfied by the other watched literal
            // fun xor swap trick
            stats.mems++;
            const Lit other = clauses[w.cls]^clauses[w.cls+1]^ff;
            int ov = LitVal(other);
            if (ov > 0) { // SAT by other watch - change blit
                j2[-1].blit = other;
                continue;
            }
            clauses[w.cls] = other;
            clauses[w.cls+1] = ff;
            // Try to find true or unassigned lit
            size_t fo = 0;
            for (size_t k = w.cls+2; clauses[k]; k++) {
                if (LitVal(clauses[k]) != -1) {
                    fo = k;
                    break;
                }
            }
            // Found true or unassigned lit
            if (fo) {
                clauses[w.cls+1] = clauses[fo];
                clauses[fo] = ff;
                watches[clauses[w.cls+1]].push_back({w.cls, other, w.size});
                j2--;
                continue;
            }
            // Now the clause is unit or unsat.
            if (LitAssigned(clauses[w.cls])) {
                // CONFLICT!!
                conflict = w.cls;
                break;
            } else {
                // UNIT
                Assign(clauses[w.cls], w.cls, level);
            }
        }
        if (j2 != wt.end()) {
            while (j1 != wt.end()) {
                *j2++ = *j1++;
            }
            wt.resize(j2 - wt.begin());
        }
        if (conflict) break;
    }
    //stats.propagations += (int)prop_q.size();
    prop_q.clear();
    return conflict;
}


int Oracle::CDCLBT(size_t confl_clause, int min_level) {
    stats.conflicts++;
    auto clause = LearnUip(confl_clause);
    assert(clause.size() >= 1);
    if (clause.size() == 1 || vs[VarOf(clause[1])].level == 1) {
        assert(min_level <= 2);
        UnDecide(3);
        Assign(clause[0], 0, 2);
        learned_units.push_back(clause[0]);
        stats.learned_units++;
        return 2;
    } else {
        int ass_level = vs[VarOf(clause[1])].level;
        assert(ass_level >= 2);
        assert(ass_level < vs[VarOf(clause[0])].level);
        oclv("ass_level: " << ass_level << " min_level: " << min_level);
        if (ass_level >= min_level) {
            oclv("if (ass_level >= min_level) ");
            UnDecide(ass_level+1);
            size_t cl_id = AddLearnedClause(clause);
            Assign(clause[0], cl_id, ass_level);
            return ass_level;
        } else {
            oclv("NOT if (ass_level >= min_level)");
            assert(prop_q.empty());
            UnDecide(min_level+1);
            vector<pair<Lit, int>> decs;
            for (int i = (int)decided.size()-1;; i--) {
                assert(i>0);
                Var v = decided[i];
                assert(vs[v].level <= min_level);
                if (vs[v].level <= ass_level) {
                    break;
                }
                decs.push_back({MkLit(v, vs[v].phase), vs[v].level});
            }
            UnDecide(ass_level+1);
            size_t cl_id = AddLearnedClause(clause);
            Assign(clause[0], cl_id, ass_level);
            if (Propagate(ass_level)) return min_level-1;
            int level = ass_level;
            std::reverse(decs.begin(), decs.end());
            for (int i = 0; i < (int)decs.size(); i++) {
                if (LitVal(decs[i].first) == -1) {
                    return min_level-1;
                }
                if (LitVal(decs[i].first) == 0) {
                    Decide(decs[i].first, decs[i].second);
                    if (Propagate(decs[i].second)) {
                        return min_level-1;
                    }
                    level = decs[i].second;
                }
                if (i) {
                    assert(decs[i].second >= decs[i-1].second);
                }
            }
            return max(level, min_level);
        }
    }
}

TriState Oracle::HardSolve(int64_t max_mems) {
    InitLuby();
    int64_t confls = 0;
    int64_t next_restart = 1;
    int64_t mems_startup = stats.mems;
    int cur_level = 2;
    Var nv = 1;
    while (true) {
        size_t confl_clause = Propagate(cur_level);
        if (stats.mems > mems_startup+max_mems) return TriState::unknown();
        if (confl_clause) {
            confls++;
            total_confls++;
            if (cur_level <= 2) return false;
            cur_level = CDCLBT(confl_clause);
            assert(cur_level >= 2);
            continue;
        }
        if (confls >= next_restart) {
            int nl = NextLuby();
            next_restart = confls + nl*restart_factor;
            UnDecide(3);
            cur_level = 2;
            stats.restarts++;
            if (total_confls > last_db_clean + 10000) {
                last_db_clean = total_confls;
                oclv("c [oracle] Resizing cldb"
                    << " num_lbd2_red_cls: " << num_lbd2_red_cls
                    << " num_used_red_cls: " << num_used_red_cls
                    << " cla_info.size(): " << cla_info.size());
                ResizeClauseDb();
                oclv("c [oracle] after cldb resize"
                    << " num_lbd2_red_cls: " << num_lbd2_red_cls
                    << " num_used_red_cls: " << num_used_red_cls
                    << " cla_info.size(): " << cla_info.size());
            }
        }
        Var decv = 0;
        if (confls == 0) {
            while (nv <= vars && LitVal(PosLit(nv)) != 0) nv++;
            if (nv <= vars) decv = nv;
        } else {
            while (true) {
                decv = PopVarHeap();
                if (decv == 0 || LitVal(PosLit(decv)) == 0) break;
            }
        }
        if (decv == 0) return true;
        cur_level++;
        Decide(MkLit(decv, vs[decv].phase), cur_level);
    }
}

void Oracle::AddOrigClause(vector<Lit> clause, bool entailed) {
    assert(CurLevel() == 1);
    for (int i = 0; i < (int)clause.size(); i++) {
        assert(VarOf(clause[i]) >= 1 && VarOf(clause[i]) <= vars);
        if (LitVal(clause[i]) == 1) return;
        if (LitVal(clause[i]) == -1) {
            SwapDel(clause, i);
            i--;
        }
    }
    for (Lit lit : clause) assert(LitVal(lit) == 0);
    if (!entailed) ClearSolCache();
    if (clause.size() == 0) {
        unsat = true;
        return;
    }
    if (clause.size() == 1) {
        FreezeUnit(clause[0]);
        return;
    }
    assert(clause.size() >= 2);
    bool og = (clauses.size() == orig_clauses_size);
    size_t pt = clauses.size();
    watches[clause[0]].push_back({clauses.size(), clause[1], (int)clause.size()});
    watches[clause[1]].push_back({clauses.size(), clause[0], (int)clause.size()});
    for (Lit lit : clause) clauses.push_back(lit);
    clauses.push_back(0);
    // If we have no learned clauses then this is original clause
    // Otherwise this is "learned" clause with glue -1 and used -1
    if (og) {
        orig_clauses_size = clauses.size();
        return;
    }
    cla_info.push_back({pt, -1, -1, 0});
}

Oracle::Oracle(int vars_, const vector<vector<Lit>>& clauses_,
        const vector<vector<Lit>>& learned_clauses_) : Oracle(vars_, clauses_) {
    for (const auto& clause : learned_clauses_) {
        AddClauseIfNeededAndStr(clause, true);
    }
}

Oracle::Oracle(int vars_, const vector<vector<Lit>>& clauses_) : vars(vars_), rand_gen(1337) {
    assert(vars >= 1);
    vs.resize(vars+1);
    seen.resize(vars+1);
    lvl_seen.resize(vars+3);
    sol_cache.resize(vars+1);
    watches.resize(vars*2+2);
    lit_val.resize(vars*2+2);
    redu_seen.resize(vars*2+2);
    in_cc.resize(vars*2+2);
    // setting magic constants
    restart_factor = 100;

    clauses.push_back(0);
    orig_clauses_size = 1;
    for (const vector<Lit>& clause : clauses_) {
        AddOrigClause(clause, false);
    }

    // variable activity
    var_fact = powl((long double)2, (long double)(1.0L/(long double)vars));
    assert(var_fact > 1.0);
    heap_N = 1;
    while (heap_N <= (size_t)vars) heap_N *= 2;
    var_act_heap.resize(heap_N*2);
    for (Var v = 1; v <= vars; v++) {
        var_act_heap[heap_N + v] = var_inc * RandInt(95, 105, rand_gen);
    }
    for (int i = heap_N-1; i >= 1; i--) {
        var_act_heap[i] = max(var_act_heap[i*2], var_act_heap[i*2+1]);
    }
}

TriState Oracle::Solve(const vector<Lit>& assumps, bool usecache, int64_t max_mems) {
    if (unsat) return false;
    if (usecache && SatByCache(assumps)) {stats.cache_useful++; return true;}
    oclv("SOLVE called ");
    // TODO: solution caching
    for (const auto& lit : assumps) {
        if (LitVal(lit) == -1) {
            prop_q.clear();
            UnDecide(2);
            return false;
        } else if (LitVal(lit) == 0) {
            Decide(lit, 2);
        }
    }
    size_t confl_clause = Propagate(2);
    if (confl_clause) { UnDecide(2); return false; }
    oclv("HARD SOLVING");
    TriState sol = HardSolve(max_mems);
    UnDecide(2);
    if (!unsat) {
        while (!learned_units.empty()) {
            Decide(learned_units.back(), 1);
            learned_units.pop_back();
        }
        if (Propagate(1)) {
            unsat = true;
            assert(sol.isFalse() || sol.isUnknown());
            sol = TriState(false);
        }
    }
    if (sol.isTrue()) {
        if (usecache) { AddSolToCache(); }
    } else if (sol.isFalse()) {
        // UNSAT
        if (assumps.size() == 1) {
            bool ok = FreezeUnit(Neg(assumps[0]));
            if (!ok) {
                assert(unsat);
            }
        }
    }
    return sol;
}

bool Oracle::FreezeUnit(Lit unit) {
    if (unsat) return false;
    assert(CurLevel() == 1);
    if (LitVal(unit) == -1) {
        unsat = true;
        return false;
    }
    if (LitVal(unit) == 1) {
        return true;
    }
    Decide(unit, 1);
    stats.learned_units++;
    oclv("[oracle] learnt unit: " << VarOf(unit) << " val: " << LitVal(unit));
    size_t confl = Propagate(1);
    if (confl) {
        unsat = true;
        return false;
    }
    return true;
}

// Checks if the clause is SAT & then does strengthening on the clause
// before adding it to the CNF
bool Oracle::AddClauseIfNeededAndStr(vector<Lit> clause, bool entailed) {
    if (unsat) return false;
    assert(CurLevel() == 1);
    for (int i = 0; i < (int)clause.size(); i++) {
        if (LitVal(clause[i]) == 1) return false;
        else if (LitVal(clause[i]) == -1) {
            SwapDel(clause, i);
            i--;
        }
    }
    if (clause.size() <= 1) {
        AddOrigClause(clause, entailed);
        return true;
    }

    // We are now going to do strengthening, lit-by-lit, recursively.
    // For example, a V b V c. Enqueue (!a && !b) and propagate. If UNSAT,
    // then we know (a V b) is also true. We just removed a literal.
    for (int i = 0; i < (int)clause.size(); i++) {
        Lit tp = clause[i];
        assert(LitVal(tp) == 0);
        for (Lit o : clause) {
            if (o != tp) {
                Decide(Neg(o), 2);
            }
        }
        size_t confl = Propagate(2);
        if (confl || LitVal(tp) == -1) {
            UnDecide(2);
            SwapDel(clause, i);
            return AddClauseIfNeededAndStr(clause, true);
        }

        // No conflict.
        if (LitVal(tp) == 1) {
            // Propagation as intended
            UnDecide(2);
        } else if (LitVal(tp) == 0) {
            // No propagation -- we need to add the clause
            UnDecide(2);
            AddOrigClause(clause, entailed);
            return true;
        } else {
            assert(0);
        }
    }
    return false;
}

vector<vector<Lit>> Oracle::GetLearnedClauses() const {
    assert(CurLevel() == 1);
    vector<vector<Lit>> ret;
    ret.push_back({});
    for (size_t i = orig_clauses_size; i < clauses.size(); i++) {
        if (clauses[i] == 0) {
            assert(ret.back().size() >= 2);
            sort(ret.back().begin(), ret.back().end());
            ret.push_back({});
        } else {
            ret.back().push_back(clauses[i]);
        }
    }
    assert(ret.back().empty());
    ret.pop_back();

    // Units
    for (Var v = 1; v <= vars; v++) {
        if (LitVal(PosLit(v)) == 1) {
            ret.push_back({PosLit(v)});
        } else if (LitVal(PosLit(v)) == -1) {
            ret.push_back({NegLit(v)});
        }
    }
    return ret;
}

} // namespace oracle
} // namespace sspp
