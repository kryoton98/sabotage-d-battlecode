// Sabotage-d / search.hpp: the searches over the remembered map - how long I can keep moving (survive, survive_,
// surv_port), how much room there is (room), food remembered in a dead end (pocket_mem), the island a tile is on
// (island_area), and the dead-end pockets of the view (owp_pockets).
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    // py: _survive - longest run of legal moves found from `start` (= step 1), capped at `need`.  Depth-first on the
    // remembered map; my own body frees up as it moves on, one step later for every pearl eaten on the way; `others`
    // are permanent walls when given.  Cut short -> count the room (AREA_CHECK).
    // (the wrapper only logs: DEBUG 3 prints every call, which is how the one bug the parity check found was pinned down)
    int survive(int start, int need, const IntMap& bat, int grow, int length, const TileSet* oth, const TileSet* prl) {
        long long work0 = work;
        int r = survive_(start, need, bat, grow, length, oth, prl);
        if(&bat == &body_at && prl == &pearls) {
            auto exact=exact_body_survival(start,need,grow,length,oth);
            if(exact.complete) {body_refined|=r!=exact.depth;r=exact.depth;}
        }
        if (P.DEBUG >= 3)
            fprintf(stderr, "SURV start=%d need=%d grow=%d len=%d others=%d pearls=%d work=%lld -> %d\n", start, need, grow,
                    length, oth ? (int)oth->items.size() : -1, prl ? (int)prl->items.size() : -1, work0, r);
        return r;
    }

    int survive_(int start, int need, const IntMap& bat, int grow, int length, const TileSet* oth, const TileSet* prl) {
        struct Lv {
            int tile, it, prev, grown;
        };
        static std::vector<Lv> path;
        path.clear();
        std::vector<int>& used = surv_used;   // tile -> step (0: None)
        path.push_back({start, 0, 0, grow});
        used[(size_t)start] = 1;
        int best = 1;
        long long nodes = 0;
        long long cap = work;
        if (cap > P.SURV_NODES) cap = (int)P.SURV_NODES;   // (3 Oct, SURV_NODES; was 220)
        auto unwind = [&]() {
            for (size_t k = path.size(); k-- > 0;) used[(size_t)path[k].tile] = path[k].prev;
            path.clear();
        };
        while (!path.empty()) {
            int d = (int)path.size();
            if (d >= need) {
                work -= nodes;
                unwind();
                return need;
            }
            nodes++;
            if (nodes > cap) {
                work -= nodes;
                unwind();
                if (T(P.AREA_CHECK)) return room(start, need, bat, grow, (oth && !oth->empty()) ? oth : nullptr, best);
                return need;
            }
            int nd = d + 1;
            Lv& top = path.back();
            int gr = top.grown;
            int life = length + gr;
            const auto& nl = nbrs[(size_t)top.tile];
            bool pushed = false;
            while (top.it < 4) {
                int v = nl[(size_t)top.it];
                top.it++;
                if (v < 0) continue;
                if (surv_seen && seen[(size_t)v] == UNSEEN) continue;   // (v3q, Q_SURV) never seen: a wall for the queen
                int pu = used[(size_t)v];
                if (pu != 0 && nd - pu <= life) continue;
                int t = bat.get(v);
                if (t != NOVAL && t + gr > nd) continue;
                if (oth != nullptr && oth->has(v)) continue;
                int ng = (prl != nullptr && prl->has(v)) ? gr + 1 : gr;
                path.push_back({v, 0, pu, ng});
                used[(size_t)v] = nd;
                if (nd > best) best = nd;
                pushed = true;
                break;
            }
            if (!pushed) {
                Lv lv = path.back();
                path.pop_back();
                used[(size_t)lv.tile] = lv.prev;
            }
        }
        work -= nodes;
        return best;
    }

    // a known portal's far side (landing g2) is a dead end as far as I remember it (Q_SURV_PORT: for the queen unseen water is a wall)
    bool surv_port(int g2, int need_depth, int length) {
        const bool ss0 = surv_seen;
        if (is_queen && T(P.Q_SURV_PORT)) surv_seen = true;
        const bool dead = survive(g2, need_depth, body_at, 0, length, nullptr, &pearls) < need_depth;
        surv_seen = ss0;
        return dead;
    }

    // py: _room - breadth-first count of the tiles I could reach from `start`
    int room(int start, int need, const IntMap& bat, int grow, const TileSet* oth, int best) {
        std::vector<int> q{start};
        room_dist[(size_t)start] = 1;
        int ret = -1;
        for (size_t qi = 0; qi < q.size() && ret < 0; qi++) {
            int u = q[qi];
            int nd = room_dist[(size_t)u] + 1;
            for (int v : nbrs[(size_t)u]) {
                if (v < 0 || room_dist[(size_t)v]) continue;
                if (surv_seen && seen[(size_t)v] == UNSEEN) continue;
                int t = bat.get(v);
                if (t != NOVAL && t + grow > nd) continue;
                if (oth != nullptr && oth->has(v)) continue;
                room_dist[(size_t)v] = nd;
                q.push_back(v);
                if ((int)q.size() >= need) {
                    ret = need;
                    break;
                }
            }
        }
        for (int g : q) room_dist[(size_t)g] = 0;
        if (ret >= 0) return ret;
        int n = (int)q.size();
        return n > best ? n : best;
    }

    // (v2.12) py: _pocket_mem - food I remember in a dead end beyond my view
    int pocket_mem(int start, int lim, const IntMap& bat, int grow, const TileSet* oth, int rnd) {
        double old = rnd - P.POCKET_MEM_AGE;
        std::vector<int> q{start};
        room_dist[(size_t)start] = 1;
        int n = 0;
        for (size_t qi = 0; qi < q.size(); qi++) {
            int u = q[qi];
            int du = room_dist[(size_t)u];
            if (du >= lim) continue;
            int nd = du + 1;
            for (int v : nbrs[(size_t)u]) {
                if (v < 0 || room_dist[(size_t)v]) continue;
                int t = bat.get(v);
                if (t != NOVAL && t + grow > nd) continue;
                if (oth != nullptr && oth->has(v)) continue;
                room_dist[(size_t)v] = nd;
                q.push_back(v);
                if (gi_get(v) < 0 && hot.has(v)) {
                    int h = hot.get(v, 0);
                    if (h <= rnd + nd && (h >= old || founts.has(v))) n++;
                }
            }
        }
        for (int g : q) room_dist[(size_t)g] = 0;
        return n;
    }

    // (8 Oct, PSC_ISLAND) the island g is on: tiles reachable without a portal (kelp and every portal edge close it), if all of
    // it has been seen and it has at most cap tiles; else -1 (a dragon there can only get out through a portal)
    int island_area(int g, int cap) {
        if (g < 0 || cap < 1 || cap > 400) return -1;
        std::vector<int> q{g};
        isl_mark.assign((size_t)N, 0);
        isl_mark[(size_t)g] = 1;
        for (size_t i = 0; i < q.size(); i++) {
            const int u = q[i];
            const auto& a = nb(u);
            for (int d = 0; d < 4; d++) {
                const int t = a[(size_t)d];
                if (t < 0) continue;                       // kelp or a portal I cannot see through
                if (t != plain_(u, d)) continue;           // a known portal: an exit, not the island
                if (isl_mark[(size_t)t]) continue;
                if (seen[(size_t)t] == UNSEEN) return -1;  // not all of it seen yet
                if ((int)q.size() >= cap) return -1;
                isl_mark[(size_t)t] = 1;
                q.push_back(t);
            }
        }
        return (int)q.size();
    }

    void owp_pockets(const int* mv, int* pk) {   // (3 Oct, OWP_NOPOCKET) dead-end pockets in view, peeled as POCKET_PEEL
        int deg[49];
        for (int i = 0; i < 49; i++) {
            int n = 0;
            for (int k = i * 4; k < i * 4 + 4; k++) {
                int x = mv[k];
                if (x >= 0) n += 1;
                else if (x <= -2) n += 2;
            }
            deg[i] = n;
            pk[i] = 0;
        }
        std::vector<int> stack;
        for (int i = 0; i < 49; i++)
            if (deg[i] <= 1) stack.push_back(i);
        while (!stack.empty()) {
            int i = stack.back();
            stack.pop_back();
            if (pk[i]) continue;
            pk[i] = 1;
            for (int k = i * 4; k < i * 4 + 4; k++) {
                int x = mv[k];
                if (x >= 0 && !pk[x]) {
                    deg[x] -= 1;
                    if (deg[x] <= 1) stack.push_back(x);
                }
            }
        }
    }
