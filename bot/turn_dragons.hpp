// Sabotage-d / turn_dragons.hpp, part 2 of the turn: the dragons in view (occ, heads), whether a queen just died
// where I looked (QDEAD, ARMY), the late clock, the longest friend seen and whether I am the king, and my own body
// (the trail, freeat / body_at).
// Statements of Brain::turn_(), in the order they run: included once from turn.hpp, inside the function. This part
// reads what the parts before it declared and declares what the parts after it use.

        // ---- dragons in view
        int occ[49];            // 0 free, 1 body, 2 head of another dragon, 3 me
        for (int i = 0; i < 49; i++) occ[i] = 0;
        SmallMap seg_n;         // id -> visible segments
        SmallMap seg_ring;      // id -> touches the window border (so it may be longer)
        std::vector<Head> heads;
        int own_face[49];
        for (int i = 0; i < 49; i++) own_face[i] = -1;
        SmallMap head_face;     // id -> facing of another dragon's head
        others.clear();
        bool has_ptiles = !pt_pool.empty();
        std::vector<std::pair<int, int>> threading;
        bool use_ebody = T(Pp.ASSN) && T(Pp.ASSN_BODY);
        std::vector<std::pair<int, std::vector<std::pair<int, int>>>> ebody;   // id -> [(index, facing)], in order
        bool use_qtb = T(Pp.Q_TAIL) && is_queen;   // (v3q, Q_TAIL) enemy body segments by id: (index, facing)
        bool oq_any = false;                      // (8 Oct, ARMY) any part of OUR queen in view
        int oq_head_g = -1;
        std::vector<int> oq_tiles;
        bool eq_any = false;                      // (7 Oct, QDEAD) any part of their queen in view
        int eq_head_g = -1;                       //   her head's tile
        std::vector<int> eq_tiles;                //   all her tiles in view
        std::vector<std::pair<int, std::vector<std::pair<int, int>>>> qtb;
        for (long long k = 0; k < nbod; k++) {
            auto q = split_ws(at(L, bl0 + k));
            if (q.size() < 6) throw PyError("body line");
            int did = (int)pyint(q[1]);
            long long c = fmod(pyint(q[2]) - hx + 3, W);
            long long r = fmod(pyint(q[3]) - hy + 3, H);
            long long ii = r * 7 + c;
            if (ii < 0 || ii >= 49) throw PyError("body outside the window");
            int i = (int)ii;
            if (did == id) {
                occ[i] = 3;
                own_face[i] = didx_s(q[4]);
                continue;
            }
            int gt = wg[i];
            others.add(gt);
            if (q[0] != team) last_enemy = rnd;
            if (q[0] == team && (did == 0 || did == 1)) {   // (8 Oct, ARMY) our queen in view
                oq_any = true;
                oq_tiles.push_back(gt);
                if (q[5] == "1") oq_head_g = gt;
            }
            if (q[0] != team && (did == 0 || did == 1)) {
                eq_any = true;
                eq_tiles.push_back(gt);
                if (q[5] == "1") eq_head_g = gt;
            }
            int fdir = didx_s(q[4]);
            if (has_ptiles) {
                const PTile* pe = ptile(gt);
                if (pe != nullptr) {
                    if (pe->has[fdir] && q[5] == "0") threading.push_back({pe->pid[fdir], did});
                }
            }
            seg_n.set(did, seg_n.get(did, 0) + 1);
            if (RING[i]) seg_ring.set(did, 1);
            if (q[5] == "1") {
                occ[i] = 2;
                heads.push_back({i, did, q[0] != team});
                head_face.set(did, fdir);
            } else {
                occ[i] = 1;
                if (use_ebody && q[0] != team) {
                    bool found = false;
                    for (auto& e : ebody)
                        if (e.first == did) {
                            e.second.push_back({i, fdir});
                            found = true;
                            break;
                        }
                    if (!found) ebody.push_back({did, {{i, fdir}}});
                }
                if (use_qtb && q[0] != team) {
                    bool found = false;
                    for (auto& e : qtb)
                        if (e.first == did) {
                            e.second.push_back({i, fdir});
                            found = true;
                            break;
                        }
                    if (!found) qtb.push_back({did, {{i, fdir}}});
                }
            }
        }

        if (T(Pp.ARMY) && !is_queen) {
            // (8 Oct, ARMY) the same test for OUR queen: seen last round, now nothing of her in view and a new pearl on her tiles
            if (!oq_dead && !oq_any && oq_seen_r == rnd - 1 && oq_seen_g >= 0 && prevw_r == rnd - 1) {
                // stricter than QDEAD (a false alarm would send the team after their queen): every tile I saw her on last
                // round is still in view, and new pearls (not spawns due now) lie on at least min(2, half of them)
                bool all_in = true;
                for (int c : oq_seen_tiles)
                    if (gi_get(c) < 0) all_in = false;
                int hits = 0;
                for (int c : oq_seen_tiles) {
                    const int si = gi_get(c);
                    if (si < 0 || !wp[si]) continue;
                    int pj = -1;
                    for (int j = 0; j < 49; j++)
                        if (prevw_g[j] == c) { pj = j; break; }
                    if (pj < 0 || prevw_p[pj] || (prevw_cd[pj] >= 0 && prevw_cd[pj] <= 2)) continue;
                    hits++;
                }
                const int nt = (int)oq_seen_tiles.size();
                const int need_hits = std::min(2, (nt + 1) / 2);
                const bool hit = all_in && nt > 0 && hits >= need_hits;
                if (hit) {
                    oq_dead = true;
                    oq_learned = rnd;
                    oq_hops = 0;
                    if (T(Pp.DEBUG)) dbg_note += "oqd-seen ";
                }
            }
            if (oq_any) {
                oq_seen_g = oq_head_g >= 0 ? oq_head_g : oq_tiles[0];
                oq_seen_r = rnd;
                oq_seen_tiles = oq_tiles;
            }
            if (!T(Pp.QDEAD)) {                   // QDEAD keeps the previous window itself
                for (int j = 0; j < 49; j++) {
                    prevw_g[j] = wg[j];
                    prevw_p[j] = wp[j];
                    prevw_cd[j] = wcd[j];
                }
                prevw_r = rnd;
            }
        }
        if (T(Pp.QDEAD)) {
            // (7 Oct, QDEAD) I saw her last round; now nothing of her is in view and a NEW pearl lies on one of the tiles she
            // was on or next to her head (her drops: head first, every second segment - she may have moved first)
            if (!tq_dead && !eq_any && tq_seen_r == rnd - 1 && tq_seen_g >= 0 && prevw_r == rnd - 1) {
                std::vector<int> cand = tq_seen_tiles;
                for (int v : nb(tq_seen_g))
                    if (v >= 0) cand.push_back(v);
                bool hit = false;
                for (int c : cand) {
                    const int si = gi_get(c);
                    if (si < 0 || !wp[si]) continue;
                    int pj = -1;
                    for (int j = 0; j < 49; j++)
                        if (prevw_g[j] == c) { pj = j; break; }
                    if (pj < 0 || prevw_p[pj] || (prevw_cd[pj] >= 0 && prevw_cd[pj] <= 2)) continue;   // not a spawn due now
                    hit = true;
                    break;
                }
                if (hit) {
                    tq_dead = true;
                    tq_learned = rnd;
                    tq_hops = 0;
                    if (T(Pp.DEBUG)) dbg_note += "tqd-seen ";
                }
            }
            if (eq_any) {
                tq_seen_g = eq_head_g >= 0 ? eq_head_g : eq_tiles[0];
                tq_seen_r = rnd;
                tq_seen_tiles = eq_tiles;
            }
            for (int j = 0; j < 49; j++) {
                prevw_g[j] = wg[j];
                prevw_p[j] = wp[j];
                prevw_cd[j] = wcd[j];
            }
            prevw_r = rnd;
        }
        if (T(Pp.REG_R)) reg_update(rnd, msgs, heads, mv);
        if (T(Pp.ECON_DANGER)) {
            // (v2.15) how much of the time an enemy head is in my view (a running average)
            double ce = 0.0;
            for (auto& h : heads)
                if (h.en) {
                    ce = 1.0;
                    break;
                }
            contact_avg += (ce - contact_avg) / Pp.DANGER_WIN;
        }

        for (auto& tp : threading) {
            // the exit stays blocked until the whole of that dragon is through
            int n = 2;
            if (T(Pp.PORTAL_BUSY)) {
                n = seg_n.get(tp.second, 0) + 1;
                if (n < 3) n = 3;
            }
            auto it = pbusy.find(tp.first);
            int cur = it == pbusy.end() ? -1 : it->second;
            if (cur < rnd + n) pbusy[tp.first] = rnd + n;
        }

        // nobody around for a long time: this game is going the distance
        bool quiet_ = T(Pp.QUIET_GAP) && Pp.QUIET_GAP > 0 && rnd >= Pp.QUIET_ROUND &&
                      rnd - last_enemy >= Pp.QUIET_GAP && ucount >= Pp.QUIET_UNITS;
        quiet = quiet_;
        double late_round = Pp.LATE_ROUND;
        double guard_round = Pp.GUARD_ROUND;
        double feed_round = Pp.FEED_ROUND;
        double beacon_round_ = Pp.BEACON_ROUND;
        double ksplit_round_ = Pp.KING_SPLIT_ROUND;
        enough = false;
        reg_late = false;
        if (T(Pp.REG_R) && reg_on && rnd >= Pp.REG_FROM && (!T(Pp.REG_EMT) || emax < Pp.REG_EMT) &&
            !(T(Pp.REG_UNITS) && (reg_units_on || (reg_units_on = (ucount >= Pp.REG_UNITS && rnd >= Pp.REG_FROM &&
                                                                  (double)W * H >= Pp.BIGCLK_AREA)))) &&   // (v3.9, REG_UNITS; sticky)
            (!T(Pp.REG_OPEN) || (double)reg_kelp >= Pp.REG_OPEN * (double)reg_edges)) {
            // (v3) the swarms mix here: the late clock (no early economy)
            late_round = Pp.REG_LATE;
            guard_round = Pp.REG_GUARD;
            feed_round = Pp.REG_FEED;
            beacon_round_ = Pp.REG_BEACON;
            ksplit_round_ = Pp.REG_KSPLIT;
            if (T(Pp.BIGCLK) && (double)W * H >= Pp.BIGCLK_AREA) {
                // (v3.9, BIGCLK) big maps go the distance: the late clock's king starts BIGCLK rounds sooner there
                guard_round -= Pp.BIGCLK;
                feed_round -= Pp.BIGCLK;
                beacon_round_ -= Pp.BIGCLK;
                ksplit_round_ -= Pp.BIGCLK;
            }
            reg_late = true;
            if (T(Pp.ENOUGH_M)) {
                // (v3) convert only as far as the enemy makes it necessary
                double kl = rnd - king_len_round <= 20 ? (double)king_len : 0.0;
                double em = emax;
                if (em < Pp.ENOUGH_FLOOR) em = Pp.ENOUGH_FLOOR;
                double er = (double)emax_round;
                if (er < Pp.REG_FROM) er = Pp.REG_FROM;
                em += Pp.ENOUGH_GROW * ((double)rnd - er);
                enough = (!lead) && kl >= em + Pp.ENOUGH_M;
                if (enough) {
                    guard_round = 999;
                    feed_round = 999;
                }
            }
        } else if (quiet_) {
            if (late_round > rnd) late_round = rnd;
            if (guard_round > rnd) guard_round = rnd;
            feed_round -= Pp.QUIET_FEED_EARLY;
        } else if (T(Pp.EARLY_ROUND) && rnd >= early_round_() &&
                   (ucount >= early_units_() || (early_on && T(Pp.EARLY_STICKY))) &&
                   (!T(Pp.EARLY_QUIET) || rnd - last_enemy >= Pp.EARLY_QUIET || (early_on && T(Pp.EARLY_STICKY)))) {
            if (!early_on) early_e = early_round_();
            early_on = true;
            double e = T(Pp.SEP_ROUND) ? early_e : Pp.EARLY_ROUND;
            if (guard_round > e) guard_round = e;
            if (beacon_round_ > e) beacon_round_ = e;
            if (ksplit_round_ > e) ksplit_round_ = e;
            if (feed_round > e + Pp.EARLY_FEED) feed_round = e + Pp.EARLY_FEED;
            if (Pp.EARLY_LATE >= 0 && late_round > e + Pp.EARLY_LATE) late_round = e + Pp.EARLY_LATE;
        }
        if (T(Pp.CLOCK_SHIFT) && !(T(Pp.CLOCK_AREA) && (double)W * H >= Pp.CLOCK_AREA)) {   // (5 Oct, CLOCK_SHIFT; v3q91)
            const double cs = Pp.CLOCK_SHIFT;
            late_round += cs;
            guard_round += cs;
            feed_round += cs;
            beacon_round_ += cs;
            ksplit_round_ += cs;
        }
        if (qk && T(Pp.QK_EARLY)) {   // (8 Oct, QKE) our queen is the king: her court and her feeding keep her own clock
            if (guard_round > Pp.Q_KING_FROM) guard_round = Pp.Q_KING_FROM;
            if (T(Pp.QK_FEED) && feed_round > Pp.QK_FEED) feed_round = Pp.QK_FEED;
        }
        beacon_round = beacon_round_;
        ksplit_round = ksplit_round_;
        guard_round_eff = guard_round;

        uint64_t relay = 0;
        if (!msgs.empty() && T(Pp.BEACON)) relay = beacon_in(msgs, rnd);
        if (T(Pp.TW)) {
            tw_relay = 0;
            if (!msgs.empty()) tw_in(msgs, rnd);   // (7 Oct, TW)
        }
        // (v2.9) the team's king by its beacon, known by id while the beacon is fresh
        int kid_ = -1;
        if (T(Pp.BEACON64) && king_pos && rnd - king_pos->b <= ((mg_now && T(Pp.MG_FRESH)) ? Pp.MG_FRESH
                                                                 : (qk && T(Pp.QK_FRESH)) ? Pp.QK_FRESH : Pp.KING_FRESH))
            kid_ = king_id;                 // (6 Oct, MG_FRESH: longer in the gathering)
        int me16 = id & 0xFFFF;
        // longest friendly dragon seen lately (visible segments: a lower bound)
        int fm = friend_max;
        if (rnd - friend_max_round > 40) fm = 0;
        int kb = (int)Pp.GUARD_KING - 1;
        king_i = -1;
        const std::optional<Tri> kp_before = king_pos;     // (py: kp, read once before the loop)
        const bool qking = qk;
        for (auto& h : heads) {
            if (!h.en) {
                int n = seg_n.get(h.did, 0);
                if ((h.did == 0 || h.did == 1) && T(Pp.Q_KING) &&
                    !(T(Pp.Q_KING_AREA) && sealed_area(wg[h.i], (int)Pp.QVAULT_MAX) > 0))
                    q_last = rnd;    // (v3q) the queen is alive (v3.9: and not sealed in - a capped queen is no king)
                if (n >= fm) {
                    fm = n;
                    friend_max_round = rnd;
                }
                if (qking) {
                    // (v3q) the king is our queen: in view, that is where it is (and, all of it showing, how long)
                    if (h.did == 0 || h.did == 1) {
                        int kl = n;
                        if (seg_ring.has(h.did) && kp_before && rnd - kp_before->b <= Pp.KING_FRESH && kp_before->c > n)
                            kl = (int)kp_before->c;
                        king_i = h.i;
                        king_id = h.did;
                        kid_ = h.did;
                        king_seen = Tri{wg[h.i], rnd, kl};
                        king_pos = Tri{wg[h.i], rnd, kl};
                    }
                    continue;
                }
                if (kid_ >= 0) {
                    if ((h.did & 0xFFFF) == kid_) {
                        // the king itself is in view: that is where it is, whatever the beacon said
                        king_i = h.i;
                        king_seen = Tri{wg[h.i], rnd, n};
                        Tri kp0 = *kp_before;
                        if (T(Pp.KING_SEEN) && !seg_ring.has(h.did)) king_pos = Tri{wg[h.i], kp0.b, n};
                        else king_pos = Tri{wg[h.i], kp0.b, kp0.c > n ? kp0.c : n};
                    }
                } else if (n > kb) {
                    kb = n;
                    king_seen = Tri{wg[h.i], rnd, n};   // where I last saw a friend worth guarding
                }
            }
        }
        friend_max = fm;
        // (v2.9) am I the king?  Nothing I know of beats me
        std::optional<Tri> kp = king_pos;
        bool lead_ = qking ? is_queen     // (v3q) the queen is the king, whatever its length
                           : (length >= fm && (kid_ < 0 || kid_ == me16 || kp->c < length || (kp->c == length && me16 < kid_)));
        lead = lead_;
        kid = kid_;
        bool king_far_ = false;
        if (kid_ >= 0 && T(Pp.FEED_CHAIN)) {
            int dx = iabs(hx - (int)fmod(kp->a, W));
            int dy = iabs(hy - (int)fdiv(kp->a, W));
            if (dx > W - dx) dx = W - dx;
            if (dy > H - dy) dy = H - dy;
            king_far_ = dx + dy > Pp.FEED_CHAIN_DIST;
        }
        king_far = king_far_;

        // ---- my own body. The trail is exact because I know every step I made.
        if (trail.empty()) {
            // new process (a split child, or a restart): follow the facings I can see
            trail.push_back(hg);
            int cur = 24;
            bool used[49] = {false};
            used[24] = true;
            while (true) {
                int nxt = -1;
                for (int d = 0; d < 4; d++) {
                    int n = mv[cur * 4 + d];
                    if (n >= 0 && !used[n] && own_face[n] >= 0 && mv[n * 4 + own_face[n]] == cur) {
                        nxt = n;
                        break;
                    }
                }
                if (nxt < 0) break;
                used[nxt] = true;
                trail.push_back(wg[nxt]);
                cur = nxt;
            }
        } else if (trail[0] != hg) {
            if (has_plan && !plan.empty() && plan.back() == hg) {
                for (int g : plan) trail.insert(trail.begin(), g);
            } else {
                trail.insert(trail.begin(), hg);
            }
        }
        if (T(Pp.TRAIL_EXTEND) && (int)trail.size() < length) {
            // (v2.13) the far end of what I know of my body is in view: follow the facings on from there
            int cur = gi_get(trail.back());
            if (cur >= 0) {
                bool used[49] = {false};
                for (int g : trail) {
                    int i = gi_get(g);
                    if (i >= 0) used[i] = true;
                }
                while ((int)trail.size() < length) {
                    int nxt = -1;
                    for (int d = 0; d < 4; d++) {
                        int n = mv[cur * 4 + d];
                        if (n >= 0 && !used[n] && own_face[n] >= 0 && mv[n * 4 + own_face[n]] == cur) {
                            nxt = n;
                            break;
                        }
                    }
                    if (nxt < 0) break;
                    used[nxt] = true;
                    trail.push_back(wg[nxt]);
                    cur = nxt;
                }
            }
        }
        if ((int)trail.size() > length) trail.resize((size_t)(length > 0 ? length : 0));
        has_plan = false;
        plan.clear();

        // freeat / body_at: the step number from which a tile may be entered (1 = the first step of this turn).
        // Segment k of L frees up at step L-k+1; the tail is NOT free on step 1.
        int freeat[49];
        for (int i = 0; i < 49; i++) freeat[i] = occ[i] ? NEVER : 0;
        body_at.clear();
        int tn = (int)trail.size();
        for (int k = 0; k < tn; k++) {
            int g = trail[(size_t)k];
            int t = length - k + 1;
            body_at.set(g, t);
            int i = gi_get(g);
            if (i >= 0) freeat[i] = t;
        }
        if (T(Pp.BODY_UNK) && tn < length) {
            // (v2.11) segments of mine in view that are not on the trail I know
            int tu = length - tn + 1;
            for (int i = 0; i < 49; i++) {
                if (occ[i] == 3) {
                    int g = wg[i];
                    if (!body_at.has(g)) body_at.set(g, tu);
                }
            }
        }
