// Sabotage-d / turn_move_choose.hpp, part 11 of the turn: after the scores - the route when nothing in view is worth
// eating (navigate), the pulls added on top (CHASE, QG, FF), the queen's last checks (Q_HARD, QSAVE, Q_ESC,
// Q_END_STILL), the ways out when every step looks fatal, the free sprint, and the move itself.
// Statements of Brain::turn_(), in the order they run: included once from turn.hpp, inside the function. This part
// reads what the parts before it declared and declares what the parts after it use.

        if (goal_d >= 0) {
            for (auto& sc : scored)
                if (sc.d == goal_d) sc.s += Pp.FOOD_STICK;
        }

        double gate = Pp.NAV_GATE;
        if (assn_ && Pp.ASSN_GATE > gate) gate = Pp.ASSN_GATE;
        if (qhunt && Pp.Q_HUNT_GATE > gate) gate = Pp.Q_HUNT_GATE;     // (v3q) nor a queen hunter
        if (guard_now && rnd >= guard_round + Pp.GUARD_GATE_FROM && Pp.GUARD_GATE > gate &&
            !(T(Pp.GUARD_GATE_AREA) && (double)W * H < Pp.GUARD_GATE_AREA))   // (v3.9, GUARD_GATE_AREA) big maps only
            gate = Pp.GUARD_GATE;
        if (mg_walk && Pp.MG_GATE > gate) gate = Pp.MG_GATE;   // (6 Oct, MG) a walker to the crown eats only what is close
        if (((T(Pp.NAV_ONE) ? food_one : food_best) < gate || rnd < stuck_until ||
             (T(Pp.RUSH_FORCE) && rush_ok == 1 && T(Pp.RUSH_ROUND) && rnd < Pp.RUSH_ROUND)) && !(i_am_king && T(Pp.KING_HOME))) {
            // nothing worth eating in sight: head for remembered pearls / unseen water
            nav_w = 0.5;
            has_nav_friends = !friends.empty() && T(Pp.FRIEND_CLAIM) && (double)friends.size() >= Pp.FRIEND_CLAIM_N &&   // (FRIEND_CLAIM_N: a crowd)
                              !(T(Pp.FRIEND_CLAIM_HUNGRY) && last_eat >= 0 && (double)(rnd - last_eat) < Pp.FRIEND_CLAIM_HUNGRY);   // (8 Oct, FCH)
            nav_friends.clear();
            if (has_nav_friends)
                for (int fi : friends) nav_friends.push_back(wg[fi]);
            nav_hold = -1;
            nav_noport = is_queen && q_noport(rnd);
            if (T(Pp.LURE_MIN)) {
                // (7 Oct, LURE) food in view that I cannot reach inside my view: it is walled off from me
                int lseen[49], lq[49], lqn = 0;
                for (int i = 0; i < 49; i++) lseen[i] = 0;
                lseen[24] = 1;
                lq[lqn++] = 24;
                for (int qi = 0; qi < lqn; qi++) {
                    const int u = lq[qi];
                    for (int k = u * 4; k < u * 4 + 4; k++) {
                        const int v = mv[k];
                        if (v >= 0 && !lseen[v] && !occ[v]) {
                            lseen[v] = 1;
                            lq[lqn++] = v;
                        }
                    }
                }
                int nl = 0;
                for (int i = 0; i < 49; i++)
                    if (!lseen[i] && !occ[i] && (wp[i] || (wcd[i] > 0 && (double)wcd[i] <= Pp.LURE_DUE))) nl++;
                if ((double)nl >= Pp.LURE_MIN) lure_until = rnd + (int)Pp.LURE_MEM;
            }
            const bool lure_len = T(Pp.LURE_MIN) && rnd <= lure_until && length <= Pp.LURE_MAXLEN && !is_queen;
            const bool hb_len = lure_len || (T(Pp.HB_ROUNDS) && T(Pp.HB_MAXLEN) && length <= Pp.HB_MAXLEN && rnd - last_eat >= Pp.HB_ROUNDS &&
                                (!is_queen || T(Pp.HB_QUEEN)) && !(T(Pp.HB_UNTIL) && rnd >= Pp.HB_UNTIL));   // (7 Oct, HB_MAXLEN)
            int nd = navigate(hg, scored, occ, rnd, (length <= 3 || hb_len) && ucount >= 2 && !(is_queen && (q_noblind(rnd) || q_noport(rnd))), mydir);
            nav_noport = false;
            last_nav = nd;
            if (assn_ && nav_w < Pp.ASSN_W) nav_w = Pp.ASSN_W;
            if (qhunt && nav_w < Pp.Q_HUNT_W) nav_w = Pp.Q_HUNT_W;
            if (mg_walk && nav_w < Pp.MG_NAV_W) nav_w = Pp.MG_NAV_W;   // (6 Oct, MG)
            if (rnd < stuck_until && nav_w < Pp.STUCK_W) nav_w = Pp.STUCK_W;   // (3 Oct, STUCK_N)
            for (auto& sc : scored) {
                if (sc.d == nd) {
                    double nw = nav_w;
                    if (nw > 0.5 && T(Pp.RUSH_SAFE) && grade && sc.v >= 0 && tpen[sc.v] > 0.0) nw = 0.5;
                    sc.s += nw;
                    if (sc.s < -500.0 && T(Pp.GOAL_BAN) && last_goal >= 0) {
                        // the way to what I was heading for starts with a fatal step: not worth planning for
                        banned[(size_t)last_goal] = rnd + (int)Pp.GOAL_BAN;
                        has_route = false;
                        route.clear();
                    }
                }
            }
            int nh = nav_hold;
            if (nh >= 0) {
                double hold_w = Pp.PORTAL_HOLD_W;
                int hm = nav_hold_m;
                int nx = nh % W, ny = nh / W;
                for (auto& sc : scored) {
                    int g2 = sc.g2;
                    if (g2 < 0) continue;
                    if (g2 == nh) {
                        sc.s -= hold_w;
                        continue;
                    }
                    int dx = iabs(g2 % W - nx), dy = iabs(g2 / W - ny);
                    if (dx > W - dx) dx = W - dx;
                    if (dy > H - dy) dy = H - dy;
                    int dd = dx + dy;
                    if (dd <= hm && (dd & 1) == (hm & 1)) sc.s += hold_w;
                }
            }
        }
        // (v3.9, CHASE) a short dragon with a friendly head close by closes on an equal or longer enemy head in view (the ram
        // itself stays try_kill's call); after CHASE_TURNS rounds, or when the target or the friend is gone, it leaves that
        // enemy alone for CHASE_COOL rounds
        if (T(Pp.CHASE) && !is_queen && length >= Pp.CHASE_MIN && length <= Pp.CHASE_MAX && !scored.empty()) {
            bool fnear = false;
            for (int fi : friends)
                if (iabs(ROWS[fi] - 3) + iabs(COLS[fi] - 3) <= 3) {
                    fnear = true;
                    break;
                }
            int tgt = -1;
            if (chase_id >= 0 && rnd - chase_from < Pp.CHASE_TURNS && fnear)
                for (auto& en : enemies)
                    if (en.did == chase_id) tgt = en.ei;
            if (tgt < 0 && chase_id >= 0) {
                chase_cool[chase_id] = rnd + (int)Pp.CHASE_COOL;
                chase_id = -1;
            }
            if (tgt < 0 && fnear) {
                int bestd = 99;
                for (auto& en : enemies) {
                    if (en.ln < length) continue;
                    auto it = chase_cool.find(en.did);
                    if (it != chase_cool.end() && rnd < it->second) continue;
                    int theirs = 0, ours = 1;   // heads within 3 of the target: not where theirs outnumber ours
                    for (auto& e2 : enemies)
                        if (iabs(ROWS[e2.ei] - ROWS[en.ei]) + iabs(COLS[e2.ei] - COLS[en.ei]) <= 3) theirs++;
                    for (int fi : friends)
                        if (iabs(ROWS[fi] - ROWS[en.ei]) + iabs(COLS[fi] - COLS[en.ei]) <= 3) ours++;
                    if (theirs > ours) continue;
                    const int dd = iabs(ROWS[en.ei] - 3) + iabs(COLS[en.ei] - 3);
                    if (dd < bestd) {
                        bestd = dd;
                        tgt = en.ei;
                    }
                }
                if (tgt >= 0) {
                    for (auto& en : enemies)
                        if (en.ei == tgt) chase_id = en.did;
                    chase_from = rnd;
                }
            }
            if (tgt >= 0) {
                const int d0 = iabs(ROWS[tgt] - 3) + iabs(COLS[tgt] - 3);
                for (auto& sc : scored) {
                    if (sc.v < 0 || sc.s <= -500.0) continue;
                    if (iabs(ROWS[sc.v] - ROWS[tgt]) + iabs(COLS[sc.v] - COLS[tgt]) < d0) sc.s += Pp.CHASE;
                }
            }
        }
        if (qres_on && !scored.empty()) {          // (8 Oct, QRES) keep off the pearls at our queen's head
            for (auto& sc : scored) {
                if (sc.v < 0 || sc.s <= -500.0 || !wp[sc.v]) continue;
                if (iabs(ROWS[sc.v] - ROWS[qhead_i]) + iabs(COLS[sc.v] - COLS[qhead_i]) <= Pp.QRES_R) sc.s -= Pp.QRES;
            }
        }
        if (T(Pp.Q_SPACE) && !is_queen && qhead_i >= 0 && !scored.empty()) {   // (8 Oct, QSPC) room for our queen
            int qdid = -1;
            for (auto& h : heads)
                if (h.i == qhead_i) qdid = h.did;
            if ((double)seg_n.get(qdid, 0) >= Pp.Q_SPACE_MINQ) {
                for (auto& sc : scored) {
                    if (sc.v < 0 || sc.s <= -500.0) continue;
                    const int dd = iabs(ROWS[sc.v] - ROWS[qhead_i]) + iabs(COLS[sc.v] - COLS[qhead_i]);
                    if ((double)dd <= Pp.Q_SPACE_R) sc.s -= Pp.Q_SPACE;
                }
            }
        }
        if (qg_ei >= 0 && !scored.empty()) {       // (8 Oct, QG) close on the enemy head at our queen
            const int d0 = iabs(ROWS[qg_ei] - 3) + iabs(COLS[qg_ei] - 3);
            for (auto& sc : scored) {
                if (sc.v < 0 || sc.s <= -500.0) continue;
                if (iabs(ROWS[sc.v] - ROWS[qg_ei]) + iabs(COLS[sc.v] - COLS[qg_ei]) < d0) sc.s += Pp.QG;
            }
            if (T(Pp.DEBUG)) dbg_note += "qg ";
        }
        if (ff_walk_dir >= 0) {                     // (8 Oct, FF) the walk to the exit of the dense water I am in
            for (auto& sc : scored)
                if (sc.d == ff_walk_dir && sc.s > -500.0) sc.s += Pp.FF_NAV_W;
            if (T(Pp.DEBUG)) dbg_note += "ff-walk ";
        }
        if (T(Pp.FF) && rnd < ff_out_until && T(Pp.FF_OUT_PEN)) {   // (8 Oct, FF) the parent of a flip goes on out
            for (auto& sc : scored)
                if (sc.g2 >= 0 && sc.s > -500.0 && ff_dense(sc.g2)) sc.s -= Pp.FF_OUT_PEN;
        }
        if (Pp.DEBUG >= 2) {
            // (py: self.dbg) the scores before the jitter, for tools/rparity_dbg.py
            fprintf(stderr, "DBG %d id%d", rnd, id);
            for (auto& sc : scored) fprintf(stderr, " [%.17g %d %d %d %d]", sc.s, sc.d, sc.v, sc.g2, sc.kind);
            fprintf(stderr, "\n");
        }
        if (T(Pp.Q_HARD) && qs && grade && !enemies.empty()) {
            // (8 Oct, Q_HARD) the queen: when some step ends out of every enemy's reach (and is not fatal), a step into a
            // reach is not an option (ladder: in 30-40 % of our queen's rammed deaths on 1,100+ maps such a step existed)
            bool safe_exists = false;
            for (auto& sc : scored)
                if (sc.s > -500.0 && sc.v >= 0 && !treach[sc.v]) safe_exists = true;
            if (safe_exists)
                for (auto& sc : scored)
                    if (sc.v >= 0 && treach[sc.v] && sc.s > -500.0) sc.s -= Pp.Q_HARD;
        }
        const Scored* best = nullptr;
        double best_s = -1e18;
        for (auto& sc : scored) {
            double s = sc.s + rand01() * 0.01;
            if (s > best_s) {
                best_s = s;
                best = &sc;
            }
        }
        if (T(Pp.QSAVE) && !is_queen && qhead_i >= 0 && best != nullptr) {
            // (7 Oct, QSAVE) our queen in my view must keep a legal step: my head on her last free neighbour (a corridor
            // met from both ends) boxes her in - take another step, or give my life so that she can walk on
            auto q_ways = [&](int myv) -> int {
                int n = 0;
                for (int k = qhead_i * 4; k < qhead_i * 4 + 4; k++) {
                    const int x = mv[k];
                    if (x <= -2) n++;
                    else if (x >= 0 && !occ[x] && x != myv) n++;
                }
                return n;
            };
            if (q_ways(-1) > 0 && best->kind == 0 && best->v >= 0 && q_ways(best->v) == 0) {
                const Scored* alt = nullptr;
                double alt_s = -1e18;
                for (auto& sc : scored) {
                    if (&sc == best || (sc.kind == 0 && sc.v >= 0 && q_ways(sc.v) == 0)) continue;
                    if (sc.s > alt_s) {
                        alt_s = sc.s;
                        alt = &sc;
                    }
                }
                if (alt != nullptr && alt_s > -500.0) {
                    best = alt;
                    best_s = alt_s;
                } else {
                    for (int dd = 0; dd < 4; dd++)
                        if (mv[dd + 96] == -1) {
                            if (T(Pp.DEBUG)) return std::string("INDICATOR qsave\nMOVE ") + DCH[dd];
                            return std::string("MOVE ") + DCH[dd];
                        }
                }
            }
        }
        if ((qs || (is_queen && T(Pp.Q_ESC_SMALL) && Pp.Q_SAFE_SMALL > 0)) && T(Pp.Q_ESC) && grade && !enemies.empty() &&
            best != nullptr && tn >= length &&
            (best_s <= -500.0 || (best->v >= 0 && treach[best->v]))) {
            // (v3q) the queen's best step ends within a rammer's reach (or every step looks fatal): sprint out of it
            std::string alt;
            if (escape_sprint(*best, mv, freeat, length, tpen, treach, threat_w, need_depth, wg, hg, alt,
                              (best_s <= -500.0 || best->v < 0) ? 1 : 0))
                return alt;
        }
        if (T(Pp.ESC_SPRINT) && grade && length >= Pp.ESC_SPRINT_LEN && !enemies.empty() && best != nullptr &&
            best->v >= 0 && (T(Pp.ESC_TREACH) ? treach[best->v] != 0 : threat_w * tpen[best->v] >= Pp.ESC_SPRINT_PEN) &&
            best_s > -500.0 && tn >= length) {
            std::string alt;
            if (escape_sprint(*best, mv, freeat, length, tpen, treach, threat_w, need_depth, wg, hg, alt)) return alt;
        }
        if (T(Pp.ESC_LEN) && length >= Pp.ESC_LEN && !enemies.empty() && ucount < limit && best_s > -500.0 &&
            !(T(Pp.ESC_LATE) && rnd >= Pp.ESC_LATE) && !qnp) {
            // (v2.8) hunted: split off everything but two segments and the body walks away from the tail end
            double tmin = 1e9;
            bool esc_ok = true;
            for (auto& sc : scored) {
                if (sc.s > -500.0 && sc.v >= 0) {
                    double tp = grade ? tpen[sc.v] : 0.0;
                    if (tp < tmin) tmin = tp;
                    if (T(Pp.ESC_REAL) && !treach[sc.v]) esc_ok = false;
                }
            }
            if (esc_ok && tmin * threat_w >= Pp.ESC_PEN) {
                std::string alt;
                if (escape_split(length, tn, mv, occ, heads, rnd, alt)) return alt;
            }
        }
        if (q_end_now && qs && grade && (double)length >= Pp.Q_END_STILL_LEN && ucount < limit && best != nullptr &&
            best_s > -500.0 && !qhthr && !(best->v >= 0 && wp[best->v])) {
            // (8 Oct, Q_END_STILL) the last rounds: a long queen with (almost) no way on stands still where nobody in view reaches her
            int nok = 0, own = 0;
            for (auto& sc : scored)
                if (sc.s > -500.0) nok++;
            for (int k = 96; k < 100; k++)
                if (mv[k] >= 0 && occ[mv[k]] == 3) own++;   // coiled: my own body on 2+ sides of my head (not a corridor)
            if ((double)nok <= Pp.Q_END_STILL_EXITS && own >= 2) {
                if (T(Pp.DEBUG)) return "INDICATOR qendstill\nSPLIT 2";
                return "SPLIT 2";
            }
        }
        if (best_s < -500.0) {
            // every move looks fatal
            if (!finish.empty() && !qnp) {
                // (v2.11) but one of them is a pearl in the dead end I am in: eat it first
                bool have = false;
                double fb_pk = 0;
                double fb_s = 0;
                int fb_d = -1;
                for (auto& f : finish) {
                    for (auto& sc : scored) {
                        if (sc.d == f.d && sc.s + f.pen > -500.0) {
                            double sv = sc.s + f.pen;
                            if (!have || f.pk > fb_pk || (f.pk == fb_pk && sv > fb_s)) {
                                have = true;
                                fb_pk = f.pk;
                                fb_s = sv;
                                fb_d = f.d;
                            }
                        }
                    }
                }
                if (have) return emit_move({fb_d}, hg, "finish");
            }
            if (T(Pp.FATAL_OK)) {
                // (v2.14) a move that keeps me going this long beats standing still at two segments a round
                double okn = Pp.FATAL_OK;
                if (okn > 500 - rnd) okn = 500 - rnd;
                bool have = false;
                int bc_sv = 0;
                double bc_s = 0;
                int bc_d = -1;
                for (auto& sc : scored) {
                    int sv = surv_of[sc.d] == NOVAL ? -1 : surv_of[sc.d];
                    if (sv >= okn && (!have || sv > bc_sv || (sv == bc_sv && sc.s > bc_s))) {
                        have = true;
                        bc_sv = sv;
                        bc_s = sc.s;
                        bc_d = sc.d;
                    }
                }
                if (have) return emit_move({bc_d}, hg, "fatal-ok");
            }
            std::string alt;
            bool got_alt = !qnp && reverse_split(length, ucount, tn, mv, occ, alt);
            if (!got_alt) {
                alt = boxed_in(mv, occ, heads, length, ucount, -1, tn);
                got_alt = !alt.empty();
            }
            if (got_alt) return alt;
        }
        food_goal = best_t[best->d];
        traversal_allowed = !is_queen && enemies.empty() && !guard_now && !mg_now && !mg_walk &&
                            !qhunt && !assn_ && king_i_ < 0 && defer_i < 0 && best_s > -500.0;
        if (T(Pp.FREE_SPRINT) && T(Pp.SPRINT_NEW) && best->v >= 0 && best_s > -500.0 && tn >= length &&
            !(qs && T(Pp.Q_NOFS)) && !(T(Pp.FS_MAXLEN) && length >= Pp.FS_MAXLEN) &&
            !(T(Pp.FS_QUIET) && length >= Pp.FS_QUIET && !enemies.empty())) {
            std::vector<int> path;
            if (free_sprint(*best, mv, freeat, occ, length, tpen, treach, threat_w, grade, need_depth, wg, wp, path))
                return emit_move(path, hg, "fs");
        }
        return emit_move({best->d}, hg, "");
