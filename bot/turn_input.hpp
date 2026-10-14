// Sabotage-d / turn_input.hpp, part 1 of the turn: the round's lines parsed (round, facing, length, team size, sonar
// inbox, the 7 x 7 view), tiles / pearls / countdowns and edges into memory, symmetry (SYM), and the window move
// table mv that every later part walks.
// Statements of Brain::turn_(), in the order they run: included once from turn.hpp, inside the function. This part
// reads what the parts before it declared and declares what the parts after it use.

        const Params& Pp = P;
        {
            int rnd0 = (int)pyint(tail_from(at(L, 0), 6));
            if (is_queen && T(Pp.Q_SAFE_UNITS) && !q_units_on && (int)pyint(tail_from(at(L, 3), 11)) >= Pp.Q_SAFE_UNITS) {
                q_units_on = true;          // (v3.9, Q_SAFE_UNITS) enough of us now: the queen plays safe from here on
                q_units_rehome = true;      //   and her home (Q_HOME) is where she is now
            }
            if (is_queen && T(Pp.Q_PMAP) && !q_units_on && portal_edges_seen >= Pp.Q_PMAP &&
                !(T(Pp.Q_PMAP_AREA) && (double)W * H >= Pp.Q_PMAP_AREA)) {
                q_units_on = true;          // (7 Oct, Q_PMAP) a portal maze: the queen plays safe from here on
                q_units_rehome = true;
            }
            if (T(Pp.ARMY) && !is_queen) {   // (8 Oct, ARMY) news of our queen's death
                long long nm1 = pyint(tail_from(at(L, 4), 9));
                if (nm1 > 0) {
                    std::vector<std::string> m1;
                    for (long long k = 5; k < 5 + nm1 && k < (long long)L.size(); k++) m1.push_back(L[(size_t)k]);
                    oqd_in(m1, rnd0);
                }
            }
            if (T(Pp.QDEAD)) {
                long long nm0 = pyint(tail_from(at(L, 4), 9));
                if (nm0 > 0) {
                    std::vector<std::string> m0;
                    for (long long k = 5; k < 5 + nm0 && k < (long long)L.size(); k++) m0.push_back(L[(size_t)k]);
                    qd_in(m0, rnd0);
                }
                if (is_queen && tq_dead && !q_units_on && T(Pp.QDEAD_SAFE)) {   // (8 Oct, QDEAD_SAFE 0: she does not)
                    q_units_on = true;          // (7 Oct, QDEAD) their queen is dead: ours plays safe from here on
                    q_units_rehome = true;      //   and her home (Q_HOME) is where she is now
                }
            }
            qs = is_queen && ((double)W * H >= Pp.Q_AREA || (T(Pp.Q_LATE) && rnd0 >= Pp.Q_LATE) || q_units_on);
        }
        surv_seen = qs && T(Pp.Q_SURV);
        if (T(Pp.SURV_SEEN_LEN) && (double)pyint(tail_from(at(L, 2), 7)) <= Pp.SURV_SEEN_LEN)
            surv_seen = true;                   // (3 Oct, SURV_SEEN_LEN) short dragons too
        qnr = is_queen && ((qs && Pp.Q_SAFE > 0) || T(Pp.Q_NORAM_ALL));
        {
            int rnd0 = (int)pyint(tail_from(at(L, 0), 6));
            qk = T(Pp.Q_KING) && rnd0 >= Pp.Q_KING_FROM && !(T(Pp.Q_KING_AREA) && (double)W * H < Pp.Q_KING_AREA) &&
                 !(T(Pp.Q_KING_SMALL) && (double)W * H >= Pp.Q_KING_SMALL);
            if (qk && !is_queen && T(Pp.Q_SILENT) && rnd0 >= beacon_round + Pp.Q_SILENT && rnd0 - q_last > Pp.Q_SILENT)
                qk = false;                 // (v3q) the queen is presumed dead: the old king rules
        }
        // ------------------------------------------------------------ parse
        int rnd = (int)pyint(tail_from(at(L, 0), 6));
        cur_rnd = rnd;
        mg_now = T(Pp.MG_ROUND) && rnd >= Pp.MG_ROUND && (double)W * H >= Pp.MG_AREA;   // (6 Oct, MG)
        if (mg_now && T(Pp.MG_TQD) && !tq_dead &&
            (Pp.MG_TQD < 2 || (Pp.MG_TQD < 3 && oq_dead) || (qprey_g >= 0 && (double)(rnd - qprey_r) <= Pp.MG_TQD_LEAD)))
            mg_now = false;                 // (9 Oct, MG_TQD) while their queen lives the hunt comes first
        mg_walk = false;
        const std::string& dl = at(L, 1);
        if (dl.size() < 5) throw PyError("DIR line");
        int mydir = didx(dl[4]);
        int length = (int)pyint(tail_from(at(L, 2), 7));
        int ucount = (int)pyint(tail_from(at(L, 3), 11));
        ucount_now = ucount;
        if (T(P.SPRINT_LEARN) && spr_r != -10) {
            int r0 = spr_r, k0 = spr_k, l0 = spr_l0, e0 = spr_e;
            spr_r = -10;
            if (r0 == rnd - 1 && k0 >= 2) {
                int paid = l0 + e0 - length;
                long long dc = (long long)P.SPRINT_DIV;
                if (0 < paid && paid < k0 && fdiv((long long)l0 + dc - 1, dc) != k0 - paid) {
                    // the engine gave me k0 - paid free steps at l0 and my divisor says otherwise: take the divisor
                    // nearest mine that gives that many (several do: 3 and 4 both give a 5-long two)
                    int f0 = k0 - paid;
                    int bd = -1;
                    for (int d0 = 2; d0 <= 16; d0++)
                        if (fdiv((long long)l0 + d0 - 1, d0) == f0 && (bd < 0 || iabs(d0 - (int)dc) < iabs(bd - (int)dc))) bd = d0;
                    if (bd > 0) P.SPRINT_DIV = bd;
                }
            }
        }
        long long nm = pyint(tail_from(at(L, 4), 9));
        std::vector<std::string> msgs;
        if (nm)
            for (long long k = 5; k < 5 + nm && k < (long long)L.size(); k++) msgs.push_back(L[(size_t)k]);
        long long p = 5 + nm;
        if (starts_with(at(L, p), "ECHOES")) p += 1;
        long long tl0 = p;
        p += 49;
        long long nbod = pyint(tail_from(at(L, p), 14));
        p += 1;
        long long bl0 = p;
        p += nbod;
        long long el0 = p;
        auto tl = [&](int i) -> const std::string& { return at(L, tl0 + i); };

        auto hs = split_ws(tl(24));
        if (hs.size() < 2) throw PyError("head tile");
        int hx = (int)pyint(hs[0]);
        int hy = (int)pyint(hs[1]);

        // window index -> global tile index
        int wg[49];
        for (int r = 0; r < 7; r++)
            for (int c = 0; c < 7; c++) wg[r * 7 + c] = (int)fmod(hy - 3 + r, H) * W + (int)fmod(hx - 3 + c, W);
        int hg = wg[24];
        if (home_x < 0) { home_x = hx; home_y = hy; }   // (3 Oct, Q_HOME)
        if (q_units_rehome) { home_x = hx; home_y = hy; q_units_rehome = false; }   // (v3.9, Q_SAFE_UNITS)
        if (T(P.Q_FRESH) && is_queen) {   // (3 Oct, Q_FRESH)
            if (viewed.empty()) viewed.assign((size_t)W * H, -1000);
            for (int i = 0; i < 49; i++) viewed[(size_t)wg[i]] = rnd;
        }
        if (T(Pp.OE_STUCK_K) && !oe_off) {   // (7 Oct, OE_STUCK)
            int dx = iabs(hx - W / 2), dy = iabs(hy - H / 2);
            if (dx > W - dx) dx = W - dx;
            if (dy > H - dy) dy = H - dy;
            if (dx + dy <= Pp.OE_STUCK_R && !(length > my_len && last_eat >= 0)) {
                if (++oe_near >= Pp.OE_STUCK_K) oe_off = true;
            } else {
                oe_near = 0;
            }
        }
        if (length > my_len || last_eat < 0) {
            if (is_queen && T(Pp.Q_HOME_HUNGRY) && last_eat >= 0 && home_x >= 0 && (double)(rnd - last_eat) >= Pp.Q_HOME_HUNGRY) {
                home_x = hx;                // (8 Oct, QHH) the hungry queen found food: this is home now
                home_y = hy;
            }
            last_eat = rnd;                 // (a newborn counts as fed: it is where its parent ate)
            prev_eat = eat2;
            eat2 = rnd;
        }
        if (T(Pp.STUCK_N) && (!T(Pp.STUCK_MAXLEN) || length <= Pp.STUCK_MAXLEN) && rnd < Pp.STUCK_ROUND && !qs) {
            stuck_hist.push_back(hg);
            int n_ = (int)Pp.STUCK_N;
            if ((int)stuck_hist.size() > n_) stuck_hist.erase(stuck_hist.begin());
            if ((int)stuck_hist.size() == n_ && rnd - last_eat >= n_) {
                std::vector<int> u(stuck_hist);
                std::sort(u.begin(), u.end());
                u.erase(std::unique(u.begin(), u.end()), u.end());
                if ((int)u.size() <= 4) {
                    stuck_until = rnd + (int)Pp.STUCK_HOLD;   // (3 Oct, STUCK_N) circling: navigate for a while
                    stuck_hist.clear();
                }
            }
        }
        my_len = length;
        bool hungry = rnd - last_eat >= Pp.HUNGRY;
        bool king_hungry_ = rnd - last_eat >= Pp.KING_HUNGRY_R;      // (for when I am the king)
        bool call_hungry = rnd - last_eat >= (T(Pp.BEACON_HUNGRY_R) ? Pp.BEACON_HUNGRY_R : Pp.KING_HUNGRY_R);
        my_hg = hg;
        if (gi_set)
            for (int i = 0; i < 49; i++) gi_arr[(size_t)gi_wg[i]] = -1;
        for (int i = 0; i < 49; i++) {
            gi_arr[(size_t)wg[i]] = i;
            gi_wg[i] = wg[i];
        }
        gi_set = true;

        // ---- tiles: pearls and countdowns, into memory
        double zone_w = Pp.ZONE_W;
        bool dead_tiles = T(Pp.DEAD_TILES);
        int wp[49], wcd[49];
        for (int i = 0; i < 49; i++) wp[i] = wcd[i] = 0;
        int horizon = 500 - rnd;
        double rush_rich_ = (rush_ok != 3 && !rush_rich) ? Pp.RUSH_RICH : -1;
        bool symp = T(Pp.SYM);
        bool has_newt = false;
        std::vector<int> newt;
        if (symp && sym != -2) {
            has_newt = true;
            for (int i = 0; i < 49; i++)
                if (seen[(size_t)wg[i]] == UNSEEN) newt.push_back(i);
        }
        bool fmem = (T(Pp.FOUNT_MEM) && !(T(Pp.FM_AREA) && (double)W * H >= Pp.FM_AREA)) ||   // (6 Oct, FM_AREA)
                    (T(Pp.FOUNT) && T(Pp.FOUNT_ONLY));
        double fmem_cd = Pp.FOUNT_CD;
        bool gossip = T(Pp.GOSSIP) && rnd < Pp.GOSSIP_STOP;
        double g_hor = 0;
        if (gossip) {
            g_hor = Pp.GOSSIP_HORIZON;
            double g_old = rnd - Pp.GOSSIP_AGE;
            auto& g_rec = gossip_recent;
            if (!g_rec.empty() && g_rec[0].second < g_old) {
                size_t k = 0, n = g_rec.size();
                while (k < n && g_rec[k].second < g_old) k++;
                g_rec.erase(g_rec.begin(), g_rec.begin() + (long)k);
            }
        }
        for (int i = 0; i < 49; i++) {
            // a = tl[i].rsplit(" ", 2)
            const std::string& s = tl(i);
            size_t p2 = s.rfind(' ');
            if (p2 == std::string::npos || p2 == 0) throw PyError("tile line");
            size_t p1 = s.rfind(' ', p2 - 1);
            if (p1 == std::string::npos) throw PyError("tile line");
            std::string a1 = s.substr(p1 + 1, p2 - p1 - 1);
            int cd = (int)pyint(s.substr(p2 + 1));
            int g = wg[i];
            bool pearl = a1 == "1";
            if (gossip && (pearl || (0 <= cd && cd <= g_hor))) gossip_recent.push_back({g, rnd});
            if (pearl) {
                wp[i] = 1;
                wcd[i] = cd;
                if (rush_rich_ >= 0) rush_rich = true;
                if (T(zone_w) && seen[(size_t)g] >= rnd - 3 && pseen[(size_t)g] < rnd - 1) {
                    // a pearl appeared here since I last looked: this water produces
                    zone[(size_t)g] += 2;
                    for (int v : nb(g))
                        if (v >= 0) zone[(size_t)v] += 1;
                }
                if (T(zone_w)) pseen[(size_t)g] = rnd;
                seen[(size_t)g] = rnd;
                hot.set(g, rnd);
                seen_pearl = true;
                last_pearl_seen = rnd;      // (8 Oct, QHHS)
            } else if (cd < 0) {
                wcd[i] = -1;
                seen[(size_t)g] = BARREN_SEEN;
                hot.erase(g);               // a dropped pearl that somebody has eaten since
            } else {
                wcd[i] = cd;
                if (fmem && cd <= fmem_cd) {
                    // (v2.11) a pearl here a round or three ago, and the next one due at once: a fountain
                    if (hot.has(g)) {
                        int pv = hot.get(g, 0);
                        if (rnd - 3 <= pv && pv < rnd) founts.add(g);
                    }
                }
                if (cd <= horizon) {
                    seen[(size_t)g] = rnd;
                    hot.set(g, rnd + cd);
                    if (cd <= rush_rich_) rush_rich = true;
                } else {
                    // due after the game ends: barren as far as this game is concerned
                    seen[(size_t)g] = dead_tiles ? BARREN_SEEN : rnd;
                    hot.erase(g);
                }
            }
        }

        if (T(Pp.SPRINT_LEARN)) {
            spr_pearls.clear();
            for (int i = 0; i < 49; i++)
                if (wp[i]) spr_pearls.push_back(wg[i]);
        }
        if (gossip) {
            if (!msgs.empty() && !T(Pp.GOSSIP_DEAF)) gossip_in(msgs, rnd);
            gossip_out = gossip_val(rnd);
        }
        if (rush_ok < 0) {
            // (v2.10) my first look: is this poor water?
            int ok = T(Pp.RUSH_ROUND) ? 1 : 0;
            if (is_queen && T(Pp.Q_NOROLE)) ok = 0;     // (v3q) the queen does not rush the middle
            if (ok && Pp.RUSH_POOR >= 0) {
                double due = Pp.RUSH_DUE;
                int n = 0;
                for (int i = 0; i < 49; i++) {
                    int cd = wcd[i];
                    if (0 <= cd && cd <= due) n++;
                }
                if (n > Pp.RUSH_POOR) ok = 0;
            }
            if (ok && Pp.RUSH_SHARE < 1.0) {
                int idx = id >> 1;
                int n0;
                if (length == 2) {
                    n0 = (int)fdiv(ucount, 2);
                    if (n0 < 1) n0 = 1;
                    idx -= n0;
                    if (!T(Pp.RUSH_KIDS)) idx = 9999;
                } else {
                    n0 = ucount - (length >= 4 ? idx : 0);
                    if (n0 < 1) n0 = 1;
                }
                int n_rush = (int)(n0 * Pp.RUSH_SHARE + 0.5);
                if (n_rush < 1) n_rush = 1;
                if (T(Pp.RUSH_LAST) && idx < 9999) idx = n0 - 1 - idx;
                if (rnd > 1 || idx < 0 || idx >= n_rush) ok = 0;
            }
            int tx = (int)fdiv(W, 2), ty = (int)fdiv(H, 2);
            if (T(Pp.RUSH_NEAR)) {
                int a = iabs(hx - (int)fdiv(W - 1, 2));
                int b = iabs(hx - (int)fdiv(W, 2));
                if (a > W - a) a = W - a;
                if (b > W - b) b = W - b;
                if (a < b) tx = (int)fdiv(W - 1, 2);
                a = iabs(hy - (int)fdiv(H - 1, 2));
                b = iabs(hy - (int)fdiv(H, 2));
                if (a > H - a) a = H - a;
                if (b > H - b) b = H - b;
                if (a < b) ty = (int)fdiv(H - 1, 2);
            }
            rush_target = ty * W + tx;
            if (ok && T(Pp.RUSH_MAXD)) {
                int dx = iabs(hx - tx), dy = iabs(hy - ty);
                if (dx > W - dx) dx = W - dx;
                if (dy > H - dy) dy = H - dy;
                if (dx + dy > Pp.RUSH_MAXD) ok = 0;
            }
            rush_ok = ok;
            if (rnd == 0 && T(Pp.OE_NR)) {
                // (6 Oct, OE_NR) the starters are ids (id >> 1) = 0, 1, ...: the first RUSH_SHARE of them are the middle's
                int idx = id >> 1, n0 = ucount > 0 ? ucount : 1;
                int n_rush = (int)(n0 * Pp.RUSH_SHARE + 0.5);
                if (n_rush < 1) n_rush = 1;
                if (T(Pp.RUSH_LAST)) idx = n0 - 1 - idx;
                oe_nr = idx >= n_rush ? 1 : 0;
                // OE_NR_TWO: only the second of exactly two starters (ids 2 / 3, at least 3 long, the team 2 strong at
                // my first look) - not a third or fourth starter, not a newborn of a round-0 split
                if (T(Pp.OE_NR_TWO)) oe_nr = (ucount <= 2 && (id == 2 || id == 3) && length >= 3) ? 1 : 0;
                // (7 Oct) OE_NR_TWO 2: the other way round - the queen (ids 0 / 1) explores at home, the second starter
                // takes the pull to the middle
                if (Pp.OE_NR_TWO == 2) oe_nr = (ucount <= 2 && (id == 0 || id == 1) && length >= 3) ? 1 : 0;
                if (T(Pp.OE_NR_AREA) && (double)W * H < Pp.OE_NR_AREA) oe_nr = 0;   // a tiny map: home is the centre
            }
            rush_home = hg;
        }

        // ---- edges (only kelp and portals are stored; everything else is open)
        bool plain = record_edges(L, el0, wg);
        if (ora_state >= 0) ora_check(L, el0, wg, wcd, rnd);   // (8 Oct, ORA)
        if (!Pp.cheat_portals.empty() && !cheated) {
            // (harness only) the whole portal network, as if every dragon had seen all of it
            cheated = true;
            for (auto& e : Pp.cheat_portals) edge_(e[0], e[1], std::to_string(e[2]));
            plain = false;
        }

        // ---- (v2.8) every map is a mirror image of itself (SYM)
        if (has_newt) sym_learn(rnd, wg, wcd, wp, newt, horizon);
        if (pos_learn && !ora_rates_on) {
            // (v3) what this view says of the rates here - and, once the symmetry is known, over there
            int symc = sym;
            if (symc >= 0 && !pos_mirrored) {
                pos_mirrored = true;
                for (int g = 0; g < N; g++) {
                    if (pos_rate[(size_t)g] >= 0) {
                        int m = mir(symc, g);
                        if (m != g && pos_rate[(size_t)m] < 0) pos_set(m, pos_rate[(size_t)g]);
                    }
                }
            }
            for (int i = 0; i < 49; i++) {
                int g = wg[i];
                pos_obs(g, wp[i], wcd[i], rnd, horizon);
                if (symc >= 0) {
                    int m = mir(symc, g);
                    if (m != g && gi_get(m) < 0) pos_obs(m, wp[i], wcd[i], rnd, horizon);
                }
            }
        }

        // ---- window move table: mv[i*4+d] = window index | -1 kelp | -2 leaves the window | -3 blind portal
        int mv_buf[196];
        int ext_buf[49];
        const int* mv;
        const int* ext;
        if (plain) {
            mv = BASE;
            ext = RING;
        } else {
            for (int i = 0; i < 49; i++) {
                ext_buf[i] = 0;
                auto& nl = nb(wg[i]);
                for (int d = 0; d < 4; d++) {
                    int v = nl[(size_t)d];
                    if (v >= 0) {
                        int gv = gi_get(v);
                        v = gv >= 0 ? gv : -2;
                        if (v == -2) ext_buf[i] = 1;
                    }
                    mv_buf[i * 4 + d] = v;
                }
            }
            mv = mv_buf;
            ext = ext_buf;
        }
