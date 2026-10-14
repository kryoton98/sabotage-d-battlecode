// Sabotage-d / turn_move_score.hpp, part 10 of the turn: the scoring loop - one score for every candidate step (food
// in reach, survival depth, threat, room, friends' exits, lanes and rooms, portals, and the queen's own terms).
// A score of -500 or below marks a step as fatal for what comes after the loop.
// Statements of Brain::turn_(), in the order they run: included once from turn.hpp, inside the function. This part
// reads what the parts before it declared and declares what the parts after it use.

        for (auto& cd_ : cands) {
            int d = cd_.d, v = cd_.v, g2 = cd_.g2, kind = cd_.kind;
            double s = 0.0;
            double food = 0.0;
            if (!apart_fr.empty()) {
                // (9 Oct, APART) the distance this step puts between me and each of them
                int gain = 0;
                for (int f : apart_fr) {
                    int d0 = iabs(ROWS[f] - 3) + iabs(COLS[f] - 3);
                    if (d0 > apart_cap) d0 = apart_cap;
                    int d1 = apart_cap;
                    if (kind == 0 && v >= 0) {
                        d1 = iabs(ROWS[f] - ROWS[v]) + iabs(COLS[f] - COLS[v]);
                        if (d1 > apart_cap) d1 = apart_cap;
                    }
                    gain += d1 - d0;
                }
                s += Pp.APART * gain * ((kind == 0 && v >= 0) ? 1.0 : Pp.APART_PORT);
            }
            double cbest_fv = 0.0;
            int cbest_t = -1;
            if (qs && Pp.Q_HOME > 0 && home_x >= 0 && !(T(Pp.Q_HOME_UNTIL) && rnd >= Pp.Q_HOME_UNTIL) &&
                !(T(Pp.Q_HOME_KING) && lead_ && rnd >= (T(Pp.Q_HOME_KING_FROM) ? Pp.Q_HOME_KING_FROM : guard_round)) &&   // (v3.9, Q_HOME_KING)
                !(T(Pp.Q_HOME_CALM) && rnd - last_enemy > Pp.Q_HOME_CALM) &&   // (3 Oct, Q_HOME; _UNTIL / _CALM: may roam)
                !(T(Pp.Q_HOME_HUNGRY) && last_eat >= 0 &&
                  (double)(rnd - (T(Pp.Q_HOME_HUNGRY_SEEN) && last_pearl_seen > last_eat ? last_pearl_seen : last_eat)) >= Pp.Q_HOME_HUNGRY)) {   // (8 Oct, QHH / QHHS)
                int dxh = std::abs(g2 % W - home_x);
                if (dxh > W - dxh) dxh = W - dxh;
                int dyh = std::abs(g2 / W - home_y);
                if (dyh > H - dyh) dyh = H - dyh;
                const double hr = Pp.Q_HOME + Pp.Q_HOME_GROW * length;   // (v3.9, Q_HOME_GROW) a longer queen needs more room
                if (dxh + dyh > hr) s -= Pp.Q_HOME_W * (dxh + dyh - hr);
            }
            if (!qs && is_queen && Pp.Q_HOME_SMALL > 0 && home_x >= 0 && g2 >= 0 && rnd >= Pp.Q_HOME_SMALL_FROM) {
                // (6 Oct, Q_HOME_SMALL) the small-map queen keeps to her home ground
                int dxh = std::abs(g2 % W - home_x);
                if (dxh > W - dxh) dxh = W - dxh;
                int dyh = std::abs(g2 / W - home_y);
                if (dyh > H - dyh) dyh = H - dyh;
                if (dxh + dyh > Pp.Q_HOME_SMALL) s -= Pp.Q_HOME_SMALL_W * (dxh + dyh - Pp.Q_HOME_SMALL);
            }
            if (qs && Pp.Q_FRESH > 0 && !viewed.empty() && kind == 0) {   // (3 Oct, Q_FRESH) where she has looked lately
                const int ddx = DXS[d], ddy = DYS[d];
                const double fresh_age = rnd - Pp.Q_FRESH_AGE;
                int nst = 0;
                for (int k = -3; k <= 3; k++) {
                    int e, t;
                    if (ddx) {
                        e = (3 + k) * 7 + 3 + 3 * ddx;
                        t = (int)fmod(hy + k, H) * W + (int)fmod(hx + 4 * ddx, W);
                    } else {
                        e = (3 + 3 * ddy) * 7 + 3 + k;
                        t = (int)fmod(hy + 4 * ddy, H) * W + (int)fmod(hx + k, W);
                    }
                    if (mv[e * 4 + d] != -1 && viewed[(size_t)t] < fresh_age) nst++;
                }
                s -= Pp.Q_FRESH * nst;
            }
            if (kind == 0) {
                int grow = wp[v];
                if (is_queen && T(Pp.Q_TUNNEL) && !(T(Pp.Q_TUNNEL_AREA) && (double)W * H >= Pp.Q_TUNNEL_AREA)) {
                    // (6 Oct, Q_TUNNEL) follow the corridor that starts at v: tiles with exactly one way on
                    int prev = 24, cur = v, tl = 0;
                    bool blind_end = false;
                    bool cfood = (wp[v] || (0 < wcd[v] && wcd[v] <= 12));
                    for (int st = 0; st < 12; st++) {
                        int exits = 0, nxt = -1;
                        bool unk = false;
                        for (int k = cur * 4; k < cur * 4 + 4; k++) {
                            const int x = mv[k];
                            if (x == -1 || x == prev) continue;
                            exits++;
                            if (x < 0) unk = true;
                            else nxt = x;
                        }
                        if (exits != 1) break;          // a junction (open again) or a dead end (the pocket checks)
                        tl++;
                        if (unk) {
                            blind_end = true;           // it runs out of my sight (or through a portal)
                            break;
                        }
                        prev = cur;
                        cur = nxt;
                        if (wp[cur] || (0 < wcd[cur] && wcd[cur] <= 12)) cfood = true;
                    }
                    if ((blind_end || tl >= Pp.Q_TUNNEL_MIN) && (!T(Pp.Q_TUNNEL_FOOD) || cfood)) s -= Pp.Q_TUNNEL;
                }
                if (lane_on && g2 >= 0 && ln_depth[(size_t)g2] > ln_depth[(size_t)hg]) {
                    // (4 Oct, LANE) a step deeper into a lane
                    if (is_queen && T(Pp.LANE_QNOENTER) && ln_depth[(size_t)hg] == 0) s -= 1000.0;
                    if (T(Pp.LANE_BUSY) && lane_others(g2, 60)) s -= Pp.LANE_BUSY;
                }
                if (grow && has_kingfood && kingfood[v]) s -= Pp.GUARD_FOOD_PEN;
                if (stock && (grow || T(Pp.STOCK_AVOID)) && T(Pp.STOCK_PEN) && dense(g2)) s -= Pp.STOCK_PEN;
                // --- distances from the tile I would step on (my body frees up as I go)
                int dist[49];
                for (int i = 0; i < 49; i++) dist[i] = -1;
                dist[v] = 0;
                int q[49];
                int qn = 0;
                q[qn++] = v;
                int off = 1 - grow;
                for (int qi = 0; qi < qn; qi++) {
                    int u = q[qi];
                    int nd = dist[u] + 1;
                    for (int k = u * 4; k < u * 4 + 4; k++) {
                        int x = mv[k];
                        if (x >= 0 && dist[x] < 0 && freeat[x] <= nd + off) {
                            dist[x] = nd;
                            q[qn++] = x;
                        }
                    }
                }
                // --- food: 1 / (turns until I eat it)^2, less if another head is nearer
                for (int t : targets) {
                    int dt = dist[t];
                    if (dt < 0) continue;
                    int arr = dt + 1;
                    if (!wp[t]) {
                        if (t == v) continue;
                        int c = wcd[t] + 1;
                        if (c > arr) arr = c;
                    }
                    double val = 1.0;
                    if (use_eta) {
                        for (auto& e : eta_r)
                            if (e[(size_t)t] < arr) {
                                val = 0.3;
                                break;
                            }
                    } else if (!rivals.empty()) {
                        int tr = ROWS[t], tc = COLS[t];
                        for (auto& rc : rivals)
                            if (iabs(rc.first - tr) + iabs(rc.second - tc) < arr) {
                                val = 0.3;
                                break;
                            }
                    }
                    if (has_kingfood) {
                        if (kingfood[t]) val = Pp.GUARD_FOOD;
                    } else if (king_i_ >= 0 && iabs(ROWS[king_i_] - ROWS[t]) + iabs(COLS[king_i_] - COLS[t]) <= 3) {
                        val = Pp.GUARD_FOOD;
                    } else if (defer_i >= 0 && iabs(ROWS[defer_i] - ROWS[t]) + iabs(COLS[defer_i] - COLS[t]) <= 3) {
                        val = Pp.GUARD_FOOD;
                    }
                    if (claimed4[t]) val *= 1.0 - (Pp.CLAIM4 > 1.0 ? 1.0 : Pp.CLAIM4);   // (v3.9, CLAIM4) a friend's
                    if (T(Pp.Q_PRIO) && !is_queen && qhead_i >= 0 &&
                        iabs(ROWS[t] - ROWS[qhead_i]) + iabs(COLS[t] - COLS[qhead_i]) < iabs(ROWS[t] - 3) + iabs(COLS[t] - 3))
                        val *= 1.0 - (Pp.Q_PRIO > 1.0 ? 1.0 : Pp.Q_PRIO);   // (8 Oct, QPRIO) the queen's pearl
                    if (qres_on && iabs(ROWS[t] - ROWS[qhead_i]) + iabs(COLS[t] - COLS[qhead_i]) <= Pp.QRES_R)
                        val = 0.0;                                         // (8 Oct, QRES) a pearl at our queen's head is hers
                    // (9 Oct, RESPAWN_W) this pearl's tile tries again soon after I get there: eaten, the tile is free for
                    // the next one; left lying, that try is lost
                    if (Pp.RESPAWN_W > 0 && wp[t] && (double)length <= Pp.RESPAWN_MAXLEN && wcd[t] > arr &&
                        (double)wcd[t] <= arr + Pp.RESPAWN_WIN)
                        val *= 1.0 + Pp.RESPAWN_W;
                    double fv = val / (double)(arr * arr);
                    food += fv;
                    if (fv > food_one) food_one = fv;
                    if (fv > cbest_fv) {
                        cbest_fv = fv;
                        cbest_t = t;
                    }
                    if (wg[t] == food_goal_ && dt < goal_dist) {
                        goal_dist = dt;
                        goal_d = d;
                    }
                }
                if (!grow && 1 <= wcd[v] && wcd[v] <= length + 1) food -= Pp.BLOCK_PEN;   // it fires while I am on it (was 0.4)
                if (!ptg.empty()) s += Pp.PLAN_W * plan_best(ptg, pav, pdd, dist, plan_t);
                if (T(hunt_w) && !enemies.empty()) {
                    for (auto& en : enemies) {
                        if (en.ln > length) {
                            int bd = 99;
                            for (int k = en.ei * 4; k < en.ei * 4 + 4; k++) {
                                int x = mv[k];
                                if (x >= 0 && 0 <= dist[x] && dist[x] < bd) bd = dist[x];
                            }
                            if (bd < 99) {
                                int gain = en.ln - length;
                                if (gain > 6) gain = 6;
                                s += hunt_w * gain / (bd + 2.0);
                            }
                        }
                    }
                }
                const PTile* pg2 = has_ptiles ? ptile(g2) : nullptr;
                if (pg2 != nullptr && T(Pp.PORTAL_LAND)) {
                    // (7 Oct, PORTAL_LAND) walking onto a landing tile (not crossing to it): a friend crossing from the far
                    // side lands on my head - both die
                    const PTile* ph_ = ptile(hg);
                    if (!(ph_ != nullptr && ph_->has[d])) s -= Pp.PORTAL_LAND;
                }
                if (pg2 != nullptr) {
                    bool rpc_waive = false;          // (3 Oct, ROUTE_PCLEAR) not the tile my route steps on next
                    if (T(Pp.ROUTE_PCLEAR) && has_route)
                        for (size_t k = 0; k < route.size() && k < 3; k++)
                            if (route[k] == g2) { rpc_waive = true; break; }
                    if (!rpc_waive) {
                        s -= Pp.PORTAL_CLEAR;
                        if (length >= Pp.PORTAL_CLEAR_LEN) s -= Pp.PORTAL_CLEAR_LONG;
                    }
                    if (pperiod) {
                        for (int oi = 0; oi < pg2->n; oi++) {
                            int pd = pg2->order[oi];
                            int bad = (pd == 0 || pd == 3) ? pse : pnw;
                            for (int k = 0; k < pocc; k++)
                                if (fmod(pph + k, pperiod) == bad) {
                                    s -= Pp.PORTAL_PHASE_W;
                                    break;
                                }
                        }
                    }
                }
                // --- long dragons need elbow room
                int sp = -1;
                if (T(space_w)) {
                    sp = 0;
                    if (space_free) {
                        for (int qi = 0; qi < qn; qi++)
                            if (freeat[q[qi]] == 0) sp += ext[q[qi]] ? 3 : 1;
                    } else {
                        for (int qi = 0; qi < qn; qi++) sp += ext[q[qi]] ? 3 : 1;
                    }
                    if (sp < 60) s -= space_w * (60 - sp) / 60.0;
                }
                // --- a room shared with other dragons must hold all of us (CROWD_W, off)
                if (T(crowd_w) && !heads.empty()) {
                    int cr = 0;
                    for (auto& h : heads) {
                        for (int k = h.i * 4; k < h.i * 4 + 4; k++) {
                            int x = mv[k];
                            if (x >= 0 && dist[x] >= 0) {
                                cr++;
                                break;
                            }
                        }
                    }
                    if (cr) {
                        if (sp < 0) {
                            sp = 0;
                            for (int qi = 0; qi < qn; qi++) sp += ext[q[qi]] ? 3 : 1;
                        }
                        int room = (2 * length + 4) * (1 + cr);
                        if (room > 60) room = 60;
                        if (sp < room) s -= crowd_w * (room - sp) / room;
                    }
                }
                // --- territory: tiles I reach strictly before any other head in view
                double terr_pen = 0.0;
                double tf_red = 0.0;
                if (has_odist) {
                    int terr = 0, tfood = 0;
                    for (int qi = 0; qi < qn; qi++) {
                        int x = q[qi];
                        int od = odist[x];
                        if (od < 0 || dist[x] < od) {
                            terr += ext[x] ? 3 : 1;
                            if (T(terr_food) && (wp[x] || (0 < wcd[x] && wcd[x] <= fount_cd))) tfood++;
                        }
                    }
                    int need = 2 * length + 4;
                    if (need > 30) need = 30;
                    if (terr < need) {
                        terr_pen = terr_w * (need - terr) / need;
                        tf_red = terr_pen;
                        if (tfood) {
                            terr_pen -= terr_food * tfood;
                            if (terr_pen < 0.0) terr_pen = 0.0;
                        }
                        tf_red -= terr_pen;
                        s -= terr_pen;
                    }
                }
                // --- can I keep moving from there? First with every other dragon frozen in place; if that
                // fails, with only kelp and my own body.
                int gk = grow + (int)kgm;
                int got = survive(g2, need_depth, body_at, gk, length, &others, &pearls);
                surv_of[d] = got;
                const bool lane_commit = lane_on && lane_dir == 1 && T(Pp.LANE_COMMIT) && g2 >= 0 &&
                                         ln_depth[(size_t)g2] > ln_depth[(size_t)hg];
                if (is_queen && Pp.Q_RUN > 0) {
                    // (9 Oct, Q_RUN) the queen does not count on anybody getting out of her way: a step after which she
                    // cannot make Q_RUN more moves with every other dragon standing where it is costs Q_RUN_PEN per missing
                    // move (the test below forgives a way blocked only by other dragons - friends that then ate and stayed)
                    const int nq = need_depth < (int)Pp.Q_RUN ? need_depth : (int)Pp.Q_RUN;
                    if (got < nq) s -= Pp.Q_RUN_PEN * (nq - got);
                }
                if (lane_commit) {
                    // (4 Oct, LANE_COMMIT) walking in I cannot turn round anyway: the lane splits get me out
                } else if (got < need_depth) {
                    int hard = got;
                    if (!others.empty()) hard = survive(g2, need_depth, body_at, gk, length, nullptr, &pearls);
                    surv_of[d] = hard;
                    if (hard < need_depth) {
                        // a real dead end: with food in it and a split to come out with, it is a harvest run
                        double pk = 0;
                        int pkf = 0;
                        if (pocket_run) {
                            int lim = hard;
                            for (int t : targets) {
                                int dt = dist[t];
                                if (0 <= dt && dt < lim &&
                                    (wp[t] || (fount && 0 < wcd[t] && wcd[t] <= dt + 1 &&
                                               (fonly == nullptr || fonly->has(wg[t]))))) {
                                    pk += 1;
                                    if (fonly != nullptr && fonly->has(wg[t])) pkf++;
                                }
                            }
                            if (T(Pp.POCKET_CROWD) && T(pk)) {
                                int nfr = 0;
                                for (auto& h : heads) {
                                    if (h.en) continue;
                                    for (int kk = h.i * 4; kk < h.i * 4 + 4; kk++) {
                                        int x = mv[kk];
                                        if (x >= 0 && 0 <= dist[x] && dist[x] < lim) {
                                            nfr++;
                                            break;
                                        }
                                    }
                                }
                                pk -= Pp.POCKET_CROWD * nfr;
                                if (pk < 0) pk = 0;
                            }
                            if (T(Pp.POCKET_MEM) && pk < Pp.FOUNT_PKMIN)
                                pk += pocket_mem(g2, lim, body_at, grow, &others, rnd);
                        }
                        if (T(tf_red) && T(Pp.TERR_FOOD_DE) && pk < Pp.FOUNT_PKMIN) s -= tf_red;
                        if (pk >= pocket_min && length + pk >= 4) {
                            s -= Pp.POCKET_RUN_COST;
                            if (fount && (fonly == nullptr || pkf) && pk >= Pp.FOUNT_PKMIN)
                                s += Pp.FOUNT_W * pk + terr_pen;
                        } else {
                            if (T(Pp.POCKET_FINISH) && pk >= 1 && pocket_run && length + pk >= 4)
                                finish.push_back({d, pk, 1000.0 * (1.0 - (double)hard / need_depth)});
                            s -= 1000.0 * (1.0 - (double)hard / need_depth);
                            if (grow && T(Pp.GOAL_BAN)) banned[(size_t)g2] = rnd + (int)Pp.GOAL_BAN;
                        }
                    } else {
                        s -= soft_w * (1.0 - (double)got / need_depth) * pocket_f;   // a jam that may clear
                    }
                }
                // --- exits that no other head can take from me
                int safe = 0;
                const bool np_safe = is_queen && q_noport(rnd) && T(Pp.Q_NOPORT_SAFE);   // (6 Oct, Q_NOPORT_SAFE)
                for (int k = v * 4; k < v * 4 + 4; k++) {
                    int x = mv[k];
                    if (x <= -2) safe += np_safe ? 0 : 1;
                    else if (x >= 0 && x != 24 && freeat[x] <= 2 && headadj[x] == 0) safe++;
                }
                if (safe == 0) s -= (is_queen && T(Pp.Q_SAFE0)) ? Pp.Q_SAFE0 : Pp.SAFE0;   // (6 Oct, Q_SAFE0)
                else if (safe == 1) s -= (is_queen && T(Pp.Q_SAFE1)) ? Pp.Q_SAFE1 : Pp.SAFE1;
                // --- never take a friend's last exit
                for (auto& fe : friend_exits) {
                    auto& ex = fe.second;
                    if (std::find(ex.begin(), ex.end(), v) != ex.end()) {
                        int n = (int)ex.size();
                        if (n == 1) s -= 60.0;
                        else if (qexit > 0 && fe.first == qhead_i) s -= qexit / (n - 1);   // (v3q) out of the queen's way
                        else if (fe.first == king_i && T(king_exit)) s -= king_exit / (n - 1);
                        else if (n == 2) s -= 0.8;
                    }
                }
                if (!room_avoid.empty()) {
                    const PTile* ph0 = ptile(hg);
                    if (ph0 != nullptr && ph0->has[d] && room_avoided(ph0->get(d, -1))) s -= Pp.ROOM_AVOID_W;   // (7 Oct, ROOM_AVOID)
                }
                if (room_guest_now) {
                    // (7 Oct, ROOM_GUEST) out of the owner's room: towards its exit, and through it
                    const int dh = room_d.get(hg);
                    if (dh != NOVAL) {
                        if (g2 >= 0 && room_set.has(g2) && !(ptile(hg) != nullptr && ptile(hg)->has[d])) {
                            const int dv = room_d.get(g2);
                            if (dv != NOVAL) s += Pp.ROOM_GUEST * (dh - dv);
                        } else {
                            s += Pp.ROOM_GUEST * (dh + 1);
                        }
                    }
                }
                if (qclear > 0 && qhead_i >= 0) {
                    // (v3q, Q_CLEAR) keep clear of the queen's head: it moves first next round and needs room
                    int dd = iabs(ROWS[v] - ROWS[qhead_i]) + iabs(COLS[v] - COLS[qhead_i]);
                    if (dd <= qclear_r) s -= qclear * (qclear_r + 1 - dd);
                }
                if (qlook > 0) {
                    // (v3q, Q_LOOK) the queen's room to run from v: the tiles in view she gets to before any enemy in view
                    int dq[49];
                    for (int i = 0; i < 49; i++) dq[i] = -1;
                    dq[v] = 0;
                    std::vector<int> qq{v};
                    int terr = 0;
                    for (size_t qi = 0; qi < qq.size(); qi++) {
                        int u = qq[qi];
                        int du = dq[u];
                        long long rq = 1 + fdiv((long long)du + qfree - 1, qfree);
                        bool safe = true;
                        for (auto& e : qed) {
                            int ed = e.de[u];
                            if (ed >= 0) {
                                long long r_e = ed <= e.re ? 1 : 1 + fdiv((long long)(ed - e.re + e.fe - 1), (long long)e.fe);
                                if (r_e <= rq) {
                                    safe = false;
                                    break;
                                }
                            }
                        }
                        if (!safe) continue;
                        terr++;
                        int nd = du + 1;
                        for (int k = u * 4; k < u * 4 + 4; k++) {
                            int x = mv[k];
                            if (x >= 0 && dq[x] < 0 && (!occ[x] || (occ[x] == 3 && freeat[x] <= nd + 1))) {
                                dq[x] = nd;
                                qq.push_back(x);
                            }
                        }
                    }
                    if (terr < qlook_n) s -= qlook * (qlook_n - terr);
                }
                if (qsolo > 0) {
                    // (v3q, Q_SOLO) the queen keeps away from friends' heads: room to move next round
                    for (auto& h : heads)
                        if (!h.en) {
                            int dd = iabs(ROWS[h.i] - ROWS[v]) + iabs(COLS[h.i] - COLS[v]);
                            if (dd <= 2) s -= qsolo * (3 - dd);
                        }
                }
                // --- an enemy that could ram me there: only matters if I am the longer one
                if (grade) {
                    if (T(tpen[v])) s -= threat_w * tpen[v];
                } else {
                    int te = thr[v];
                    if (te) {
                        int loss = length + grow - te;
                        if (loss > 0) s -= threat_w * loss;
                    }
                }
                // --- spread out (but stay with the king)
                if (!friends.empty()) {
                    int vr = ROWS[v], vc = COLS[v];
                    for (int fi : friends) {
                        int md = iabs(ROWS[fi] - vr) + iabs(COLS[fi] - vc);
                        if (fi == king_i_) {
                            if (md <= 1) {
                                s -= 0.3;               // do not stand in its way
                            } else {
                                if (ahead_r >= 0) {
                                    int ma = iabs(ahead_r - vr) + iabs(ahead_c - vc);
                                    if (ma < md) md = ma ? ma : 1;
                                }
                                s += (mg_pull ? Pp.MG_W : (qk && T(Pp.QK_GUARD_W)) ? Pp.QK_GUARD_W : Pp.GUARD_W) / md;
                            }
                            continue;
                        }
                        if (T(king_stay)) {
                            if (md <= 1) s -= 0.3;
                            else s += king_stay / md;
                            continue;
                        }
                        if (md <= 1) s -= 0.3;
                        else if (rally) {
                            if (md <= Pp.RALLY_R) s += Pp.RALLY_W / md;
                        } else if (md == 2) s -= 0.15;
                        else if (md == 3) s -= 0.05;
                        else if (md == 4) s -= 0.02;
                    }
                }
            } else {
                // the far side of a portal is out of sight and the landing tile may be taken
                const PTile* ph = ptile(hg);
                int pid = ph ? ph->get(d, -1) : -1;
                auto it = pbusy.find(pid);
                int busy = it == pbusy.end() ? -1 : it->second;
                if (busy >= rnd || ucount < 2) s -= 50.0;
                else if (pperiod && pph != ((d == 0 || d == 3) ? pnw : pse)) s -= Pp.PORTAL_PHASE_X;
                else if (kind == 1 && surv_port(g2, need_depth, length))
                    s -= 900.0;                 // what I remember of the far side is a dead end
                else if (length <= 3) s -= kind == 1 ? 0.15 : (T(P.BLIND_SHORT) ? P.BLIND_SHORT : 0.3);   // (3 Oct, BLIND_SHORT)
                else if (length <= 6) s -= 1.0;
                else s -= 6.0;
                if (room_guest_now) {
                    const int dh = room_d.get(hg);
                    if (dh != NOVAL) s += Pp.ROOM_GUEST * (dh + 1);   // (7 Oct, ROOM_GUEST) out of the owner's room
                }
                if (!room_avoid.empty() && room_avoided(pid)) s -= Pp.ROOM_AVOID_W;   // (7 Oct, ROOM_AVOID) not back in
            }
            if (owp > 0 && kind == 0) {   // (3 Oct, OWP) one-way passages
                const int v4 = v * 4;
                if (mv[v4 + ((d + 1) & 3)] == -1 && mv[v4 + ((d + 3) & 3)] == -1 && (!use_owp_pk || !owp_pk[v])) {
                    int cdir;
                    if (d == 0 || d == 2) cdir = ((g2 % W) % 2 == 0) ? 0 : 2;
                    else cdir = ((g2 / W) % 2 == 0) ? 1 : 3;
                    if (d != cdir) s -= owp;
                }
            }
            if (cy > 0 && kind == 0 && cy_any) {   // (3 Oct, CY) corridor yield
                const int v4 = v * 4, dl = (d + 1) & 3, dr = (d + 3) & 3;
                if (mv[v4 + dl] == -1 && mv[v4 + dr] == -1) {
                    int u = v;
                    for (int st_ = 0; st_ < 3; st_++) {
                        int x = mv[u * 4 + d];
                        if (x < 0) break;
                        if (occ[x]) {
                            if (occ[x] == 2 && cy_head[x] >= 0 && head_face.get(cy_head[x], -1) == ((d + 2) & 3)) s -= cy;
                            break;
                        }
                        if (mv[x * 4 + dl] != -1 || mv[x * 4 + dr] != -1) break;   // the passage ends: room to pass
                        u = x;
                    }
                }
            }
            if (d == mydir) {
                s += 0.02;
                if (is_king) s += Pp.KING_STRAIGHT;
            }
            s += food * food_w;
            if (food > food_best) food_best = food;
            if (qtrap_on && kind == 0 && v >= 0 && qreach[v]) s -= Pp.QTRAP;   // (v3.9, QTRAP) not in her way
            if (vault_a > 0 && kind == 0 && v >= 0 && wp[v] && length + 1 >= vault_a)
                s -= Pp.QVAULT_PEN;              // (v3.9, QVAULT) a pearl that would leave her no free tile
            scored.push_back({s, d, v, g2, kind});
            if (cbest_t >= 0) best_t[d] = wg[cbest_t];
        }
