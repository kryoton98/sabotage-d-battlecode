// Sabotage-d / lanes.hpp: (4 Oct, LANE) the dead-end lanes of the known map (lane_build) and what is in them
// (lane_others, lane_pearl, lane_food, lane_note), and (7 Oct, ROOM_AVOID) room_avoided. The turn's side of both is
// in turn_lanes_rooms.hpp.
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    // Peel the known map: a SEEN tile with at most one way on (kelp the only other edges; a blind portal is a way on) is
    // a dead end; take it away and repeat. What is taken are trees, each hanging off the rest by one edge; a never-seen
    // tile is never taken. Depth: steps in from the tile it hangs off.
    void lane_build() {
        const int n = N;
        ln_deg.assign((size_t)n, 0);
        ln_depth.assign((size_t)n, 0);
        ln_br.assign((size_t)n, 0);
        ln_q.clear();
        for (int g = 0; g < n; g++) {
            const auto& nl = nbrs[(size_t)g];
            int dg = 0;
            for (int d = 0; d < 4; d++)
                if (nl[(size_t)d] != -1) dg++;
            ln_deg[(size_t)g] = dg;
            if (dg <= 1 && seen[(size_t)g] != UNSEEN) {
                ln_br[(size_t)g] = 1;
                ln_q.push_back(g);
            }
        }
        for (size_t qi = 0; qi < ln_q.size(); qi++) {
            const int g = ln_q[qi];
            for (int v : nbrs[(size_t)g]) {
                if (v < 0 || ln_br[(size_t)v]) continue;
                if (--ln_deg[(size_t)v] <= 1 && seen[(size_t)v] != UNSEEN) {
                    ln_br[(size_t)v] = 1;
                    ln_q.push_back(v);
                }
            }
        }
        if ((int)ln_q.size() >= n) {   // the known map is one tree: no network to measure from
            ln_br.assign((size_t)n, 0);
            return;
        }
        // depth: breadth-first from the taken tiles that touch the rest
        std::vector<int> q;
        for (int g : ln_q)
            for (int v : nbrs[(size_t)g])
                if (v >= 0 && !ln_br[(size_t)v]) {
                    ln_depth[(size_t)g] = 1;
                    q.push_back(g);
                    break;
                }
        for (size_t qi = 0; qi < q.size(); qi++) {
            const int u = q[qi];
            for (int v : nbrs[(size_t)u])
                if (v >= 0 && ln_br[(size_t)v] && ln_depth[(size_t)v] == 0) {
                    ln_depth[(size_t)v] = ln_depth[(size_t)u] + 1;
                    q.push_back(v);
                }
        }
    }

    std::string lane_note(const std::string& act, const char* why) {
        if (T(P.DEBUG)) return std::string("INDICATOR ") + why + "\n" + act;   // (before the action: the engine stops there)
        return act;
    }

    // another dragon (in view) in the lane from tile g on, deeper
    bool lane_others(int g, int lim) {
        const int i = gi_get(g);
        if (i >= 0 && ln_occ[i] && ln_occ[i] != 3) return true;
        if (lim <= 0) return false;
        const int dg = ln_depth[(size_t)g];
        for (int v : nbrs[(size_t)g])
            if (v >= 0 && ln_depth[(size_t)v] > dg && lane_others(v, lim - 1)) return true;
        return false;
    }

    // a pearl on lane tile t for a head that gets there at step k (1 = the first step of this turn)?
    bool lane_pearl(int t, int k) {
        const int i = gi_get(t);
        if (i >= 0) {
            if (ln_wp[i]) return true;
            const int c = ln_wcd[i];
            return c >= 0 && c + 1 <= k;
        }
        if (founts.has(t)) return true;
        if (!hot.has(t)) return false;
        const int h = hot.get(t, 0);
        return h <= ln_rnd + k - 1 && h >= ln_rnd - P.POCKET_MEM_AGE;
    }

    // the most pearls a head on lane tile u (reached at step k) collects on its way deeper, along one path; a tile in view
    // with a dragon on it ends the path (*busy set). k counts steps; one a round.
    int lane_food(int u, int k, bool* busy, int lim) {
        if (lim <= 0) return 0;
        int best = 0;
        const int du = ln_depth[(size_t)u];
        for (int v : nbrs[(size_t)u]) {
            if (v < 0 || ln_depth[(size_t)v] <= du) continue;
            const int i = gi_get(v);
            if ((i >= 0 && ln_occ[i]) || (i < 0 && ln_occ_seen[(size_t)v] >= ln_rnd - P.LANE_OCC_MEM)) {
                if (busy) *busy = true;
                continue;
            }
            int f = (lane_pearl(v, k + 1) ? 1 : 0) + lane_food(v, k + 1, busy, lim - 1);
            if (f > best) best = f;
        }
        return best;
    }

    bool room_avoided(int pid) const {
        for (auto& e : room_avoid)
            if (e.first == pid && e.second > cur_rnd) return true;
        return false;
    }
