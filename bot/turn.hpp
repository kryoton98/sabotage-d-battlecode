// Sabotage-d / turn.hpp: one turn. turn() wraps turn_() (it appends the sonar words and PROTOCOL 3, and falls back
// to fallback() - any move that is not obviously fatal - on an exception); emit_move writes a move and remembers
// where the head will be; turn_() decides, and its body is the turn_*.hpp files in the order listed in it.
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    // py: turn
    std::string turn(const std::vector<std::string>& L) {
        std::string out;
        try {
            traversal_allowed=false;
            body_prepared=body_checked=body_refined=false;
            out = turn_(L);
            try { out=improve_travel(L,out); } catch(const std::exception&) {}
            if(T(P.BODY_TRACE) && body_checked) {
                std::string mark="INDICATOR corridor-check ";
                if(body_refined) mark+="corridor-refine ";
                if(body_budget.limited) mark+="corridor-limit ";
                out=mark+"\n"+out;
            }
        } catch (const std::exception&) {
            if (T(P.DEBUG)) throw;
            out = fallback(L);
        }
        try { out=protect_queen(L,out); } catch(const std::exception&) { if(T(P.DEBUG)) throw; }
        turns_played++;
        if (T(P.DEBUG) && !dbg_note.empty()) out = "INDICATOR " + dbg_note + "\n" + out;
        dbg_note.clear();
        uint64_t b = beacon_out;
        if (T(P.QDEAD) && tq_dead && (double)tq_hops < P.QDEAD_HOPS &&
            ((double)(cur_rnd - tq_learned) < P.QDEAD_NEWS || (T(P.QDEAD_EVERY) && cur_rnd % (int)P.QDEAD_EVERY == 0)))
            b = qd_val(tq_hops);                  // (7 Oct, QDEAD) the news first, then every QDEAD_EVERY-th round
        else if (T(P.ARMY) && oq_dead && (double)oq_hops < P.QDEAD_HOPS &&
                 ((double)(cur_rnd - oq_learned) < P.QDEAD_NEWS || (T(P.QDEAD_EVERY) && cur_rnd % (int)P.QDEAD_EVERY == 1)))
            b = oqd_val(oq_hops);                 // (8 Oct, ARMY) our queen is dead: pass it on
        if (!b && tw_out) b = tw_out;             // (7 Oct, TW) a threat warning
        tw_out = 0;
        if (!b && gossip_out) b = gossip_out;     // (v2.11) no king to tell of: tell what I have seen
        gossip_out = 0;
        if (!b && reg_out) b = reg_out;           // (v3, REG_R) nothing else to tell: my estimate of the contact rate
        reg_out = 0;
        if (b) {
            // the king's whereabouts, in all four directions
            beacon_out = 0;
            char buf[128];
            unsigned long long v = (unsigned long long)b;
            snprintf(buf, sizeof buf, "SONAR N %llu\nSONAR E %llu\nSONAR S %llu\nSONAR W %llu\n", v, v, v, v);
            out = std::string(buf) + out;
        }
        if (T(P.BEACON64)) return out + "\nPROTOCOL 3";
        return out;
    }

    std::string fallback(const std::vector<std::string>& L) {   // py: _fallback - any move not obviously fatal
        long long nm = pyint(tail_from(at(L, 4), 9));
        long long p = 5 + nm;
        if (starts_with(at(L, p), "ECHOES")) p += 1;
        long long tl0 = p;
        p += 49;
        long long nbod = pyint(tail_from(at(L, p), 14));
        p += 1;
        long long bl0 = p;
        p += nbod;
        long long el0 = p;
        auto hs = split_ws(at(L, tl0 + 24));
        if (hs.size() < 2) throw PyError("fallback: head tile");
        long long hx = pyint(hs[0]), hy = pyint(hs[1]);
        std::vector<std::pair<long long, long long>> occ;
        for (long long k = 0; k < nbod; k++) {
            auto q = split_ws(at(L, bl0 + k));
            if (q.size() < 4) throw PyError("fallback: body");
            occ.push_back({pyint(q[2]), pyint(q[3])});
        }
        auto h3 = split_ws(at(L, el0 + 3)), h4 = split_ws(at(L, el0 + 4)), v3 = split_ws(at(L, el0 + 8 + 3));
        if (h3.size() < 4 || h4.size() < 4 || v3.size() < 5) throw PyError("fallback: edges");
        std::string edge[4] = {h3[3], v3[4], h4[3], v3[3]};
        for (int d = 0; d < 4; d++) {
            if (edge[d] != ".") continue;
            std::pair<long long, long long> t{fmod(hx + DXS[d], W), fmod(hy + DYS[d], H)};
            bool hit = false;
            for (auto& o : occ)
                if (o == t) hit = true;
            if (hit) continue;
            return std::string("MOVE ") + DCH[d];
        }
        return "MOVE N";
    }

    // py: _emit_move - remember where I expect to go, so the trail stays exact
    std::string emit_move(const std::vector<int>& path, int hg, const char* why) {
        if (room_guest_now && T(P.ROOM_AVOID) && !path.empty()) {
            // (7 Oct, ROOM_AVOID) a guest crossing out of the owner's room remembers the portal
            const PTile* ph = ptile(hg);
            const int pid = ph != nullptr ? ph->get(path[0], -1) : -1;
            if (pid >= 0) room_avoid.push_back({pid, cur_rnd + (int)P.ROOM_AVOID});
        }
        std::vector<int> pl;
        bool ok = true;
        int g = hg;
        for (int d : path) {
            g = nb(g)[(size_t)d];
            if (g < 0) {
                ok = false;
                break;
            }
            pl.push_back(g);
        }
        has_plan = ok;
        plan = ok ? pl : std::vector<int>();
        if (T(P.SPRINT_LEARN) && path.size() >= 2 && ok) {
            int e = 0;
            std::vector<int> done;
            for (int t : pl)
                if (std::find(done.begin(), done.end(), t) == done.end()) {
                    done.push_back(t);
                    if (std::find(spr_pearls.begin(), spr_pearls.end(), t) != spr_pearls.end()) e++;
                }
            spr_r = cur_rnd;
            spr_k = (int)path.size();
            spr_l0 = my_len;
            spr_e = e;
        }
        std::string s = "MOVE ";
        for (int d : path) s += DCH[d];
        if (why[0] && T(P.DEBUG)) s = std::string("INDICATOR ") + why + "\n" + s;
        return s;
    }

    // (py: _turn) The turn itself. One function, in parts: each #include below is the next run of its statements, and
    // they share its locals. It returns the action; turn() above adds the sonar and the protocol line.
    std::string turn_(const std::vector<std::string>& L) {
#include "turn_input.hpp"         // the input parsed; tiles, pearls and edges into memory; the window move table mv
#include "turn_dragons.hpp"       // the dragons in view: queens seen or dead, the clocks, who is king; my own body
#include "turn_threats.hpp"       // enemy reach: the tiles a head can ram next; warnings heard; the queen's guard
        // ------------------------------------------------------- 0'. (8 Oct, ARMY) our queen is dead, theirs seen lately:
        //                                                              a big dragon makes hunters
        if (T(Pp.ARMY) && oq_dead && !is_queen && rnd >= Pp.ARMY_FROM && !tq_dead && rnd < 499 && (double)length >= Pp.ARMY_SPLIT &&
            qprey_g >= 0 && rnd - qprey_r <= Pp.ARMY_SEEN && ucount < limit && enemies.empty() &&
            length - (int)Pp.ARMY_CHILD >= 2 && (int)Pp.ARMY_CHILD >= 2) {
            if (T(Pp.DEBUG)) dbg_note += "army-split ";
            return "SPLIT " + std::to_string((int)Pp.ARMY_CHILD);
        }
        // ------------------------------------------------------- 1. kill?
        if (T(Pp.RAM) && !enemies.empty() && ucount >= 2) {
            std::string act;
            ct_fr = friends;                           // (9 Oct, CT) where our other heads are
            if (try_kill(enemies, mv, freeat, length, rnd, hg, occ, (int)friends.size(), act)) return act;
        }

        // ------------------------------------------------------- 1a'. (6 Oct, LF) the late feed on big maps
        if (T(Pp.LF_ROUND) && rnd >= Pp.LF_ROUND && rnd < 499 && (double)W * H >= Pp.LF_AREA && enemies.empty() &&
            !friends.empty() && length <= Pp.LF_MAXLEN && ucount > Pp.LF_UNITS && !is_queen && !lead) {
            std::string act;
            if (late_feed(heads, seg_n, mv, occ, length, hg, act)) return act;
        }

        // ------------------------------------------------------- 1a''. (6 Oct, MG) the gathering: feed the crown in view
        if (mg_now && !lead && !is_queen && kid >= 0 && king_i >= 0 && enemies.empty() && ucount >= 2 && rnd < 499) {
            std::string act;
            if (merge_feed(king_i, mv, occ, hg, rnd, act)) return act;
        } else if (mg_now && T(Pp.MG_LOCAL) && !lead && !is_queen && kid < 0 && enemies.empty() && ucount >= 2 && rnd < 499) {
            // (6 Oct, MG_LOCAL) no crown known: the longest friend in view that is clearly longer than I am
            int li = -1, ln = length + (int)Pp.MG_LOCAL - 1;
            for (auto& h : heads)
                if (!h.en && seg_n.get(h.did, 0) > ln) {
                    ln = seg_n.get(h.did, 0);
                    li = h.i;
                }
            std::string act;
            if (li >= 0 && merge_feed(li, mv, occ, hg, rnd, act)) return act;
        }

        // ------------------------------------------------------- 1b. feed?
        {
            bool khc;   // ((self.king_hungry & 1) if (...) else hungry)
            if (king_hungry >= 0 && rnd - king_len_round <= 20 && !king_far_) khc = (king_hungry & 1) != 0;
            else khc = hungry;
            bool gate_ok = T(Pp.FEED_LOCAL) || length >= 4 || !T(Pp.FEED_HUNGRY) || rnd >= Pp.FEED_LATE ||
                           (T(Pp.CAP_FEED) && ucount >= limit - Pp.CAP_FEED + 1) || khc;
            const bool qkf = qk && T(Pp.QK_FEED) && rnd >= Pp.QK_FEED;   // (8 Oct, LQ) feeding the queen-king from round QK_FEED
            if (T(Pp.FEED) && (rnd >= feed_round || qkf) && enemies.empty() && !friends.empty() && length >= Pp.FEED_MIN_LEN &&
                ucount >= 4 && !assn_ && (gate_ok || qkf)) {
                for (int i = 0; i < 49; i++) cur_wp[i] = wp[i];
                feed_gate = Pp.FEED_LOCAL == 0 ||
                            (length >= 4 || !T(Pp.FEED_HUNGRY) || rnd >= Pp.FEED_LATE ||
                             (T(Pp.CAP_FEED) && ucount >= limit - Pp.CAP_FEED + 1) || khc);
                std::string act;
                if (feed(heads, seg_n, mv, occ, length, rnd, hg, wg, &head_face, act)) return act;
            }
        }
#include "turn_lanes_rooms.hpp"   // friends' exits, never-trap, lanes, closed rooms, the vault, farm & flip
#include "turn_split.hpp"         // 2. split?
#include "turn_move_targets.hpp"  // 3. move: the candidate steps, territory, food targets, pockets
#include "turn_move_roles.hpp"    //    what this dragon is doing: guard / king, the walk to the crown, the queen hunt
#include "turn_move_weights.hpp"  //    the weights and tables the scoring uses
#include "turn_move_score.hpp"    //    one score for every candidate step
#include "turn_move_choose.hpp"   //    navigate when nothing is worth eating, the last checks, the move
    }
