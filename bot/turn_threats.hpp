// Sabotage-d / turn_threats.hpp, part 3 of the turn: enemy reach - the heads in view sorted into enemies and
// friends, the tiles an enemy head can ram before my next turn (thr, treach, tpen), a tail that may split into a
// head (Q_TAIL), warnings seen and heard (TW), what the queen remembers (Q_MEM), the assassins (ASSN, off) and the
// enemy head nearest our queen (QG).
// Statements of Brain::turn_(), in the order they run: included once from turn.hpp, inside the function. This part
// reads what the parts before it declared and declares what the parts after it use.

        // ---- enemy reach: tiles an enemy head can ram before my next turn
        int reach_cap = (int)Pp.REACH_CAP;
        bool grade = T(Pp.THREAT_GRADE);
        int thr[49];
        double tpen[49];
        int treach[49];
        for (int i = 0; i < 49; i++) {
            thr[i] = 0;
            tpen[i] = 0.0;
            treach[i] = 0;
        }
        double keep_even = Pp.KEEP_EVEN;
        double teven = Pp.THREAT_EVEN;
        double teven_slack = Pp.THREAT_EVEN_SLACK;
        double teven_round = Pp.THREAT_EVEN_ROUND;
        double tpearl = Pp.THREAT_PEARL;
        int n_en = 0;
        for (auto& h : heads)
            if (h.en) n_en++;
        int n_fr = (int)heads.size() - n_en;
        vis_n = seg_n;
        double qsafe_ = (qs && Pp.Q_SAFE > 0) ? Pp.Q_SAFE : 0.0;
        if (!(qsafe_ > 0) && !is_queen && T(Pp.KSAFE) && lead_ && length >= Pp.KSAFE_LEN)
            qsafe_ = Pp.KSAFE;                     // (v3.9, KSAFE) our king guards its life like the queen
        if (!(qsafe_ != 0.0) && is_queen && !qs && Pp.Q_SAFE_SMALL > 0)
            qsafe_ = Pp.Q_SAFE_SMALL;            // (v3q) a smaller stake where Q_AREA leaves the queen's safety off
        if (qsafe_ > 0 && is_queen && tq_dead && T(Pp.QDEAD_FORT)) qsafe_ *= Pp.QDEAD_FORT;   // (7 Oct, QDEAD) the fortress
        const double qsafe = qsafe_;
        if (qsafe > 0 && Pp.Q_REACH_CAP > reach_cap) reach_cap = (int)Pp.Q_REACH_CAP;   // (v3q) the queen counts long sprints in full
        const bool q_end_now = is_queen && T(Pp.Q_END_STILL) && (double)rnd >= 500.0 - Pp.Q_END_STILL;   // (8 Oct)
        const bool chk24 = qsafe > 0 && (T(Pp.Q_SPLIT_SAFE) || q_end_now) && grade;
        const double lsafe = (!(qsafe > 0) && Pp.LS_SAFE > 0 && length >= Pp.LS_LEN) ? Pp.LS_SAFE : 0.0;   // (2 Oct, LS_SAFE)
        bool qhthr = false;                      // (v3q, Q_SPLIT_SAFE) an enemy in view can ram my head where it is
        double qpass = T(Pp.THREAT_PASS) ? 1 : 0;
        if (qsafe > 0 && T(Pp.Q_PASS)) qpass = Pp.Q_ESC > 1 ? Pp.Q_ESC : 1;
        const bool snew = T(Pp.SPRINT_NEW);
        const double sdiv = Pp.SPRINT_DIV;
        std::vector<Enemy> enemies;
        std::vector<int> friends;
        bool last_round = T(Pp.LAST_ROUND) && rnd >= Pp.LAST_ROUND;
        for (auto& h : heads) {
            int ei = h.i, did = h.did;
            if (!h.en) {
                friends.push_back(ei);
                if (T(Pp.MT_W) && !(T(Pp.MT_AREA) && (double)W * H >= Pp.MT_AREA)) {
                    // (MT) the area this mate can see is the team's: mark it
                    const int fg = wg[ei], fx = fg % W, fy = fg / W, R = (int)Pp.MT_R;
                    for (int oy = -R; oy <= R; oy++) {
                        const int row = (((fy + oy) % H) + H) % H * W;
                        for (int ox = -R; ox <= R; ox++) mate_rnd[(size_t)(row + (((fx + ox) % W) + W) % W)] = rnd;
                    }
                }
                continue;
            }
            int ln = seg_n.get(did, 0);
            bool ring = seg_ring.has(did);
            double est = ln + (ring ? 2 : 0);          // for the threat it poses
            double rl = ln;                            // for whether it is worth a ram: what I can vouch for
            if (T(Pp.ELEN_MEMORY) && rnd >= Pp.LATE_KILL_ROUND) {
                auto it = elen.find(did);
                if (!ring) elen[did] = {ln, rnd};
                else if (it == elen.end() || ln >= it->second.first) elen[did] = {ln, rnd};
                else if (rnd - it->second.second <= Pp.ELEN_MEMORY && it->second.first > rl) rl = it->second.first;
            }
            if (ring && rnd >= Pp.LATE_KILL_ROUND && T(Pp.LATE_RING)) {
                double late = (double)(long long)(ln * (1.0 + Pp.LATE_RING));
                if (late > rl) rl = late;
            }
            if (rl > est) est = rl;
            if (qsafe > 0 && T(Pp.Q_UNSEEN) && ring && est < Pp.Q_UNSEEN)
                est = Pp.Q_UNSEEN;                  // (v3.9, Q_UNSEEN) the queen: an enemy that runs out of my view may be this long
            if (T(Pp.THREAT_EDGE) && length >= Pp.THREAT_EDGE_LEN && ring && est < Pp.THREAT_EDGE &&
                rnd >= Pp.THREAT_EDGE_ROUND && (lead || !T(Pp.THREAT_EDGE_LEAD)))
                est = Pp.THREAT_EDGE;               // (v2.14) a long dragon assumes the worst
            enemies.push_back({ei, did, (int)rl, (int)est});
            if (est >= elong || rnd - elong_r > 100) {      // (Q_HUNT_LEAD)
                elong = est;
                elong_r = rnd;
            }
            if ((T(Pp.ENOUGH_M) || T(Pp.REG_EMT)) && est > emax && rnd >= Pp.ENOUGH_FROM) {
                emax = est;                          // (v3, ENOUGH_M) the longest enemy I have seen myself
                emax_round = rnd;
            }
            if (last_round && did < id) continue;   // (v2.10) it has moved for the last time
            double reach = snew ? (double)fdiv((long long)est + (long long)sdiv - 1, (long long)sdiv) + est - 2.0   // (v3q) free steps, then a segment a step
                                : est - 1;
            if (reach < 1) reach = 1;
            else if (reach > reach_cap) reach = reach_cap;
            int dist[49];
            for (int i = 0; i < 49; i++) dist[i] = -1;
            dist[ei] = 0;
            std::vector<int> q;
            q.reserve(49);
            q.push_back(ei);
            if (!grade) {
                for (size_t qi = 0; qi < q.size(); qi++) {
                    int u = q[qi];
                    int nd = dist[u] + 1;
                    for (int k = u * 4; k < u * 4 + 4; k++) {
                        int v = mv[k];
                        if (v >= 0 && dist[v] < 0 && (!occ[v] || (qpass > 0 && occ[v] == 3 && freeat[v] <= qpass))) {
                            dist[v] = nd;
                            if (thr[v] == 0 || est < thr[v]) thr[v] = (int)est;
                            if (nd < reach) q.push_back(v);
                        }
                    }
                }
                continue;
            }
            double stake = length - est;
            const bool qtrade = !(qsafe > 0) && is_queen && T(Pp.Q_TRADE);
            if (qsafe > 0) stake = qsafe;          // (v3q) any trade for our queen is a good one for them
            else if (qtrade) stake = stake > Pp.Q_TRADE ? stake : Pp.Q_TRADE;   // (6 Oct, Q_TRADE)
            else if (lsafe > 0 && stake > 0 && stake < lsafe) stake = lsafe;   // (2 Oct, LS_SAFE) a long dragon
            double pstake = 0.0;
            double preach = 0;
            if (T(tpearl) && !(qsafe > 0)) {
                double emin = ln + (ring ? 1 : 0);
                if (rl > emin) emin = rl;
                if (length + tpearl - emin > 0) pstake = length + tpearl - emin;
                preach = snew ? (double)fdiv((long long)emin + (long long)sdiv - 1, (long long)sdiv) + emin - 2.0 : emin - 1;
                if (preach < 1) preach = 1;
                else if (preach > reach_cap) preach = reach_cap;
            }
            // (9 Oct, CT_FEAR) outnumbered in view: any of their heads may start the trade, whatever its length
            const bool ct_out = Pp.CT_FEAR > 0 && n_en >= n_fr + 1 + (int)Pp.CT_FEAR_M && !(qsafe > 0) &&
                                est <= length + Pp.CT_FEAR_TOL;
            if (ct_out && stake < Pp.CT_FEAR) stake = Pp.CT_FEAR;
            if (stake > 0 && n_fr >= n_en && Pp.THREAT_OUTNUM != 1.0 && length <= Pp.THREAT_OUTNUM_MAXLEN && !(qsafe > 0) && !qtrade) {
                stake *= Pp.THREAT_OUTNUM;          // (v2.13) a friend for every other enemy: the drops would be ours
                pstake *= Pp.THREAT_OUTNUM;
                if (stake <= 0 && pstake <= 0) continue;
            }
            if (stake <= 0 && pstake > 0) {
                stake = 0.0;                        // (v2.13) only its pearl tiles are a threat
            } else if (stake <= 0) {
                if (T(teven) && est <= length + teven_slack && n_en >= n_fr && rnd < teven_round) {
                    stake = teven;
                } else if (!(T(keep_even) && est == length && length >= Pp.KEEP_EVEN_LEN && did > id && n_en > n_fr)) {
                    continue;
                } else {
                    stake = keep_even;
                }
            }
            int er = ROWS[ei], ec = COLS[ei];
            int frem[49];                           // (8 Oct, THREAT_FUEL) steps left on arrival, pearls eaten on the way
            const bool fuel = T(Pp.THREAT_FUEL) || (qsafe > 0 && T(Pp.Q_FUEL));   // (8 Oct, Q_FUEL) the queen's own map
            if (fuel) {
                for (int i = 0; i < 49; i++) frem[i] = -1;
                frem[ei] = (int)reach;
                std::vector<int> fq{ei};
                for (size_t qi = 0; qi < fq.size() && qi < 400; qi++) {
                    const int u = fq[qi];
                    if (frem[u] < 1) continue;
                    for (int k = u * 4; k < u * 4 + 4; k++) {
                        const int v = mv[k];
                        if (v < 0 || (occ[v] && !(qpass > 0 && occ[v] == 3 && freeat[v] <= qpass))) continue;
                        const int r = frem[u] - 1 + (wp[v] ? 1 : 0);
                        if (r > frem[v]) {
                            frem[v] = r;
                            fq.push_back(v);
                        }
                    }
                }
            }
            double k1 = Pp.KEEP1, k2 = Pp.KEEP2;
            for (size_t qi = 0; qi < q.size(); qi++) {
                int u = q[qi];
                int nd = dist[u] + 1;
                for (int k = u * 4; k < u * 4 + 4; k++) {
                    int v = mv[k];
                    if (v >= 0 && dist[v] < 0 && (!occ[v] || (qpass > 0 && occ[v] == 3 && freeat[v] <= qpass))) {
                        dist[v] = nd;
                        double f;
                        if (nd <= reach || (fuel && frem[v] >= 0)) {
                            // it has to SEE my head to aim at it: 7x7 around its own head
                            int dr = ROWS[v] - er, dc = COLS[v] - ec;
                            if (-3 <= dr && dr <= 3 && -3 <= dc && dc <= 3) {
                                f = nd == 1 ? 1.0 : (nd == 2 ? 0.9 : (nd == 3 ? 0.8 : 0.7));
                                if (stake > 0 || wp[v]) treach[v] = 1;
                            } else {
                                f = k1;
                            }
                        } else if (nd == reach + 1) {
                            f = k1;
                        } else {
                            f = k2;
                        }
                        double w = ((T(pstake) && wp[v] && nd <= preach) ? pstake : stake) * f;
                        if (w > tpen[v]) tpen[v] = w;
                        if (nd < reach + 2 || (fuel && frem[v] >= 1)) q.push_back(v);
                    }
                }
            }
            if (chk24 && !qhthr && -3 <= 3 - er && 3 - er <= 3 && -3 <= 3 - ec && 3 - ec <= 3) {
                for (int k = 96; k < 100; k++) {
                    int u = mv[k];
                    if (u >= 0 && dist[u] >= 0 && dist[u] + 1 <= reach) {
                        qhthr = true;
                        break;
                    }
                }
            }
        }
        if (use_qtb && !qtb.empty() && qsafe > 0 && grade) {
            // (v3q, Q_TAIL) an enemy that moves after me may split instead: its tail becomes the head of a child that
            // moves later in this round, as long as the parent less two segments
            std::map<int, int> eest;
            for (auto& en : enemies) eest[en.did] = en.est;
            double k1 = Pp.KEEP1, k2 = Pp.KEEP2;
            std::vector<int> order;
            for (auto& e : qtb) order.push_back(e.first);
            std::sort(order.begin(), order.end());
            for (int tdid : order) {
                if (last_round && tdid < id) continue;
                const std::vector<std::pair<int, int>>* segs = nullptr;
                for (auto& e : qtb)
                    if (e.first == tdid) {
                        segs = &e.second;
                        break;
                    }
                auto eit = eest.find(tdid);
                double est = eit != eest.end() ? (double)eit->second
                                               : (double)(seg_n.get(tdid, 0) + (seg_ring.has(tdid) ? 2 : 0));
                if (est < 4) continue;
                bool pv[49];
                for (int i = 0; i < 49; i++) pv[i] = false;
                for (auto& sg : *segs) {
                    int r_ = ROWS[sg.first] + DYS[sg.second], c_ = COLS[sg.first] + DXS[sg.second];
                    if (0 <= r_ && r_ < 7 && 0 <= c_ && c_ < 7) pv[r_ * 7 + c_] = true;
                }
                int tl = -1;
                for (auto& sg : *segs)
                    if (!pv[sg.first] && !RING[sg.first]) {
                        tl = sg.first;
                        break;
                    }
                if (tl < 0) continue;
                double n = est - 2;
                double reach = snew ? (double)fdiv((long long)n + (long long)sdiv - 1, (long long)sdiv) + n - 2.0 : n - 1;
                if (reach < 1) reach = 1;
                else if (reach > reach_cap) reach = reach_cap;
                int er = ROWS[tl], ec = COLS[tl];
                int dist[49];
                for (int i = 0; i < 49; i++) dist[i] = -1;
                dist[tl] = 0;
                std::vector<int> q;
                q.reserve(49);
                q.push_back(tl);
                for (size_t qi = 0; qi < q.size(); qi++) {
                    int u = q[qi];
                    int nd = dist[u] + 1;
                    for (int k = u * 4; k < u * 4 + 4; k++) {
                        int v = mv[k];
                        if (v >= 0 && dist[v] < 0 && (!occ[v] || (qpass > 0 && occ[v] == 3 && freeat[v] <= qpass))) {
                            dist[v] = nd;
                            double f;
                            if (nd <= reach) {
                                int dr = ROWS[v] - er, dc = COLS[v] - ec;
                                if (-3 <= dr && dr <= 3 && -3 <= dc && dc <= 3) {
                                    f = nd == 1 ? 1.0 : (nd == 2 ? 0.9 : (nd == 3 ? 0.8 : 0.7));
                                    treach[v] = 1;
                                } else {
                                    f = k1;
                                }
                            } else if (nd == reach + 1) {
                                f = k1;
                            } else {
                                f = k2;
                            }
                            double w = qsafe * f;
                            if (w > tpen[v]) tpen[v] = w;
                            if (nd < reach + 2) q.push_back(v);
                        }
                    }
                }
                if (chk24 && !qhthr && -3 <= 3 - er && 3 - er <= 3 && -3 <= 3 - ec && 3 - ec <= 3) {
                    for (int k = 96; k < 100; k++) {
                        int u = mv[k];
                        if (u >= 0 && dist[u] >= 0 && dist[u] + 1 <= reach) {
                            qhthr = true;
                            break;
                        }
                    }
                }
            }
        }
        if (T(Pp.TW) && !is_queen && !(T(Pp.TW_UNTIL) && rnd >= Pp.TW_UNTIL)) {
            // (7 Oct, TW) the longest enemy head in view: where (else pass on a warning heard)
            int bi = -1, be = 0;
            for (auto& en : enemies)
                if ((double)en.est >= Pp.TW_MINLEN && en.est > be) {
                    be = en.est;
                    bi = en.ei;
                }
            tw_out = bi >= 0 ? tw_val(wg[bi], be, 0, 0) : tw_relay;
        }
        if (qsafe > 0 && T(Pp.TW) && grade && !tw_mem.empty()) {
            // (7 Oct, TW) what friends saw: an enemy beyond my view threatens the tiles it could reach by now
            const double tww = qsafe * Pp.TW_W;
            for (size_t k = 0; k < tw_mem.size();) {
                const int g0 = tw_mem[k][0], r0 = tw_mem[k][1], e0 = tw_mem[k][2];
                const int age = rnd - r0;
                if (age > Pp.TW_AGE) {
                    tw_mem.erase(tw_mem.begin() + (long)k);
                    continue;
                }
                k++;
                if (gi_get(g0) >= 0) continue;        // that tile is in my view: I see for myself
                long long f0 = snew ? fdiv((long long)e0 + (long long)sdiv - 1, (long long)sdiv) : 1;
                long long R = snew ? (f0 + e0 - 2) : (e0 - 1);
                if (R < 1) R = 1;
                else if (R > reach_cap) R = (long long)reach_cap;
                R += (long long)(age < 1 ? 1 : age) * f0;
                const int gx = g0 % W, gy = g0 / W;
                for (int i = 0; i < 49; i++) {
                    if (occ[i]) continue;
                    const int gi_ = wg[i];
                    int dx = iabs(gi_ % W - gx);
                    if (dx > W - dx) dx = W - dx;
                    int dy = iabs(gi_ / W - gy);
                    if (dy > H - dy) dy = H - dy;
                    const int dd = dx + dy;
                    if (dd <= R) {
                        const double w = tww * (1.0 - (double)dd / ((double)R + 1.0));
                        if (w > tpen[i]) tpen[i] = w;
                    }
                }
            }
        }
        if (qsafe > 0 && T(Pp.Q_MEM) && grade) {
            // (v3q, Q_MEM) enemies the queen saw lately and cannot see now still threaten what they could reach since
            std::vector<int> vis;
            for (auto& en : enemies) {
                vis.push_back(en.did);
                emem[en.did] = {wg[en.ei], rnd, en.est};
            }
            const double qmw = qsafe * Pp.Q_MEM_W;
            for (auto it = emem.begin(); it != emem.end();) {
                int g0 = it->second[0], r0 = it->second[1], e0 = it->second[2];
                int age = rnd - r0;
                if (age > Pp.Q_MEM) {
                    it = emem.erase(it);
                    continue;
                }
                bool seen_now = std::find(vis.begin(), vis.end(), it->first) != vis.end();
                ++it;
                if (age < 1 || seen_now) continue;
                long long f0 = snew ? fdiv((long long)e0 + (long long)sdiv - 1, (long long)sdiv) : 1;
                long long R = snew ? (f0 + e0 - 2) : (e0 - 1);
                if (R < 1) R = 1;
                else if (R > reach_cap) R = (long long)reach_cap;
                R += (long long)age * f0;
                int gx = g0 % W, gy = g0 / W;
                for (int i = 0; i < 49; i++) {
                    if (occ[i]) continue;
                    int gi_ = wg[i];
                    int dx = iabs(gi_ % W - gx);
                    if (dx > W - dx) dx = W - dx;
                    int dy = iabs(gi_ / W - gy);
                    if (dy > H - dy) dy = H - dy;
                    int dd = dx + dy;
                    if (dd <= R) {
                        double w = qmw * (1.0 - (double)dd / ((double)R + 1.0));
                        if (w > tpen[i]) tpen[i] = w;
                    }
                }
            }
        }

        // (v2.12) the assassins (ASSN, off)
        bool assn_ = false;
        scout_goal = -1;
        if (T(Pp.ASSN) && rnd >= Pp.ASSN_ROUND && !lead && length <= Pp.ASSN_MAXLEN && ucount >= 3 &&
            !(is_queen && T(Pp.Q_NORAM_ALL))) {
            auto& pr = prey;
            auto& kpx = king_pos;
            if (pr && rnd - pr->b <= Pp.HUNT_MEM) {
                if (!T(Pp.ASSN_BEHIND) || !kpx || pr->c + Pp.ASSN_BEHIND > kpx->c) assn_ = true;
            } else if (T(Pp.ASSN_SCOUT) && fmod(id, (long long)Pp.ASSN_SCOUT) == 0 && kpx &&
                       rnd - kpx->b <= Pp.KING_FRESH) {
                int tx = W - 1 - (int)fmod(kpx->a, W);
                int ty = H - 1 - (int)fdiv(kpx->a, W);
                int dx = iabs(tx - hx), dy = iabs(ty - hy);
                if (dx > W - dx) dx = W - dx;
                if (dy > H - dy) dy = H - dy;
                if (dx + dy > 3) {
                    scout_goal = ty * W + tx;
                    assn_ = true;
                }
            }
        }
        assn = assn_;

        // (8 Oct, QG) the enemy head nearest our queen's head, if she is in my view and it is within QG_R of her
        qg_ei = -1;
        if (T(Pp.QG) && !is_queen && (double)length <= Pp.QG_MAXLEN && !enemies.empty()) {
            int qhi = -1, qdid = -1;
            for (auto& h : heads)
                if (!h.en && (h.did == 0 || h.did == 1)) {
                    qhi = h.i;
                    qdid = h.did;
                }
            if (qhi >= 0 && (double)seg_n.get(qdid, 0) >= Pp.QG_MINQ) {
                int bd = (int)Pp.QG_R + 1;
                for (auto& en : enemies) {
                    const int dd = iabs(ROWS[en.ei] - ROWS[qhi]) + iabs(COLS[en.ei] - COLS[qhi]);
                    if (dd < bd) {
                        bd = dd;
                        qg_ei = en.ei;
                    }
                }
            }
        }
