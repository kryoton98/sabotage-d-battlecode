// Sabotage-d / turn_split.hpp, part 6 of the turn: "2. split?" - the opening cascade (CASC) and the routine split.
// The member functions it calls are in split.hpp.
// Statements of Brain::turn_(), in the order they run: included once from turn.hpp, inside the function. This part
// reads what the parts before it declared and declares what the parts after it use.

        // ------------------------------------------------------ 2. split?
        double cap_free = (rnd >= Pp.CAP_FREE_ROUND && !lead_) ? Pp.CAP_FREE : 0;
        // (9 Oct, CASC) the opening cascade: everything behind my front two goes to the newborn, which acts later this
        // same round and does the same - a long starter is all 2-longs (one 3-long if odd) before anybody has moved
        bool casc = false;
        if (T(Pp.CASC) && rnd <= Pp.CASC && length >= 5 && (!is_queen || T(Pp.CASC_QUEEN))) {
            casc = true;
            if (T(Pp.CASC_NOQ) && !is_queen)
                for (auto& h : heads)
                    if (!h.en && h.did == qid) casc = false;        // our queen is right here: do not box her in with stubs
        }
        if (T(Pp.SPLIT) && length >= Pp.SPLIT_MIN && ucount < limit - cap_free && !qhthr && !lane_nosplit && (!nb_hold || casc) && !ff_hold &&
            want_split(rnd, length, late_round, ucount) &&
            (seen_pearl || rnd >= Pp.SPLIT_WAIT) &&
            (!T(Pp.SPLIT_WINDOW) || rnd - prev_eat <= Pp.SPLIT_WINDOW || ucount < Pp.SPLIT_WINDOW_MIN)) {
            int child = ucount * 2 <= limit ? (int)fdiv(length, 2) : (int)Pp.SPLIT_BUDLEN;
            if (T(Pp.SPLIT_HALF)) child = (int)fdiv(length, 2);          // (8 Oct, SPLIT_HALF) always halve
            if (T(Pp.SPLIT_41)) child = split41(length, child);         // (8 Oct, SPLIT_41) 4k+1 sizes
            if (T(Pp.BUD) && ucount < Pp.BUD) child = (int)Pp.SPLIT_BUDLEN;   // (9 Oct, BUD) heads first: shed the 2-long tail
            if (casc) child = length - 2;                                // (9 Oct, CASC)
            if (length - child < 2) child = length - 2;
            bool ok = true;
            bool keep_off = false;
            if (T(Pp.Q_KEEP_RICH) && rnd - last_enemy > Pp.Q_KEEP_CALM) {   // (3 Oct, Q_KEEP_RICH)
                int npk = 0;
                for (int i = 0; i < 49; i++) npk += wp[i];
                keep_off = npk >= Pp.Q_KEEP_RICH;
            }
            if (qs && Pp.Q_KEEP > 0 && length - child < Pp.Q_KEEP && !keep_off && !(T(Pp.Q_KEEP_FROM) && rnd < Pp.Q_KEEP_FROM)) {
                child = length - (int)Pp.Q_KEEP;     // (v3q) the queen keeps Q_KEEP segments
                ok = child >= 2;
            }
            if (is_queen && Pp.Q_MINLEN > 0 && length - child < Pp.Q_MINLEN) {
                child = length - (int)Pp.Q_MINLEN;   // (v3q) on every map the queen keeps Q_MINLEN segments
                ok = ok && child >= 2;
            }
            if (T(Pp.SPLIT_FAST) && ucount >= Pp.SPLIT_FAST && !is_queen) {
                // (7 Oct, SPLIT_FAST) both parts fast: wait until each can have SPLIT_FAST_LEN segments
                const int fl = (int)Pp.SPLIT_FAST_LEN;
                if (child < fl) child = fl;
                if (length - child < fl) ok = false;
            }
            if (ok && T(Pp.PORTAL_LAND_SPLIT) && tn >= length && has_ptiles && ptile(trail[(size_t)(length - 1)]) != nullptr)
                ok = false;   // (7 Oct, PORTAL_LAND_SPLIT) the newborn would start on a landing tile
            if (ok && tn >= length) {
                int ti = gi_get(trail[(size_t)(length - 1)]);
                if (ti >= 0) {
                    // the child is born on my tail and must move this very round
                    ok = false;
                    int nfree = 0, nsafe = 0;
                    for (int k = ti * 4; k < ti * 4 + 4; k++) {
                        int v = mv[k];
                        if (v == -2 || (v >= 0 && !occ[v])) {
                            ok = true;
                            nfree++;
                            if (v == -2 || headadj[v] == 0) nsafe++;
                        }
                    }
                    if (ok && T(Pp.SPLIT_CHILD_SAFE) && nsafe == 0 && nfree < 2) ok = false;
                }
            }
            if (ok && T(Pp.SPLIT_KID) && tn >= length && gi_get(trail[(size_t)(length - 1)]) >= 0) {
                // (7 Oct, SPLIT_KID) the newborn needs a way out: its head is on my tail and it moves later this round. From a
                // free neighbour of that tile it needs a run of SPLIT_KID steps; my kept part and every free neighbour of my head
                // (I move next round) are walls, its own body frees up from its tail
                const int need_k = (int)Pp.SPLIT_KID;
                kid_w.clear();
                for (int g : others.items) kid_w.add(g);
                for (int k = 0; k < length - child; k++) kid_w.add(trail[(size_t)k]);
                {
                    auto nbh = nb(hg);
                    for (int e = 0; e < 4; e++) {
                        const int t = nbh[(size_t)e];
                        if (t >= 0 && !body_at.has(t)) kid_w.add(t);
                    }
                }
                kid_bat.clear();
                for (int j = 0; j < child; j++) kid_bat.set(trail[(size_t)(length - 1 - j)], child - j + 1);
                const int kh = trail[(size_t)(length - 1)];
                bool okk = false;
                work = (long long)Pp.WORK;
                auto nbk = nb(kh);
                for (int e = 0; e < 4 && !okk; e++) {
                    const int t = nbk[(size_t)e];
                    if (t < 0 || kid_w.has(t) || kid_bat.has(t)) continue;
                    if (survive(t, need_k, kid_bat, 0, child, &kid_w, nullptr) >= need_k) okk = true;
                }
                kid_w.clear();
                if (!okk) ok = false;
            }
            if (ok && T(Pp.SPLIT_SAFE)) {
                // I do not move this turn: my head needs a way out that nobody can take first
                // (v3.9, SPLIT_PEXIT) the newborn too: its head is on my tail and it moves later this round
                const int cti = (T(Pp.SPLIT_PEXIT) && tn >= length) ? gi_get(trail[(size_t)(length - 1)]) : -1;
                int safe = 0, free = 0;
                for (int d = 0; d < 4; d++) {
                    int v = mv[d + 96];
                    if (v == -2) safe++;
                    else if (v >= 0 && !occ[v]) {
                        free++;
                        bool kid_near = false;
                        if (cti >= 0)
                            for (int k = cti * 4; k < cti * 4 + 4; k++)
                                if (mv[k] == v) kid_near = true;
                        if (headadj[v] == 0 && !kid_near) safe++;
                    }
                }
                if (safe == 0 && free < 2) ok = false;
            }
            if (ok && Pp.SPLIT_PEXIT >= 2 && (Pp.SPLIT_PEXIT < 3 || is_queen) && tn >= length) {   // 3: the run check for the queen only
                // (v3.9, SPLIT_PEXIT 2) after the split the newborn's body and every tile its head can step to are walls: I
                // stand still this turn, then I need a run of SPLIT_PEXIT_RUN steps from one of my free neighbours
                pexit_w.clear();
                for (int g : others.items) pexit_w.add(g);
                for (int k = length - child; k < length; k++) pexit_w.add(trail[(size_t)k]);
                const int chg = trail[(size_t)(length - 1)];
                for (int e = 0; e < 4; e++) {
                    const int t = nb(chg)[(size_t)e];
                    if (t >= 0) pexit_w.add(t);
                }
                const int need_r = (int)Pp.SPLIT_PEXIT_RUN;
                bool okp = false;
                work = (long long)Pp.WORK;
                auto nbh = nb(hg);
                for (int d = 0; d < 4 && !okp; d++) {
                    const int v = mv[d + 96];
                    if (v == -2) {
                        okp = true;
                        break;
                    }
                    if (v < 0 || occ[v] || freeat[v] > 1) continue;
                    const int g2 = nbh[(size_t)d];
                    if (g2 < 0 || pexit_w.has(g2)) continue;
                    if (survive(g2, need_r, body_at, 0, length - child, &pexit_w, nullptr) >= need_r) okp = true;
                }
                pexit_w.clear();
                if (!okp) ok = false;
            }
            if (ok && T(Pp.SPLIT_DEAD)) {
                int hl = length - child;
                int need_s = (int)Pp.SPLIT_DEAD;
                bool okm = false;
                work = (long long)Pp.WORK;
                auto nbh = nb(hg);
                for (int d = 0; d < 4; d++) {
                    int v = mv[d + 96];
                    if (v == -2) {
                        okm = true;
                        break;
                    }
                    if (v < 0 || occ[v] || freeat[v] > 1) continue;
                    int g2 = nbh[(size_t)d];
                    if (g2 < 0) continue;
                    if (survive(g2, need_s, body_at, 0, hl, &others, nullptr) >= need_s) {
                        okm = true;
                        break;
                    }
                }
                if (!okm) ok = false;
            }
            if (ok && T(Pp.SPLIT_ROOM)) {
                int area = 0;
                int seen_w[49] = {0};
                seen_w[24] = 1;
                std::vector<int> q{24};
                for (size_t qi = 0; qi < q.size(); qi++) {
                    int u = q[qi];
                    for (int k = u * 4; k < u * 4 + 4; k++) {
                        int x = mv[k];
                        if (x >= 0 && !seen_w[x] && !occ[x]) {
                            seen_w[x] = 1;
                            q.push_back(x);
                            area += ext[x] ? 3 : 1;
                        }
                    }
                }
                if (area < Pp.SPLIT_ROOM) ok = false;
            }
            if (ok) return "SPLIT " + std::to_string(child);
        }
