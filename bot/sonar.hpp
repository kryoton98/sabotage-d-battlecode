// Sabotage-d / sonar.hpp: the 64-bit words a team's dragons send each other. *_val builds a word, *_in reads the
// words of this turn's inbox: the king's beacon (beacon_*), gossip (gossip_*), threat warnings (tw_*), "their queen
// is dead" / "ours is" (qd_*, oqd_*), and the team's contact estimate (reg_update). One word goes out a turn, in
// all four directions: turn() (turn.hpp) chooses which.
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    uint64_t beacon_val(int g, long long ln, long long age, long long kind = 0, long long hungry = 0,
                        long long did = 0) const {
        uint64_t x = (uint64_t)fmod(g, W);
        uint64_t y = (uint64_t)fdiv(g, W);
        if (age > 7) age = 7;
        if (T(P.BEACON64)) {
            if (ln > 255) ln = 255;
            did &= 0xFFFF;
            long long star = (hungry >> 1) & 1;      // bit 1 of hungry: starving
            long long danger = (hungry >> 2) & 1;    // (v2.15) bit 2: in contact with the enemy
            hungry &= 1;
            uint64_t tm = (uint64_t)tbit;
            uint64_t L = (uint64_t)ln, A = (uint64_t)age, K = (uint64_t)kind, S = (uint64_t)star,
                     D = (uint64_t)did, Hh = (uint64_t)hungry, Dg = (uint64_t)danger;
            uint64_t chk = ((x * 73 + y * 151 + L * 31 + Hh * 5 + A * 17 + K * 91 + S * 3 + D * 7 + tm * 11 + Dg * 13 +
                             0x5A) * 0x9E3779B1ULL) & 0xFFFF;
            return x | (y << 6) | (L << 12) | (Hh << 20) | (A << 21) | (K << 24) | (S << 25) | (D << 26) |
                   (chk << 42) | (tm << 58) | (Dg << 59);
        }
        if (ln > 127) ln = 127;
        ln |= hungry << 7;
        uint64_t L = (uint64_t)ln, A = (uint64_t)age, K = (uint64_t)kind;
        uint64_t chk = ((x * 73 + y * 151 + L * 31 + A * 17 + K * 91 + 0x5A) * 0x9E37ULL) & 0xFF;
        return (x << 26) | (y << 20) | (L << 12) | (A << 9) | (K << 8) | chk;
    }

    // py: _beacon_in - read the beacons among the sonar values, remember the best king and the best enemy king they
    // tell of; return the value to relay (0: none)
    uint64_t beacon_in(const std::vector<std::string>& msgs, int rnd) {
        const double relay_lim = (mg_now && T(P.MG_RELAY)) ? P.MG_RELAY
                                 : (qk && T(P.QK_RELAY)) ? P.QK_RELAY : P.BEACON_RELAY;   // (6 Oct, MG_RELAY; 8 Oct, QK_RELAY)
        bool b64 = T(P.BEACON64);
        long long best_ln[2] = {-1, -1};
        long long best_h[2] = {0, 0};
        long long best_id[2] = {-1, -1};
        long long best_age[2] = {0, 0};
        long long best_g[2] = {-1, -1};
        const bool qcall = T(P.Q_HUNT_CALL) && (!T(P.Q_HUNT_AREA) || (double)W * H >= P.Q_HUNT_AREA);
        bool qhave = false;
        long long qb_g = 0, qb_age = 0, qb_ln = 0;
        for (auto& m : msgs) {
            MsgVal mv = parse_msg(m);
            if (mv.st == 0) continue;
            if (mv.st < 0) continue;
            long long x, y, ln, hungry, age, kind, did;
            if (b64) {
                if (mv.big || mv.v >= (1ULL << 60)) continue;
                uint64_t v = mv.v;
                x = (long long)(v & 63);
                y = (long long)((v >> 6) & 63);
                ln = (long long)((v >> 12) & 255);
                hungry = (long long)((v >> 20) & 1);
                age = (long long)((v >> 21) & 7);
                kind = (long long)((v >> 24) & 1);
                long long star = (long long)((v >> 25) & 1);
                did = (long long)((v >> 26) & 0xFFFF);
                long long tm = (long long)((v >> 58) & 1);
                long long danger = (long long)((v >> 59) & 1);
                if (tm != tbit) continue;
                uint64_t chk = (((uint64_t)x * 73 + (uint64_t)y * 151 + (uint64_t)ln * 31 + (uint64_t)hungry * 5 +
                                 (uint64_t)age * 17 + (uint64_t)kind * 91 + (uint64_t)star * 3 + (uint64_t)did * 7 +
                                 (uint64_t)tm * 11 + (uint64_t)danger * 13 + 0x5A) * 0x9E3779B1ULL) & 0xFFFF;
                if (chk != ((v >> 42) & 0xFFFF)) continue;
                hungry |= (star << 1) | (danger << 2);
            } else {
                if (mv.big || mv.v >= (1ULL << 32)) continue;
                uint64_t v = mv.v;
                x = (long long)(v >> 26);
                y = (long long)((v >> 20) & 63);
                ln = (long long)((v >> 12) & 255);
                age = (long long)((v >> 9) & 7);
                kind = (long long)((v >> 8) & 1);
                uint64_t chk = (((uint64_t)x * 73 + (uint64_t)y * 151 + (uint64_t)ln * 31 + (uint64_t)age * 17 +
                                 (uint64_t)kind * 91 + 0x5A) * 0x9E37ULL) & 0xFF;
                hungry = ln >> 7;
                ln &= 127;
                did = -1;
                if (chk != (v & 0xFF)) continue;
            }
            if (qcall && kind == 1 && (hungry & 2) && b64) {
                // (v3q) a queen call: where somebody saw the enemy queen's head, age rounds ago
                if (T(P.QH_AGE) && age >= 7 && did > age) age = did;   // (8 Oct, QH_AGE) an old sighting keeps its true age
                if (x < W && y < H && (!qhave || age < qb_age)) {
                    qhave = true;
                    qb_g = y * W + x;
                    qb_age = age;
                    qb_ln = ln;
                }
                continue;
            }
            if (kind == 0 && T(P.Q_KING) && (did == 0 || did == 1) && rnd - age > q_last) q_last = (int)(rnd - age);   // (v3q) alive
            if (kind == 0 && qk) {
                if (!(did == 0 || did == 1) || x >= W || y >= H || (double)ln < P.Q_BEACON_LEN) continue;   // (v3q) only the queen
            } else if (x >= W || y >= H || (double)ln < (kind == 0 ? P.BEACON_LEN : P.HUNT_SEG)) continue;
            // the longest wins; among equals the lower id (the same rule everywhere: one king for the team)
            int k = (int)kind;
            if (ln > best_ln[k] || (ln == best_ln[k] && did >= 0 && (best_id[k] < 0 || did < best_id[k]))) {
                best_ln[k] = ln;
                best_h[k] = hungry;
                best_id[k] = did;
                best_age[k] = age;
                best_g[k] = y * W + x;
            }
        }
        uint64_t relay = 0;
        qrelay = 0;
        if (qhave) {
            bool stale_ = false;            // (8 Oct, QH_NOREINF) the spot I found empty, no newer than what I dropped
            if (T(P.QH_NOREINF) && qdrop_g >= 0 && rnd - qb_age <= qdrop_r) {
                int dx = iabs((int)(qb_g % W) - qdrop_g % W), dy = iabs((int)(qb_g / W) - qdrop_g / W);
                if (dx > W - dx) dx = W - dx;
                if (dy > H - dy) dy = H - dy;
                stale_ = dx + dy <= 2;
            }
            if (!stale_ && (qprey_g < 0 || rnd - qb_age > qprey_r)) {
                qprey_g = (int)qb_g;
                qprey_r = (int)(rnd - qb_age);
            }
            if (qb_age < P.BEACON_RELAY) {
                qrelay = beacon_val((int)qb_g, qb_ln, qb_age + 1, 1, 2);
                qrelay_r = rnd;
            }
        }
        if (best_ln[0] >= 0 && (best_h[0] & 4) && best_id[0] != (long long)(id & 0xFFFF))
            danger_heard = rnd;             // (v2.15) a king in the thick of the enemy (DANGER_MEM)
        for (int kind = 0; kind < 2; kind++) {
            if (best_ln[kind] < 0) continue;
            long long ln = best_ln[kind];
            long long age = best_age[kind];
            int g = (int)best_g[kind];
            if (kind == 0) {
                long long did = best_id[kind];
                std::optional<Tri> kp = king_pos;
                if (b64) {
                    if (kp && did == king_id && rnd - age < kp->b && T(P.BEACON_FRESH)) {
                        long long a2 = rnd - kp->b + 1;
                        if (a2 < relay_lim && !relay)
                            relay = beacon_val(kp->a, kp->c, a2, 0, king_hungry > 0 ? king_hungry : 0, king_id);
                        continue;
                    }
                    if (kp && rnd - kp->b <= P.FEED_FRESH && did != king_id &&
                        (kp->c > ln || (kp->c == ln && king_id < did))) {
                        long long a2 = rnd - kp->b + 1;
                        if (a2 < relay_lim && !relay)
                            relay = beacon_val(kp->a, kp->c, a2, 0, king_hungry > 0 ? king_hungry : 0, king_id);
                        continue;
                    }
                    king_id = (int)did;
                }
                if (!king_seen || ln >= king_seen->c || rnd - king_seen->b > 3)
                    king_seen = Tri{g, (int)(rnd - age), (int)ln};
                king_pos = Tri{g, (int)(rnd - age), (int)ln};
                if (ln >= king_len || rnd - king_len_round > 20) {
                    king_len = (int)ln;
                    king_len_round = rnd;
                    king_hungry = (int)best_h[0];
                }
                if (b64) {
                    bool mine = did == (long long)(id & 0xFFFF);
                    bool beats_me = qk || ln > my_len || (ln == my_len && did < (long long)(id & 0xFFFF));
                    if (age < relay_lim && beats_me && !mine && !relay)
                        relay = beacon_val(g, ln, age + 1, 0, best_h[0], did);
                } else if (age < relay_lim && my_len < ln && !relay) {
                    relay = beacon_val(g, ln, age + 1, 0, best_h[0]);
                }
            } else {
                if ((double)ln >= elong || rnd - elong_r > 100) {   // (Q_HUNT_LEAD)
                    elong = (double)ln;
                    elong_r = (int)(rnd - age);
                }
                if (!prey || ln >= prey->c || rnd - prey->b > 3) prey = Tri{g, (int)(rnd - age), (int)ln};
                if ((T(P.ENOUGH_M) || T(P.REG_EMT)) && (double)ln > emax && rnd - age >= P.ENOUGH_FROM) {
                    emax = (double)ln;                  // (v3) a long enemy somebody saw
                    emax_round = (int)(rnd - age);
                }
                if (age < P.BEACON_RELAY && !relay) relay = beacon_val(g, ln, age + 1, 1);
            }
        }
        return relay;
    }

    // (v2.11) py: _gossip_val
    uint64_t gossip_val(int rnd) {
        int n = (int)gossip_recent.size();
        if (!n) return 0;
        int i = gossip_i;
        if (i >= n) i = 0;
        struct E {
            long long x, y, due;
        };
        std::vector<E> ent;
        long long age = 0;
        int tries = 0;
        while (ent.size() < 2 && tries < n) {
            int g = gossip_recent[(size_t)i].first, r0 = gossip_recent[(size_t)i].second;
            i++;
            if (i >= n) i = 0;
            tries++;
            if (!hot.has(g)) continue;
            long long due = (long long)hot.get(g, 0) - rnd;
            if (due < 0) due = 0;
            if (due > 254) continue;
            long long a = rnd - r0;
            if (a > age) age = a;
            ent.push_back({g % W, g / W, due});
        }
        gossip_i = i;
        if (ent.empty()) return 0;
        uint64_t x1 = (uint64_t)ent[0].x, y1 = (uint64_t)ent[0].y, d1 = (uint64_t)ent[0].due;
        uint64_t x2 = 0, y2 = 0, d2 = 255;
        if (ent.size() > 1) {
            x2 = (uint64_t)ent[1].x;
            y2 = (uint64_t)ent[1].y;
            d2 = (uint64_t)ent[1].due;
        }
        if (age > 3) age = 3;
        uint64_t A = (uint64_t)age, tm = (uint64_t)tbit;
        uint64_t chk = ((x1 * 73 + y1 * 151 + d1 * 31 + x2 * 37 + y2 * 97 + d2 * 13 + A * 17 + tm * 11 + 0xC3) *
                        0x9E3779B1ULL) & 0xFFFF;
        return x1 | (y1 << 6) | (d1 << 12) | (x2 << 20) | (y2 << 26) | (d2 << 32) | (A << 40) | (chk << 42) | (tm << 58);
    }

    // (v2.11) py: _gossip_in
    void gossip_in(const std::vector<std::string>& msgs, int rnd) {
        int tb = tbit;
        bool seen_only = T(P.GOSSIP_SEEN);
        for (auto& m : msgs) {
            MsgVal mv = parse_msg(m);
            if (mv.st == 0) continue;
            if (mv.st < 0 || mv.big || mv.v >= (1ULL << 59) || (int)(mv.v >> 58) != tb) continue;
            uint64_t v = mv.v;
            uint64_t x1 = v & 63, y1 = (v >> 6) & 63, d1 = (v >> 12) & 255, x2 = (v >> 20) & 63, y2 = (v >> 26) & 63,
                     d2 = (v >> 32) & 255, age = (v >> 40) & 3;
            uint64_t chk = ((x1 * 73 + y1 * 151 + d1 * 31 + x2 * 37 + y2 * 97 + d2 * 13 + age * 17 + (uint64_t)tb * 11 +
                             0xC3) * 0x9E3779B1ULL) & 0xFFFF;
            if (chk != ((v >> 42) & 0xFFFF) || d1 == 255) continue;
            int told = rnd - (int)age - 1;
            uint64_t xs[2] = {x1, x2}, ys[2] = {y1, y2}, ds[2] = {d1, d2};
            for (int e = 0; e < 2; e++) {
                uint64_t x = xs[e], y = ys[e], due = ds[e];
                if (due == 255 || x >= (uint64_t)W || y >= (uint64_t)H) continue;
                int g = (int)y * W + (int)x;
                if (gi_get(g) >= 0 || seen[(size_t)g] >= told || seen[(size_t)g] == BARREN_SEEN ||
                    gossip_told[(size_t)g] >= told)
                    continue;
                if (seen_only && seen[(size_t)g] == UNSEEN) continue;
                hot.set(g, due ? rnd + (int)due - 1 : rnd);
                gossip_told[(size_t)g] = told;
            }
        }
    }

    // (7 Oct, TW) a threat warning: tag 6 in bits 60-63, the team bit at 59, x, y, length, age, hops and a checksum
    uint64_t tw_val(int g, long long ln, long long age, long long hops) const {
        uint64_t x = (uint64_t)(g % W), y = (uint64_t)(g / W);
        if (ln > 255) ln = 255;
        if (age > 7) age = 7;
        if (hops > 7) hops = 7;
        uint64_t L = (uint64_t)ln, A = (uint64_t)age, Hp = (uint64_t)hops, tb = (uint64_t)tbit;
        uint64_t chk = ((x * 73 + y * 151 + L * 31 + A * 17 + Hp * 29 + tb * 11 + 0x6B) * 0x9E3779B1ULL) & 0xFFFF;
        return x | (y << 6) | (L << 12) | (A << 20) | (Hp << 23) | (chk << 26) | (tb << 59) | (6ULL << 60);
    }

    // (7 Oct, TW) read the warnings among the sonar values: remember them, keep the freshest to pass on
    void tw_in(const std::vector<std::string>& msgs, int rnd) {
        tw_relay = 0;
        long long best_age = 99;
        for (auto& m : msgs) {
            MsgVal mv = parse_msg(m);
            if (mv.st != 1 || mv.big) continue;
            uint64_t v = mv.v;
            if ((v >> 60) != 6ULL || (int)((v >> 59) & 1ULL) != tbit) continue;
            uint64_t x = v & 63, y = (v >> 6) & 63, L = (v >> 12) & 255, A = (v >> 20) & 7, Hp = (v >> 23) & 7;
            uint64_t chk = ((x * 73 + y * 151 + L * 31 + A * 17 + Hp * 29 + (uint64_t)tbit * 11 + 0x6B) * 0x9E3779B1ULL) & 0xFFFF;
            if (chk != ((v >> 26) & 0xFFFF) || x >= (uint64_t)W || y >= (uint64_t)H) continue;
            const int g = (int)(y * (uint64_t)W + x);
            const int r0 = rnd - (int)A - 1;          // sent at the latest last round
            bool dup = false;
            for (auto& e : tw_mem)
                if (e[0] == g) {
                    dup = true;
                    if (r0 > e[1]) e = {g, r0, (int)L};
                }
            if (!dup) tw_mem.push_back({g, r0, (int)L});
            if ((double)Hp < P.TW_HOPS && (long long)A < best_age) {
                best_age = (long long)A;
                tw_relay = tw_val(g, (long long)L, (long long)A + 1, (long long)Hp + 1);
            }
        }
        if (tw_mem.size() > 24) tw_mem.erase(tw_mem.begin(), tw_mem.begin() + (long)(tw_mem.size() - 24));
    }

    // (7 Oct, QDEAD) "their queen is dead": tag 7, the team bit, a fixed pattern, the relay count and a checksum
    uint64_t qd_val(long long hops) const {
        if (hops > 31) hops = 31;
        uint64_t Hp = (uint64_t)hops, tb = (uint64_t)tbit;
        uint64_t chk = ((Hp * 29 + tb * 11 + 0x3C5) * 0x9E3779B1ULL) & 0xFFFFF;
        return Hp | (chk << 5) | (0x15ULL << 30) | (tb << 59) | (7ULL << 60);
    }

    void qd_in(const std::vector<std::string>& msgs, int rnd) {
        for (auto& m : msgs) {
            MsgVal mv = parse_msg(m);
            if (mv.st != 1 || mv.big) continue;
            const uint64_t v = mv.v;
            if ((v >> 60) != 7ULL || (int)((v >> 59) & 1ULL) != tbit) continue;
            const long long hp = (long long)(v & 31ULL);
            if (v != qd_val(hp)) continue;
            if (!tq_dead) {
                tq_dead = true;
                tq_learned = rnd;
                tq_hops = (int)hp + 1;
                if (T(P.DEBUG)) dbg_note += "tqd-heard ";
            } else if ((int)hp + 1 < tq_hops) {
                tq_hops = (int)hp + 1;
            }
        }
    }

    uint64_t oqd_val(long long hops) const {   // (8 Oct, ARMY) "our queen is dead": tag 7, another fixed pattern
        if (hops > 31) hops = 31;
        uint64_t Hp = (uint64_t)hops, tb = (uint64_t)tbit;
        uint64_t chk = ((Hp * 37 + tb * 13 + 0x5D1) * 0x9E3779B1ULL) & 0xFFFFF;
        return Hp | (chk << 5) | (0x16ULL << 30) | (tb << 59) | (7ULL << 60);
    }

    void oqd_in(const std::vector<std::string>& msgs, int rnd) {
        for (auto& m : msgs) {
            MsgVal mv = parse_msg(m);
            if (mv.st != 1 || mv.big) continue;
            const uint64_t v = mv.v;
            if ((v >> 60) != 7ULL || (int)((v >> 59) & 1ULL) != tbit) continue;
            const long long hp = (long long)(v & 31ULL);
            if (v != oqd_val(hp)) continue;
            if (!oq_dead) {
                oq_dead = true;
                oq_learned = rnd;
                oq_hops = (int)hp + 1;
                if (T(P.DEBUG)) dbg_note += "oqd-heard ";
            } else if ((int)hp + 1 < oq_hops) {
                oq_hops = (int)hp + 1;
            }
        }
    }

    // (v3, REG_R) py: _reg_update - reachable contact in my view, my own rate, the estimates heard, the switch, what I send
    void reg_update(int rnd, const std::vector<std::string>& msgs, const std::vector<Head>& heads, const int* mv) {
        int R = (int)P.REG_R;
        std::vector<double> heard;
        for (auto& m : msgs) {
            MsgVal mv_ = parse_msg(m);
            if (mv_.st != 1 || mv_.big) continue;
            uint64_t v = mv_.v;
            if ((v >> 60) != 5ULL || (int)((v >> 59) & 1ULL) != tbit) continue;
            long long q = (long long)((v >> 47) & 4095ULL);
            if ((long long)((v >> 31) & 0xFFFFULL) != ((q * 40503LL + (long long)tbit * 7919LL + 0x2D1FLL) & 0xFFFFLL)) continue;
            heard.push_back((double)q / 8192.0);
        }
        bool hit = false;
        bool es[49];
        bool anye = false;
        for (int i = 0; i < 49; i++) es[i] = false;
        for (auto& h : heads)
            if (h.en) {
                es[h.i] = true;
                anye = true;
            }
        if (anye) {
            std::vector<int> starts;
            starts.push_back(24);
            for (auto& h : heads)
                if (!h.en) starts.push_back(h.i);
            for (int st : starts) {
                int dd[49];
                for (int i = 0; i < 49; i++) dd[i] = -1;
                int q[49];
                int qn = 0;
                dd[st] = 0;
                q[qn++] = st;
                for (int qi = 0; qi < qn; qi++) {
                    int u = q[qi];
                    if (es[u]) {
                        hit = true;
                        break;
                    }
                    int du = dd[u];
                    if (du >= R) continue;
                    for (int k = u * 4; k < u * 4 + 4; k++) {
                        int x = mv[k];
                        if (x >= 0 && dd[x] < 0) {
                            dd[x] = du + 1;
                            q[qn++] = x;
                        }
                    }
                }
                if (hit) break;
            }
        }
        if (reg_born < 0) reg_born = rnd;
        if (T(P.REG_OPEN)) {
            long long kc = 0;
            for (int k = 0; k < 196; k++)
                if (mv[k] == -1) kc++;
            reg_kelp += kc;
            reg_edges += 196;
        }
        if (hit) reg_hist.push_back(rnd);
        double win = P.REG_WIN;
        while (!reg_hist.empty() && (double)reg_hist[0] <= rnd - win) reg_hist.erase(reg_hist.begin());
        double span = rnd - reg_born + 1;
        if (span > win) span = win;
        double est = reg_est;
        double hs = 0.0;
        for (double h : heard) hs += h;
        double prior = est >= 0.0 ? est : (heard.empty() ? 0.0 : hs / (double)heard.size());
        double own = ((double)reg_hist.size() + (win - span) * prior) / win;
        double mean = (own + hs) / (double)(1 + heard.size());
        if (est < 0.0) est = mean;
        else est += P.REG_A * (mean - est);
        reg_est = est;
        if (reg_on) {
            if (est < P.REG_T / 2) reg_on = false;
        } else if (est >= P.REG_T) reg_on = true;
        if (T(P.REG_SEND)) {
            long long qv = (long long)(est * 8192.0 + 0.5);
            if (qv > 4095) qv = 4095;
            long long chk = (qv * 40503LL + (long long)tbit * 7919LL + 0x2D1FLL) & 0xFFFFLL;
            reg_out = (5ULL << 60) | ((uint64_t)tbit << 59) | ((uint64_t)qv << 47) | ((uint64_t)chk << 31);
        }
    }
