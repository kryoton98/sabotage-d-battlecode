// Sabotage-d / turn_move_targets.hpp, part 7 of the turn: "3. move" begins - the candidate steps (cands), how fast
// other heads get to each tile (territory), the food targets, the dead-end pockets in view, and how deep the
// survival search must look (need_depth).
// Statements of Brain::turn_(), in the order they run: included once from turn.hpp, inside the function. This part
// reads what the parts before it declared and declares what the parts after it use.

        // ------------------------------------------------------- 3. move
        std::vector<Cand> cands;
        std::vector<Cand> blind_q;
        auto nbh = nb(hg);
        bool portal_body = T(Pp.PORTAL_BODY);
        for (int d = 0; d < 4; d++) {
            int v = mv[d + 96];
            if (v == -1) continue;
            int g2 = nbh[(size_t)d];
            if (v >= 0) {
                if (freeat[v] > 1) continue;
                cands.push_back({d, v, g2, 0});
            } else if (v == -2) {
                int bg = body_at.get(g2);
                if (g2 >= 0 && (bg == NOVAL ? 0 : bg) <= 1 && (tn >= length || !portal_body)) {
                    if (is_queen && q_noport(rnd))
                        blind_q.push_back({d, -1, g2, 1});  // (Q_NOPORT) the queen: only if nothing else is left
                    else
                        cands.push_back({d, -1, g2, 1});    // known portal, far side out of sight
                }
            } else if (tn >= length || !portal_body) {
                if (is_queen && (q_noblind(rnd) || q_noport(rnd)))
                    blind_q.push_back({d, -1, -1, 2});      // (v3q) the queen: only if nothing else is left
                else
                    cands.push_back({d, -1, -1, 2});        // blind portal
            }
        }
        if (!blind_q.empty() && cands.empty()) cands = blind_q;
        const bool qnp = T(Pp.Q_NOPOCKET) && (qs || (is_queen && T(Pp.Q_NOPOCKET_ALL)));
        if (cands.empty()) {
            std::string alt;
            if (!qnp && reverse_split(length, ucount, tn, mv, occ, alt)) return alt;
            return boxed_in(mv, occ, heads, length, ucount, mydir, tn);
        }

        // how fast the other heads in view get to each tile (for territory)
        bool has_odist = false;
        int odist[49];
        std::vector<Head> terr_heads = heads;
        if (!heads.empty() && !T(Pp.KING_TERR) && rnd >= guard_round && length >= Pp.GUARD_KING && lead_) {
            terr_heads.clear();
            for (auto& h : heads)
                if (h.en) terr_heads.push_back(h);
        }
        bool rally = T(Pp.RALLY) && !enemies.empty() && !friends.empty() && rnd < Pp.RALLY_ROUND &&
                     length <= Pp.RALLY_MAXLEN;
        if (rally && T(Pp.RALLY_TERR)) {
            terr_heads.clear();
            for (auto& h : heads)
                if (h.en) terr_heads.push_back(h);
        }
        if (!terr_heads.empty()) {
            has_odist = true;
            for (int i = 0; i < 49; i++) odist[i] = -1;
            std::vector<int> q;
            for (auto& h : terr_heads) {
                odist[h.i] = 0;
                q.push_back(h.i);
            }
            for (size_t qi = 0; qi < q.size(); qi++) {
                int u = q[qi];
                int nd = odist[u] + 1;
                for (int k = u * 4; k < u * 4 + 4; k++) {
                    int x = mv[k];
                    if (x >= 0 && odist[x] < 0 && !occ[x]) {
                        odist[x] = nd;
                        q.push_back(x);
                    }
                }
            }
        }

        // food: pearls, and tiles that spawn one within 12 rounds
        bool pocket_run = T(Pp.POCKET_RUN) && ucount < limit - 1 && !qnp;
        if (pocket_run && T(Pp.POCKET_TAIL) && tn + 2 < length) pocket_run = false;
        if (pocket_run && T(Pp.KING_POCKET) && lead_ && T(ksplit_round) && rnd >= ksplit_round &&
            length >= Pp.BEACON_LEN)
            pocket_run = false;
        double pocket_min = Pp.POCKET_MIN;
        struct Finish {
            int d;
            double pk;
            double pen;
        };
        std::vector<Finish> finish;
        bool fount = T(Pp.FOUNT) && length <= Pp.FOUNT_MAXLEN;
        double fount_cd = Pp.FOUNT_CD;
        double terr_food = length <= Pp.FOUNT_MAXLEN ? Pp.TERR_FOOD : 0.0;
        const TileSet* fonly = (fount && T(Pp.FOUNT_ONLY)) ? &founts : nullptr;
        bool stock = T(Pp.STOCK_ROUND) && rnd >= Pp.STOCK_ROUND && length < Pp.STOCK_LEN &&
                     (quiet_ || (T(Pp.STOCK_ALL) && enemies.empty()));
        stock_now = stock;
        bool prun = false;
        std::vector<int> targets;
        bool has_pocket = false;
        int pocket[49];
        if (!plain && T(Pp.POCKET_PEEL)) {
            // (v2.8) dead-end pockets in view, peeled from their ends
            int deg[49];
            for (int i = 0; i < 49; i++) {
                int n = 0;
                for (int k = i * 4; k < i * 4 + 4; k++) {
                    int x = mv[k];
                    if (x >= 0) n += 1;
                    else if (x <= -2) n += 2;
                }
                deg[i] = n;
            }
            has_pocket = true;
            for (int i = 0; i < 49; i++) pocket[i] = 0;
            std::vector<int> stack;
            for (int i = 0; i < 49; i++)
                if (deg[i] <= 1) stack.push_back(i);
            while (!stack.empty()) {
                int i = stack.back();
                stack.pop_back();
                if (pocket[i]) continue;
                pocket[i] = 1;
                for (int k = i * 4; k < i * 4 + 4; k++) {
                    int x = mv[k];
                    if (x >= 0 && !pocket[x]) {
                        deg[x] -= 1;
                        if (deg[x] <= 1) stack.push_back(x);
                    }
                }
            }
            int npk = 0;
            for (int i = 0; i < 49; i++)
                if (pocket[i] && (wp[i] || (fount && 0 < wcd[i] && wcd[i] <= fount_cd &&
                                            (fonly == nullptr || fonly->has(wg[i])))))
                    npk++;
            if (!(pocket_run && npk >= pocket_min && length + npk >= 4)) {
                for (int i = 0; i < 49; i++)
                    if (pocket[i] && (wp[i] || wcd[i] > 0)) hot.erase(wg[i]);
            } else {
                has_pocket = false;
                prun = fount;
            }
        }
        for (int i = 0; i < 49; i++) {
            if ((wp[i] || (0 < wcd[i] && wcd[i] <= 12)) && !occ[i]) {
                int i4 = i * 4;
                if ((mv[i4] == -1) + (mv[i4 + 1] == -1) + (mv[i4 + 2] == -1) + (mv[i4 + 3] == -1) >= 3 && !prun) {
                    hot.erase(wg[i]);             // a pearl in a dead-end pocket is bait, not food
                    continue;
                }
                if (has_pocket && pocket[i]) continue;
                if (banned[(size_t)wg[i]] > rnd) continue;   // found to be in a pocket when I stood next to it
                if (stock && dense(wg[i])) continue;         // (v2.11) the field is the king's
                targets.push_back(i);
            }
        }
        std::vector<std::pair<int, int>> rivals;
        if (T(Pp.CONTEST)) {
            for (auto& h : heads)
                if (!h.en || seg_n.get(h.did, 0) > length) rivals.push_back({ROWS[h.i], COLS[h.i]});
        } else {
            for (auto& h : heads) rivals.push_back({ROWS[h.i], COLS[h.i]});
        }
        // (v3, FOOD_ETA) the rivals' steps to each pearl by their paths over the tiles in view they may enter (no path in view:
        // FOOD_ETA_H, or the Manhattan distance if more)
        std::vector<std::array<int, 49>> eta_r;
        const bool use_eta = T(Pp.FOOD_ETA) && !targets.empty() && !rivals.empty();
        if (use_eta) {
            const int feh = (int)Pp.FOOD_ETA_H;
            for (auto& rc : rivals) {
                std::array<int, 49> dd;
                dd.fill(-1);
                const int s0 = rc.first * 7 + rc.second;
                dd[(size_t)s0] = 0;
                int q[49];
                int qh = 0, qt = 0;
                q[qt++] = s0;
                while (qh < qt) {
                    const int u = q[qh++];
                    const int nd = dd[(size_t)u] + 1;
                    for (int k = u * 4; k < u * 4 + 4; k++) {
                        const int x = mv[k];
                        if (x >= 0 && dd[(size_t)x] < 0 && freeat[x] <= nd) {
                            dd[(size_t)x] = nd;
                            q[qt++] = x;
                        }
                    }
                }
                std::array<int, 49> e;
                e.fill(0);
                for (int t : targets) {
                    if (dd[(size_t)t] >= 0) {
                        e[(size_t)t] = dd[(size_t)t];
                    } else {
                        const int m = iabs(ROWS[t] - rc.first) + iabs(COLS[t] - rc.second);
                        e[(size_t)t] = m > feh ? m : feh;
                    }
                }
                eta_r.push_back(e);
            }
        }
        // (v2.9) the king's table (GUARD_FOOD_R, off)
        bool has_kingfood = false;
        int kingfood[49];
        if (kid_ >= 0 && !lead_ && T(Pp.GUARD_FOOD_R) && rnd >= guard_round) {
            int kx = (int)fmod(kp->a, W), ky = (int)fdiv(kp->a, W);
            has_kingfood = true;
            for (int i = 0; i < 49; i++) kingfood[i] = 0;
            double gfr = Pp.GUARD_FOOD_R;
            for (int i : targets) {
                int g = wg[i];
                int dx = iabs(g % W - kx), dy = iabs(g / W - ky);
                if (dx > W - dx) dx = W - dx;
                if (dy > H - dy) dy = H - dy;
                if (dx + dy <= gfr) kingfood[i] = 1;
            }
        }
        pearls.clear();
        for (int i = 0; i < 49; i++)
            if (wp[i]) pearls.add(wg[i]);
        int need_depth = length + 4;
        double kgm = length >= Pp.KING_GROW_LEN ? Pp.KING_GROW_MARGIN : 0;
        if (need_depth < 10) need_depth = 10;
        else if (need_depth > Pp.SURV_CAP) need_depth = (int)Pp.SURV_CAP;
        if (T(Pp.NEED_LEFT) && need_depth > 500 - rnd) need_depth = rnd < 499 ? 500 - rnd : 1;
