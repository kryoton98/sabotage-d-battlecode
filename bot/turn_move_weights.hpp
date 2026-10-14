// Sabotage-d / turn_move_weights.hpp, part 9 of the turn: the weights and tables the scoring loop uses - space,
// crowd and threat weights, the queen's look-ahead (Q_LOOK), portal phase, the route planner's targets (PLAN_W),
// one-way passages (OWP), corridor yield (CY), claims (CLAIM4) and spreading out (APART).
// Statements of Brain::turn_(), in the order they run: included once from turn.hpp, inside the function. This part
// reads what the parts before it declared and declares what the parts after it use.

        double food_best = 0.0;
        double food_one = 0.0;
        std::vector<Scored> scored;
        work = (long long)Pp.WORK;
        if (T(Pp.WORK_LONG) && length >= Pp.WORK_LONG_LEN) work = (long long)Pp.WORK_LONG;
        double hunt_w = (ucount >= 2 && length <= Pp.FIGHT_MAX_LEN) ? Pp.HUNT_W : 0.0;
        double space_w = length >= 6 ? Pp.SPACE_W * (length < 30 ? length : 30) / 10.0 : 0.0;
        if (is_king && enemies.empty()) space_w *= Pp.KING_SPACE;
        double soft_w = Pp.SOFT_W;
        if (qs && Pp.Q_JAM > 0) soft_w *= Pp.Q_JAM;      // (v3q) a jam the queen walks into may not clear in time
        const double qexit = (is_queen || (T(Pp.Q_EXIT_UNTIL) && rnd >= Pp.Q_EXIT_UNTIL)) ? 0.0 : Pp.Q_EXIT;   // (3 Oct, Q_EXIT_UNTIL)
        const double qclear = (is_queen || (T(Pp.Q_CLEAR_NK) && (kid_ == 0 || kid_ == 1)) ||
                               (T(Pp.Q_CLEAR_AREA) && (double)W * H < Pp.Q_CLEAR_AREA)) ? 0.0 : Pp.Q_CLEAR;
        const int qclear_r = (int)Pp.Q_CLEAR_R;
        const double qsolo = (qs && !(T(Pp.Q_CLEAR_NK) && lead_)) ? Pp.Q_SOLO : 0.0;   // (v3q) the king waits for its feeders
        double qlook = 0.0;
        struct QEd {
            int de[49];
            double re, fe;
        };
        std::vector<QEd> qed;
        long long qfree = 1;
        const double qlook_n = Pp.Q_LOOK_N;
        if (Pp.Q_LOOK > 0 && qsafe > 0 && grade && !enemies.empty()) {
            // (v3q, Q_LOOK) how soon each enemy in view gets to each tile in view: (distance, full reach, free steps)
            qlook = Pp.Q_LOOK;
            const long long sdv = (long long)Pp.SPRINT_DIV;
            for (auto& en : enemies) {
                QEd e;
                for (int i = 0; i < 49; i++) e.de[i] = -1;
                e.de[en.ei] = 0;
                std::vector<int> qq{en.ei};
                for (size_t qi = 0; qi < qq.size(); qi++) {
                    int u = qq[qi];
                    int nd = e.de[u] + 1;
                    for (int k = u * 4; k < u * 4 + 4; k++) {
                        int x = mv[k];
                        if (x >= 0 && e.de[x] < 0 && !occ[x]) {
                            e.de[x] = nd;
                            qq.push_back(x);
                        }
                    }
                }
                double fe, re;
                if (snew) {
                    fe = (double)fdiv((long long)en.est + sdv - 1, sdv);
                    re = fe + en.est - 2;
                } else {
                    fe = 1;
                    re = en.est - 1;
                }
                if (fe < 1) fe = 1;
                if (re < 1) re = 1;
                else if (re > reach_cap) re = reach_cap;
                e.re = re;
                e.fe = fe;
                qed.push_back(e);
            }
            if (snew) qfree = fdiv((long long)length + sdv - 1, sdv);
        }
        double terr_w = Pp.TERR_W;
        double crowd_w = Pp.CROWD_W;
        bool space_free = T(Pp.SPACE_FREE);
        double threat_w = Pp.THREAT_W * (1.0 + Pp.THREAT_LEN * (length < 24 ? length : 24));
        double food_w = 1.0;
        double pocket_f = 1.0 + Pp.POCKET_L2 * length * length;
        if (Pp.CAP_JAM != 1.0 && ucount >= limit) soft_w *= Pp.CAP_JAM;
        if (rnd >= Pp.BANK_ROUND && length >= Pp.BANK_LEN) {
            soft_w *= Pp.BANK_SAFETY;
            threat_w *= Pp.BANK_SAFETY;
            food_w = Pp.BANK_FOOD;
        }
        if (is_king) food_w *= Pp.KING_FOOD_W;
        int food_goal_ = T(Pp.FOOD_STICK) ? food_goal : -1;
        int goal_dist = 99;
        int goal_d = -1;
        int best_t[4] = {-1, -1, -1, -1};        // direction -> the pearl that draws it most (-1: none)
        int pperiod = has_ptiles ? (int)Pp.PORTAL_PERIOD : 0;
        int pph = 0, pnw = 0, pse = 0, pocc = 0;
        if (pperiod) {
            pph = (int)fmod(rnd, pperiod);
            pnw = 1;
            pse = 1 + (int)fdiv(pperiod, 2);
            pocc = length < pperiod ? length : pperiod;
        }
        int surv_of[4] = {NOVAL, NOVAL, NOVAL, NOVAL};   // direction -> steps I can keep moving after it
        // (v3, PLAN_W) the route planner's targets: the nearest pearls no rival head is nearer to, and their distances
        std::vector<int> ptg;
        int pav[49];
        std::vector<std::array<int, 49>> pdd;
        int plan_t = (int)Pp.PLAN_T;
        if (T(Pp.PLAN_W) && !targets.empty()) {
            int hd[49];
            for (int i = 0; i < 49; i++) hd[i] = -1;
            hd[24] = 0;
            int q[49];
            int qn = 0;
            q[qn++] = 24;
            for (int qi = 0; qi < qn; qi++) {
                int u = q[qi];
                int nd = hd[u] + 1;
                for (int k = u * 4; k < u * 4 + 4; k++) {
                    int x = mv[k];
                    if (x >= 0 && hd[x] < 0 && !occ[x]) {
                        hd[x] = nd;
                        q[qn++] = x;
                    }
                }
            }
            std::vector<std::pair<int, int>> ct;
            int plan_open = (int)Pp.PLAN_OPEN;
            for (int t : targets) {
                int dt_ = hd[t];
                if (dt_ < 0) continue;
                if (plan_open) {
                    auto& nl_ = nb(wg[t]);
                    if ((nl_[0] != -1) + (nl_[1] != -1) + (nl_[2] != -1) + (nl_[3] != -1) < plan_open) continue;
                }
                int tr = ROWS[t], tc = COLS[t];
                bool ok_ = true;
                for (auto& rc : rivals)
                    if (iabs(rc.first - tr) + iabs(rc.second - tc) < dt_) {
                        ok_ = false;
                        break;
                    }
                if (ok_) ct.push_back({dt_, t});
            }
            std::sort(ct.begin(), ct.end());
            int kk = (int)Pp.PLAN_K;
            for (int j = 0; j < (int)ct.size() && j < kk; j++) ptg.push_back(ct[(size_t)j].second);
            for (int t : ptg) {
                pav[t] = wp[t] ? 0 : wcd[t] + 1;
                std::array<int, 49> dd;
                for (int i = 0; i < 49; i++) dd[(size_t)i] = -1;
                dd[(size_t)t] = 0;
                qn = 0;
                q[qn++] = t;
                for (int qi = 0; qi < qn; qi++) {
                    int u = q[qi];
                    int nd = dd[(size_t)u] + 1;
                    for (int k = u * 4; k < u * 4 + 4; k++) {
                        int x = mv[k];
                        if (x >= 0 && dd[(size_t)x] < 0 && !occ[x]) {
                            dd[(size_t)x] = nd;
                            q[qn++] = x;
                        }
                    }
                }
                pdd.push_back(dd);
            }
        }
        const double owp = (T(Pp.OWP) && (!T(Pp.OWP_FRIEND) || n_fr > 0)) ? Pp.OWP : 0.0;   // (3 Oct, OWP)
        int owp_pk[49];
        const bool use_owp_pk = owp > 0 && T(Pp.OWP_NOPOCKET);
        if (use_owp_pk) owp_pockets(mv, owp_pk);
        const double cy = Pp.CY;   // (3 Oct, CY) friendly heads in view by window index
        int cy_head[49];
        bool cy_any = false;
        if (cy > 0) {
            for (int i = 0; i < 49; i++) cy_head[i] = -1;
            for (const auto& h : heads)
                if (!h.en && h.did != id) {
                    cy_head[h.i] = h.did;
                    cy_any = true;
                }
        }
        // (v3.9, CLAIM4) pearls in view that a friend in view claims: it needs fewer pearls to its next 4k+1 length (a free
        // step), then it arrives earlier, then it has the lower id - provided it beats every enemy there (or neither of us does)
        uint8_t claimed4[49] = {0};
        if (T(Pp.CLAIM4) && !friends.empty() && !targets.empty()) {
            auto wbfs = [&](int s0, int* dd) {
                for (int i = 0; i < 49; i++) dd[i] = -1;
                dd[s0] = 0;
                int qq[49];
                int qn = 0;
                qq[qn++] = s0;
                for (int qi = 0; qi < qn; qi++) {
                    const int u = qq[qi];
                    for (int k = u * 4; k < u * 4 + 4; k++) {
                        const int x = mv[k];
                        if (x < 0 || dd[x] >= 0 || occ[x]) continue;
                        dd[x] = dd[u] + 1;
                        qq[qn++] = x;
                    }
                }
            };
            auto next4k1 = [](int L) {
                int m = 5;
                while (m <= L) m += 4;
                return m;
            };
            auto turns = [](int dist, int L) {
                const int f = ((L < 2 ? 2 : L) + 3) / 4;
                return (dist + f - 1) / f;
            };
            int myd[49], en_arr[49], tmp[49];
            wbfs(24, myd);
            for (int i = 0; i < 49; i++) en_arr[i] = 1 << 20;
            for (auto& en : enemies) {
                wbfs(en.ei, tmp);
                for (int t : targets)
                    if (tmp[t] >= 0 && turns(tmp[t], en.est) < en_arr[t]) en_arr[t] = turns(tmp[t], en.est);
            }
            struct FC {
                int len, id;
                int dd[49];
            };
            std::vector<FC> fcs;
            for (auto& h : heads) {
                if (h.en) continue;
                FC f;
                f.len = seg_n.get(h.did, 2);
                f.id = h.did;
                wbfs(h.i, f.dd);
                fcs.push_back(f);
            }
            const int my_need = next4k1(length) - length;
            for (int t : targets) {
                if (myd[t] < 0) continue;
                const int my_arr = turns(myd[t], length);
                const bool me_ok = my_arr < en_arr[t];
                for (auto& f : fcs) {
                    if (f.dd[t] < 0) continue;
                    const int f_arr = turns(f.dd[t], f.len);
                    const bool f_ok = f_arr < en_arr[t];
                    if (me_ok && !f_ok) continue;
                    const int f_need = next4k1(f.len) - f.len;
                    if ((!me_ok && f_ok) || std::make_tuple(f_need, f_arr, f.id) < std::make_tuple(my_need, my_arr, id)) {
                        claimed4[t] = 1;
                        break;
                    }
                }
            }
        }
        // (9 Oct, APART) the friendly heads in view that push me away this turn
        const int apart_cap = (int)Pp.APART_CAP;
        std::vector<int> apart_fr;
        if (Pp.APART > 0 && !is_queen && !friends.empty() && (double)rnd < Pp.APART_UNTIL && (double)length <= Pp.APART_MAXLEN &&
            !lead_ && !qhunt && !assn_ && !guard_now && !mg_walk && !mg_pull) {
            for (auto& h : heads)
                if (!h.en && (T(Pp.APART_Q) || h.did > 1)) apart_fr.push_back(h.i);
        }
