// Sabotage-d / navigate.hpp: where to go when nothing in view is worth eating - the route over the remembered map
// (navigate), keeping to it (follow), and the direction from one tile to the next (dir_to).
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    int dir_to(int a, int b) const {     // py: _dir_to
        const auto& nl = nbrs[(size_t)a];
        if (nl[0] == b) return 0;
        if (nl[1] == b) return 1;
        if (nl[2] == b) return 2;
        if (nl[3] == b) return 3;
        return -1;
    }

    // py: _follow - the next step of the route I am on, if it still holds; -1 means plan again
    int follow(int hg, const int* legal, int rnd) {
        if (!has_route || route.empty()) return -1;
        size_t k = 0, n = route.size();
        while (k < n && route[k] == hg) k++;
        if (k) route.erase(route.begin(), route.begin() + (long)k);
        if (route.empty()) {
            has_route = false;
            route_rush = false;
            if (route_king) {
                if (!route_part) king_seen.reset();   // got there and it is not in view: the memory is stale
                route_king = false;
                route_part = false;
            }
            return -1;
        }
        if (rnd - route_round > P.ROUTE_AGE) return -1;
        // is the goal still worth the trip?
        int goal = route.back();
        int kind = route_kind;
        if (kind == 1) {
            if (!hot.has(goal) || hot.get(goal, 0) > rnd + (double)route.size() + P.ROUTE_SLACK) return -1;
        } else if (kind == 0) {
            int sg = seen[(size_t)goal];
            if (sg >= rnd - 1 || sg == BARREN_SEEN) return -1;
        }
        // I may have stepped aside for a pearl: pick the route up where it passes next to me
        int d = -1;
        n = route.size();
        for (size_t k2 = 0; k2 < (n > 3 ? 3 : n); k2++) {
            int dd = dir_to(hg, route[k2]);
            if (dd >= 0) {
                if (k2) route.erase(route.begin(), route.begin() + (long)k2);
                d = dd;
                break;
            }
        }
        if (d < 0) return -1;
        if (legal[d] != route[0]) {
            if (T(P.ROUTE_HOLD) && route_held < P.ROUTE_HOLD) {
                route_held++;
                return -3;
            }
            return -1;
        }
        route_held = 0;
        int pperiod = (int)P.PORTAL_PERIOD;
        if (pperiod) {
            const PTile* pe = ptile(hg);
            if (pe != nullptr && pe->has[d]) {
                int px = (d == 0 || d == 3) ? 1 : 1 + (int)fdiv(pperiod, 2);
                if (fmod(rnd, pperiod) != px) {
                    nav_hold_m = (int)fmod(px - 2 - rnd, pperiod) + 1;
                    nav_hold = hg;
                    return -3;
                }
            } else if (route.size() > 1) {
                int nxt = route[0];
                const PTile* pe2 = ptile(nxt);
                if (pe2 != nullptr) {
                    int d2 = dir_to(nxt, route[1]);
                    if (d2 >= 0 && pe2->has[d2]) {
                        int px = (d2 == 0 || d2 == 3) ? 1 : 1 + (int)fdiv(pperiod, 2);
                        if (fmod(rnd + 1, pperiod) != px) {
                            nav_hold_m = (int)fmod(px - 1 - rnd, pperiod);
                            nav_hold = nxt;
                            return -3;
                        }
                    }
                }
            }
        }
        if (route_dirty) {
            // an edge somewhere may have turned out to be kelp: walk the route once
            int a = hg;
            for (int b : route) {
                if (dir_to(a, b) < 0) return -1;
                a = b;
            }
            route_dirty = false;
        }
        return d;
    }

    // py: _navigate - where to go when nothing in view is worth eating (the first step; -1 nothing found)
    int navigate(int hg, std::vector<Scored>& scored, const int* occ, int rnd, bool blind_ok, int mydir) {
        int legal[4] = {-1, -1, -1, -1};        // direction -> tile of every first step I may take now
        for (auto& sc : scored)
            if (sc.s >= -20 && sc.g2 >= 0) legal[sc.d] = sc.g2;
        bool use_route = T(P.ROUTE);
        // a guard with no king in view goes back to where it last saw one
        int kgoal = -1;
        if (scout_goal >= 0 && scout_goal != hg) kgoal = scout_goal;
        else if (hunt_now && prey && prey->a != hg) kgoal = prey->a;
        else if (guard_now && king_seen && T(P.GUARD_MEM) && rnd - king_seen->b <= P.GUARD_MEM && king_seen->a != hg)
            kgoal = king_seen->a;
        else if (rdv_goal >= 0 && rdv_goal != hg)
            kgoal = rdv_goal;                    // (8 Oct, RDV) the meeting point
        if (use_route && (assn || qhunt) && has_route && !route_king) {
            has_route = false;
            route.clear();
        }
        if (use_route) {
            double credit = nav_credit + P.NAV_RATE;
            if (credit > P.NAV_MAX) credit = P.NAV_MAX;
            nav_credit = credit;
            if (route_king) {
                bool drop = kgoal < 0;
                int ref = route_part ? route_kpos : route_goal;
                if (!drop && ref != kgoal) {
                    int dx = iabs((int)fmod(kgoal, W) - (int)fmod(ref, W));
                    int dy = iabs((int)fdiv(kgoal, W) - (int)fdiv(ref, W));
                    if (dx > W - dx) dx = W - dx;
                    if (dy > H - dy) dy = H - dy;
                    drop = dx + dy > (route_part ? 6 : 3);
                }
                if (drop) {
                    has_route = false;
                    route.clear();
                    route_king = false;
                    route_part = false;
                }
            }
            int d = follow(hg, legal, rnd);
            if (d >= 0) {
                nav_nodes = 0;
                if (route_rush) nav_w = P.RUSH_W;
                return d;
            }
            if (d == -3) {
                nav_nodes = 0;
                if (nav_hold < 0) {
                    nav_hold = route[0];
                    nav_hold_m = 2;
                }
                return -3;
            }
        }
        bool mode = T(P.EXPLORE);
        int order[4];
        if (mode) {
            // Explore on a private DIAGONAL: two perpendicular headings taken in turn.
            if (pref < 0 || rnd - pref_round > P.EXPLORE_REDRAW) {
                int d1, d2;
                if (pref < 0) d1 = mydir;
                else d1 = (int)fmod((long long)(rand01() * 4), 4);
                d2 = (int)fmod(d1 + (rand01() < 0.5 ? 1 : 3), 4);
                if (pref < 0 && rnd < P.EXPLORE_CENTRE) {
                    int offx = (int)fmod(((int)fdiv(W, 2) - hg % W) + (int)fdiv(W, 2), W) - (int)fdiv(W, 2);
                    int offy = (int)fmod(((int)fdiv(H, 2) - hg / W) + (int)fdiv(H, 2), H) - (int)fdiv(H, 2);
                    int dx_ = offx ? (offx > 0 ? 1 : 3) : -1;
                    int dy_ = offy ? (offy > 0 ? 2 : 0) : -1;
                    if (dx_ >= 0 && dy_ >= 0) {
                        if (iabs(offx) >= iabs(offy)) {
                            d1 = dx_;
                            d2 = dy_;
                        } else {
                            d1 = dy_;
                            d2 = dx_;
                        }
                    } else if (dx_ >= 0) {
                        d1 = dx_;
                        d2 = (int)fmod(d1 + (rand01() < 0.5 ? 1 : 3), 4);
                    } else if (dy_ >= 0) {
                        d1 = dy_;
                        d2 = (int)fmod(d1 + (rand01() < 0.5 ? 1 : 3), 4);
                    }
                } else if (pref < 0 && rnd == 0) {
                    if (d1 == 0 || d1 == 2) {
                        int off = (int)fmod(((int)fdiv(W, 2) - hg % W) + (int)fdiv(W, 2), W) - (int)fdiv(W, 2);
                        if (off) d2 = off > 0 ? 1 : 3;
                    } else {
                        int off = (int)fmod(((int)fdiv(H, 2) - hg / W) + (int)fdiv(H, 2), H) - (int)fdiv(H, 2);
                        if (off) d2 = off > 0 ? 2 : 0;
                    }
                }
                pref = d1;
                pref2 = d2;
                pref_round = rnd;
            }
            int a = pref, b = pref2;
            if (last_nav == a) std::swap(a, b);
            order[0] = a;
            order[1] = b;
            order[2] = (int)fmod(b + 2, 4);
            order[3] = (int)fmod(a + 2, 4);
        } else {
            order[0] = mydir;
            order[1] = (int)fmod(mydir + 1, 4);
            order[2] = (int)fmod(mydir + 3, 4);
            order[3] = (int)fmod(mydir + 2, 4);
        }
        // par: tile -> parent (NOVAL: not in par)
        std::vector<int>& par = nav_par;
        auto& pkeys = nav_par_keys;
        auto par_set = [&](int g, int v) {
            if (par[(size_t)g] == NOVAL) pkeys.push_back(g);
            par[(size_t)g] = v;
        };
        par_set(hg, -1);
        std::vector<int> q;
        for (int d : order) {
            int g2 = legal[d];
            if (g2 >= 0 && par[(size_t)g2] == NOVAL) {
                par_set(g2, hg);
                q.push_back(g2);
            }
        }
        TileSet& blocked = nav_blocked;         // my own body and every dragon in view stand still
        blocked.clear();
        nav_soft.clear();
        const int body_delay = (int)P.NAV_BODY_DELAY;
        for (int g : body_at.keys) blocked.add(g);
        for (int i = 0; i < 49; i++) {
            int g = gi_wg[i];
            if (gi_arr[(size_t)g] == i && occ[i]) {
                if (body_delay > 0 && occ[i] == 1 && !body_at.has(g)) nav_soft.add(g);   // (7 Oct, NAV_BODY_DELAY)
                else blocked.add(g);   // for g, i in gi.items()
            }
        }
        for (int g : blocked.items)
            if (par[(size_t)g] == NOVAL) par_set(g, -2);
        double stale = rnd - P.REVISIT;
        double zone_w = P.ZONE_W;
        double zone_cap = P.ZONE_CAP;
        double zone_wait = P.ZONE_WAIT;
        double zone_min = P.ZONE_MIN;
        double best_cost = 1e9;
        int best_tile = -1;
        int best_kind = 0;
        long long nodes = 0;
        double cap;
        if (use_route) {
            cap = nav_credit;
            if (cap < P.NAV_MIN) cap = P.NAV_MIN;
        } else {
            cap = P.NAV_CAP;
        }
        if (T(P.KING_NAV_CAP) && (rnd >= guard_round_eff || quiet) && body_at.size() >= 12 && cap < P.KING_NAV_CAP)
            cap = P.KING_NAV_CAP;
        int rush = -1;
        if (T(P.RUSH_ROUND) && rnd < P.RUSH_ROUND && rush_ok > 0 && (T(P.RUSH_KEEP) || !seen_pearl)) {
            // no food seen yet: the middle of the map counts as a pearl until something better shows up
            rush = rush_target;
            if (T(P.RUSH_ARRIVE) && seen[(size_t)rush] != UNSEEN) {
                rush = -1;              // been there, seen nothing: back to exploring
                if (rush_ok == 1) rush_ok = 2;
            } else if (cap < P.RUSH_CAP) {
                cap = P.RUSH_CAP;
            }
        } else if (rush_ok == 1 && T(P.RUSH_ROUND) && rnd >= P.RUSH_ROUND) {
            rush_ok = 2;                // over: out of time
        }
        if (rush_ok == 2) {
            rush_ok = T(P.RUSH_BACK) ? 5 : 3;
            rush_end_round = rnd;
            rush_len0 = my_len;
        }
        if (rush_ok == 5) {
            if (my_len - rush_len0 >= P.RUSH_FED) rush_ok = 3;
            else if (rnd - rush_end_round >= P.RUSH_GRACE) {
                rush_ok = 4;
                has_route = false;
                route.clear();
                rush_back_round = rnd;
            }
        }
        if (rush_ok == 4) {
            if (rush_home == hg || rnd - rush_back_round > P.RUSH_BACK) rush_ok = 3;
            else {
                rush = rush_home;
                if (cap < P.RUSH_CAP) cap = P.RUSH_CAP;
            }
        }
        bool stale_due = T(P.STALE_DUE);
        double xb = P.EXPLORE_BARREN;
        if (rush_ok > 0 && rush < 0 && P.RUSH_XB > xb && rnd < P.RUSH_XB_ROUND) xb = P.RUSH_XB;
        double rush_wait = P.RUSH_WAIT;
        double unseen_c = P.UNSEEN_COST;
        bool fr_claim = has_nav_friends;
        double claim_m = P.FRIEND_CLAIM;
        auto& bctx = nav_bctx;
        auto& bkeys = nav_bctx_keys;
        int stick = use_route ? route_goal : -1;
        double stick_w = P.ROUTE_STICK;
        double blind_cost = P.BLIND_COST;
        if (T(P.PSCOUT_UNTIL) && rnd < P.PSCOUT_UNTIL && !is_queen &&
            (!T(P.PSC_ISLAND) || island_area(hg, (int)P.PSC_ISLAND) > 0))   // (8 Oct, PSC_ISLAND) only off a small island
            blind_cost = P.PSCOUT_COST;   // (6 Oct, PSCOUT)
        if (T(P.HB_ROUNDS) && last_eat >= 0 && rnd - last_eat >= P.HB_ROUNDS && (!is_queen || T(P.HB_QUEEN)) &&
            !(T(P.HB_UNTIL) && rnd >= P.HB_UNTIL) && P.HB_COST < blind_cost)
            blind_cost = P.HB_COST;   // (7 Oct, HB) nothing eaten for a while: what is behind a portal is worth a look
        if (T(P.LURE_MIN) && rnd <= lure_until && !is_queen && P.LURE_COST < blind_cost)
            blind_cost = P.LURE_COST;   // (7 Oct, LURE) food seen behind a wall
        if (T(P.PORTAL_NEXT) && blind_ok) {
            // an unexplored portal right next to me is a frontier like any other
            for (auto& sc : scored)
                if (sc.kind == 2 && sc.s >= -20 && 1 + blind_cost < best_cost) {
                    best_cost = 1 + blind_cost;
                    best_tile = -2 - sc.d;      // no route: the direction itself
                    best_kind = 2;
                }
        }
        int dep = 1;
        double pstep = P.PORTAL_STEP;
        int pperiod = (int)P.PORTAL_PERIOD;
        bool use_gtold = T(P.GOSSIP);
        bool use_founts = T(P.FOUNT_MEM) && !founts.empty();
        bool stock_nav = stock_now;
        const bool ff_out_nav = T(P.FF) && cur_rnd < ff_out_until;   // (8 Oct, FF)
        double cluster_w = P.CLUSTER_W;
        double cluster_cap = P.CLUSTER_CAP;
        double cluster_max = cluster_w * cluster_cap;
        const double hot_pull = P.HOT_PULL;   // (7 Oct, HOT_PULL)
        std::map<long long, std::vector<std::pair<int, int>>> later;   // depth -> [(tile, from)] through a portal
        // (v2.9) a guard whose plan does not reach the king goes to the tile it reached that is nearest to it
        bool reach = kgoal >= 0 && T(P.GUARD_REACH);
        int near_t = -1;
        int near_dep = 0;
        int kx = 0, ky = 0, near_d = 0;
        if (reach) {
            if (T(P.GUARD_NAV_CAP) && cap > P.GUARD_NAV_CAP) cap = P.GUARD_NAV_CAP;
            kx = (int)fmod(kgoal, W);
            ky = (int)fdiv(kgoal, W);
            int dx = iabs(hg % W - kx), dy = iabs(hg / W - ky);
            if (dx > W - dx) dx = W - dx;
            if (dy > H - dy) dy = H - dy;
            near_d = dx + dy;           // only tiles nearer than I am now count
        }
        std::vector<int> nq;
        double pos_maxb = 0.0, pos_w = 0.0, pos_prior = 0.0, pos_cap = P.POS_CAP;
        int pos_mode = (int)P.POS_MODE;
        int pos_open = (int)P.POS_OPEN;
        double pos_taper = P.POS_TAPER;
        bool pos_nb1 = P.POS_NB == 1;
        if (pos_learn) {
            pos_w = P.POS_W;
            pos_prior = pos_cnt ? pos_sum / pos_cnt : 0.0;
            pos_maxb = pos_nb1 ? pos_w * pos_maxr1 : pos_w * pos_maxr5;
            if (T(pos_cap) && pos_maxb > pos_cap) pos_maxb = pos_cap;
        }
        auto pos_bonus = [&](int u, int dep_) -> double {   // py: the POS_LEARN part of the unseen / stale costs
            double pb = pos_nb1 ? pos_rate[(size_t)u] - pos_prior : pos_r5[(size_t)u] - pos_n5[(size_t)u] * pos_prior;
            if (pb > 0.0 && T(pos_taper)) pb *= dep_ < pos_taper ? 1.0 - (double)dep_ / pos_taper : 0.0;
            if (pb > 0.0 && pos_open) {
                auto& nl_ = nbrs[(size_t)u];
                if ((nl_[0] != -1) + (nl_[1] != -1) + (nl_[2] != -1) + (nl_[3] != -1) < pos_open) pb = 0.0;
            }
            if (pb > 0.0) {
                pb *= pos_w;
                return (!T(pos_cap) || pb < pos_cap) ? pb : pos_cap;
            }
            return 0.0;
        };
        const double xmass = P.XMASS;   // (3 Oct, XMASS)
        const double mt_w = (T(P.MT_W) && !(T(P.MT_AREA) && (double)W * H >= P.MT_AREA)) ? P.MT_W : 0.0;   // (6 Oct, MT)
        const int mt_since = rnd - (int)P.MT_AGE;
        // (4 Oct, OE) the opening's pull towards the centre (off once the centre is seen to be barren)
        double oe_w = 0.0, oe_rad = 1.0;
        const int oe_cx = W / 2, oe_cy = H / 2;
        if (T(P.OE_ROUND) && rnd < P.OE_ROUND && P.OE_W > 0 && !(T(P.OE_AREA) && (double)W * H >= P.OE_AREA) &&
            !(T(P.OE_HALF) && ((id >> 1) & 1) == (P.OE_HALF == 1 ? 1 : 0)) &&
            !(T(P.OE_NR) && oe_nr && rnd < P.OE_NR) && !oe_off) {
            int oseen = 0, olive = 0;
            for (int dy = -4; dy <= 4; dy++)
                for (int dx = -4 + iabs(dy); dx <= 4 - iabs(dy); dx++) {
                    const int t = ((oe_cy + dy) % H + H) % H * W + ((oe_cx + dx) % W + W) % W;
                    const int sv = seen[(size_t)t];
                    if (sv == UNSEEN) continue;
                    oseen++;
                    if (sv != BARREN_SEEN) olive++;
                }
            if (!(oseen >= P.OE_BMIN && olive < P.OE_BFRAC * oseen)) {
                oe_w = P.OE_W;
                if (T(P.OE_DECAY)) oe_w *= 1.0 - (double)rnd / P.OE_ROUND;   // (6 Oct, OE_DECAY) strong at first, none by OE_ROUND
                oe_rad = P.OE_R * (W + H) / 2.0;
                if (oe_rad < 1.0) oe_rad = 1.0;
            }
        }
        auto oe_pull = [&](int u) -> double {
            int dx = iabs(u % W - oe_cx), dy = iabs(u / W - oe_cy);
            if (dx > W - dx) dx = W - dx;
            if (dy > H - dy) dy = H - dy;
            const double c = 1.0 - (dx + dy) / oe_rad;
            return c > 0.0 ? oe_w * c : 0.0;
        };
        while ((!q.empty() || !later.empty()) && nodes < cap && dep < best_cost + pos_maxb + xmass + oe_w) {
            nq.clear();
            for (size_t qi = 0; qi < q.size(); qi++) {
                int u = q[qi];
                nodes++;
                int su = seen[(size_t)u];
                int kind = 0;
                double cost;
                if (reach) {
                    int dx = iabs(u % W - kx), dy = iabs(u / W - ky);
                    if (dx > W - dx) dx = W - dx;
                    if (dy > H - dy) dy = H - dy;
                    if (dx + dy < near_d) {
                        near_d = dx + dy;
                        near_t = u;
                        near_dep = dep;
                    }
                }
                bool stale_look = false;
                if (u != rush && su < stale) {
                    // not (stale_due and h is not None and h > rnd and (gtold is None or gtold.get(u, -1) < su))
                    bool keep = stale_due && hot.has(u) && hot.get(u, 0) > rnd &&
                                (!use_gtold || gossip_told[(size_t)u] < su);
                    stale_look = !keep;
                }
                if (u == rush) {
                    cost = dep;
                    kind = 2;
                } else if (stale_look) {
                    if (rush >= 0) cost = dep + 30;           // unexplored water is not the goal while rushing
                    else if (reach) cost = 1e9;               // (v2.9) a guard on its way does not stop to look
                    else if (su == UNSEEN) {
                        cost = dep + unseen_c;
                        if (mt_w > 0.0 && mate_rnd[(size_t)u] >= mt_since) cost += mt_w;   // (MT) a mate has looked here
                        if (oe_w > 0.0) cost -= oe_pull(u);   // (4 Oct, OE)
                        if (pos_learn && pos_mode != 2) cost -= pos_bonus(u, dep);
                        if (use_gtold && hot.has(u)) {
                            int h = hot.get(u, 0);
                            if (h > rnd) {
                                int arr = h - rnd + 1;
                                if (arr < dep) arr = dep;
                                if (arr < cost) {
                                    cost = arr;
                                    kind = 1;
                                }
                            }
                        }
                        if (T(xb)) {
                            // (v2.8) unseen water next to barren tiles is probably barren too
                            int nb_ = 0, known = 0;
                            for (int v : nbrs[(size_t)u]) {
                                if (v >= 0 && seen[(size_t)v] != UNSEEN) {
                                    known++;
                                    if (seen[(size_t)v] == BARREN_SEEN) nb_++;
                                }
                            }
                            if (!known) {
                                int pu = par[(size_t)u];
                                nb_ = (pu >= 0 && bctx[(size_t)pu] != NOVAL) ? bctx[(size_t)pu] : 0;
                            }
                            if (bctx[(size_t)u] == NOVAL) bkeys.push_back(u);
                            bctx[(size_t)u] = nb_;
                            cost += xb * nb_;
                        }
                        if (T(xmass)) {   // (3 Oct, XMASS) unseen water with more unknown water around it is worth more
                            const int ux = u % W, uy = u / W;
                            int nun = 0;
                            for (int oy = -2; oy <= 2; oy++) {
                                const int row = (((uy + oy) % H + H) % H) * W;
                                for (int ox = -2; ox <= 2; ox++)
                                    if (seen[(size_t)(row + ((ux + ox) % W + W) % W)] == UNSEEN) nun++;
                            }
                            cost -= xmass * nun / 25.0;
                        }
                    } else {
                        // water I have not looked at for a while
                        int z = zone[(size_t)u];
                        cost = dep + 6 - zone_w * (z < zone_cap ? z : zone_cap);
                        if (mt_w > 0.0 && mate_rnd[(size_t)u] >= mt_since) cost += mt_w;   // (MT)
                        if (oe_w > 0.0) cost -= oe_pull(u);   // (4 Oct, OE)
                        if (pos_learn && pos_mode != 1) cost -= pos_bonus(u, dep);
                    }
                } else {
                    if (!hot.has(u)) {
                        cost = 1e9;
                    } else {
                        int h = hot.get(u, 0);
                        // (a pearl whose time I know is worth exactly the wait for it)
                        int arr = h - rnd + 1;
                        if (arr < dep) arr = dep;
                        cost = arr + (h < rnd - 25 ? 4 : 0);
                        if (T(hot_pull) && h >= rnd - (int)P.HOT_PULL_AGE && h <= rnd + (int)P.HOT_PULL_DUE)
                            cost -= hot_pull;   // (7 Oct, HOT_PULL) a pearl that is there (or due) now beats looking around
                        kind = 1;
                        if (use_founts && founts.has(u)) cost = dep;   // (v2.11) a fountain is there again
                        if (stock_nav && cost < best_cost && dense(u)) cost = 1e9;
                        if (ff_out_nav && cost < best_cost && ff_dense(u)) cost = 1e9;   // (8 Oct, FF) the flip's parent
                        if (rush >= 0 && arr > dep + rush_wait) cost = 1e9;   // no waiting on the way to the middle
                        if (T(zone_wait) && zone[(size_t)u] >= zone_min && cost > dep + zone_wait) cost = dep + zone_wait;
                        if (T(cluster_w) && cost < best_cost + cluster_max) {
                            int ux = u % W, uy = u / W;
                            int nc = 0;
                            double lim = rnd + cost + 8;
                            for (auto& o : CLUSTER_OFF) {
                                int vx = ux + o.first;
                                if (vx < 0) vx += W;
                                else if (vx >= W) vx -= W;
                                int vy = uy + o.second;
                                if (vy < 0) vy += H;
                                else if (vy >= H) vy -= H;
                                int vv = vy * W + vx;
                                if (hot.has(vv) && hot.get(vv, 0) <= lim) nc++;
                            }
                            if (nc) cost -= cluster_w * (nc < cluster_cap ? nc : cluster_cap);
                        }
                    }
                }
                if (cost < best_cost || u == stick || u == kgoal) {
                    if (fr_claim && cost < best_cost && kind != 2 && u != kgoal && !(T(P.FRIEND_CLAIM_K) && kind != 1) &&
                        !(P.FRIEND_CLAIM_K >= 2 && !(use_founts && founts.has(u)))) {   // (FRIEND_CLAIM_K: 1 pearls only, 2 fountains only)
                        // (v2.10) a pearl or a frontier that a friend in view is nearer to is that friend's
                        int ux = u % W, uy = u / W;
                        for (int f : nav_friends) {
                            int dx = iabs(f % W - ux), dy = iabs(f / W - uy);
                            if (dx > W - dx) dx = W - dx;
                            if (dy > H - dy) dy = H - dy;
                            if (dx + dy + claim_m < dep) {
                                cost = 1e9;
                                break;
                            }
                        }
                    }
                    if (u == stick) cost -= stick_w;
                    if (cost < best_cost && banned[(size_t)u] > rnd) cost = 1e9;   // been there: the way in is fatal
                    if (u == kgoal) {
                        double c2 = dep - (assn ? 1000 : P.GUARD_MEM_BONUS);
                        if (c2 < cost) {
                            cost = c2;
                            kind = 2;
                        }
                    }
                    if (cost < best_cost) {
                        best_cost = cost;
                        best_tile = u;
                        best_kind = kind;
                    }
                }
                const auto& nl = nbrs[(size_t)u];
                const PTile* pu = (T(pstep) || nav_noport) ? ptile(u) : nullptr;
                for (int d = 0; d < 4; d++) {
                    int v = nl[(size_t)d];
                    if (v < 0) {
                        if (v == -3 && blind_ok && dep + blind_cost < best_cost) {
                            best_cost = dep + blind_cost;   // an unexplored portal is a frontier too
                            best_tile = u;
                            best_kind = 2;
                        }
                        continue;
                    }
                    if (par[(size_t)v] != NOVAL) continue;
                    if (pu != nullptr && pu->has[d]) {
                        if (nav_noport) continue;   // (Q_NOPORT)
                        if (!room_avoid.empty() && room_avoided(pu->get(d, -1))) continue;   // (7 Oct, ROOM_AVOID)
                        // through a portal: a step that may land on somebody, so it counts as several
                        long long pw = 0;
                        if (pperiod) {
                            pw = fmod((long long)((d == 0 || d == 3) ? 1 : 1 + (int)fdiv(pperiod, 2)) -
                                          (long long)(rnd + dep + 1),
                                      pperiod);
                            if (pw & 1) pw += pperiod;
                        }
                        later[(long long)(dep + pstep + pw)].push_back({v, u});
                        continue;
                    }
                    if (body_delay > 0 && nav_soft.has(v)) {
                        later[(long long)(dep + body_delay)].push_back({v, u});   // (7 Oct, NAV_BODY_DELAY) it moves on
                        continue;
                    }
                    par_set(v, u);
                    nq.push_back(v);
                }
            }
            dep++;
            q.swap(nq);
            auto it = later.find(dep);
            if (it != later.end()) {
                std::vector<std::pair<int, int>> lt = std::move(it->second);
                later.erase(it);
                for (auto& e : lt) {
                    if (par[(size_t)e.first] == NOVAL && !blocked.has(e.first)) {
                        par_set(e.first, e.second);
                        q.push_back(e.first);
                    }
                }
            }
        }
        nav_nodes = nodes;
        if (use_route) {
            nav_credit -= nodes;
            if (nav_credit < 0) nav_credit = 0;
        }
        bool part = false;
        if (reach && near_t >= 0 && best_tile != kgoal) {
            // the king is beyond the plan: the nearest tile reached is a leg of the way there
            double c2 = near_dep - (assn ? 1000 : P.GUARD_MEM_BONUS);
            if (c2 < best_cost) {
                best_cost = c2;
                best_tile = near_t;
                best_kind = 2;
                part = true;
            }
        }
        if (P.DEBUG >= 3)
            fprintf(stderr, "NAV %d %d %d %d %.2f %lld %d\n", rnd, id, best_tile, best_kind, best_cost, nodes, (int)use_route);
        int result;
        if (best_tile <= -2) {
            has_route = false;
            route.clear();
            result = -2 - best_tile;            // the portal next to me
        } else {
            last_goal = best_tile >= 0 ? best_tile : -1;
            if (best_tile >= 0) {
                // the way there, for next time
                std::vector<int> rt;
                int t = best_tile;
                while (t != hg) {
                    rt.push_back(t);
                    t = par[(size_t)t];
                    if (t == NOVAL || t < 0) throw PyError("route: broken parent chain");
                }
                std::reverse(rt.begin(), rt.end());
                int d = dir_to(hg, rt[0]);
                if (use_route && (best_kind || T(P.ROUTE_EXPLORE))) {
                    has_route = true;
                    route = rt;
                    route_goal = best_tile;
                    route_kind = best_kind;
                    route_round = rnd;
                    route_held = 0;
                    route_dirty = false;
                    route_king = best_tile == kgoal || part;
                    route_part = part;
                    route_kpos = kgoal;
                    route_rush = best_tile == rush;
                }
                if (best_tile == rush) nav_w = P.RUSH_W;
                result = d;
            } else {
                result = -1;
                if (mode && T(P.EXPLORE_FAR)) {
                    // the search ran out before it found anything: keep sweeping in my private direction
                    for (int d : order)
                        if (legal[d] >= 0) {
                            result = d;
                            break;
                        }
                }
            }
        }
        for (int g : pkeys) par[(size_t)g] = NOVAL;
        pkeys.clear();
        for (int g : bkeys) bctx[(size_t)g] = NOVAL;
        bkeys.clear();
        return result;
    }
