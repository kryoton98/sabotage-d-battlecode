// Sabotage-d / sprint.hpp: moves of more than one step - the free steps after the chosen one (free_sprint), a sprint
// out of an enemy's reach (escape_sprint), and the exact check of a sprint against my own body (sprint_ok).
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    // py: _free_sprint - (v3q) the first ceil(L / 4) steps of a sprint cost nothing: after the step chosen, go on along the
    // shortest way to the pearl that drew it (in view), or along my route, over open water only, and stop on the last tile
    // that is no more threatened than the first step's and from which I can keep moving (true + path: the directions)
    bool free_sprint(const Scored& best, const int* mv, const int* freeat, const int* occ, int length, const double* tpen,
                     const int* treach, double threat_w, bool grade, int need_depth, const int* wg, const int* wp,
                     std::vector<int>& path) {
        int free = (int)fdiv((long long)length + (long long)P.SPRINT_DIV - 1, (long long)P.SPRINT_DIV);
        const bool esp = T(P.ESP) && (double)length <= P.ESP_MAXLEN && !(T(P.ESP_UNTIL) && cur_rnd >= P.ESP_UNTIL);   // (8 Oct, ESP)
        if (free < 2 && !esp) return false;
        int d1 = best.d, v1 = best.v;
        std::vector<int> ext;
        int goal = food_goal;
        int gw = goal >= 0 ? gi_get(goal) : -1;
        const bool on_route = goal < 0 && has_route && !route.empty() && route[0] == wg[v1];
        if (gw >= 0 && gw != v1) {
            int par[49];
            for (int i = 0; i < 49; i++) par[i] = -1;
            par[v1] = v1;
            std::vector<int> q{v1};
            bool found = false;
            for (size_t qi = 0; qi < q.size() && !found; qi++) {
                int u = q[qi];
                for (int k = u * 4; k < u * 4 + 4; k++) {
                    int x = mv[k];
                    if (x >= 0 && par[x] < 0 && !occ[x] && freeat[x] == 0) {
                        par[x] = u;
                        if (x == gw) {
                            found = true;
                            break;
                        }
                        q.push_back(x);
                    }
                }
            }
            if (found) {
                int u = gw;
                while (u != v1) {
                    ext.push_back(u);
                    u = par[u];
                }
                std::reverse(ext.begin(), ext.end());
            }
        } else if (on_route) {
            int u = v1;
            for (size_t ri = 1; ri < route.size() && (int)ri < free; ri++) {
                int x = gi_get(route[ri]);
                if (x < 0 || occ[x] || freeat[x] != 0) break;
                if (!(mv[u * 4] == x || mv[u * 4 + 1] == x || mv[u * 4 + 2] == x || mv[u * 4 + 3] == x)) break;
                ext.push_back(x);
                u = x;
            }
        }
        if (T(P.FS_MULTI) && gw >= 0 && (gw == v1 || !ext.empty())) {
            // (3 Oct, FS_MULTI) the pearl I went for is reached with free steps to spare: on to the nearest pearl in view
            int left = free - 1 - (int)ext.size();
            bool used[49];
            for (int i = 0; i < 49; i++) used[i] = false;
            used[v1] = true;
            for (int x : ext) used[x] = true;
            int cur = ext.empty() ? v1 : ext.back();
            while (left > 0) {
                int par[49], dep[49];
                for (int i = 0; i < 49; i++) { par[i] = -2; dep[i] = 0; }
                par[cur] = -1;
                std::vector<int> q{cur};
                int hit = -1;
                for (size_t qi = 0; qi < q.size() && hit < 0; qi++) {
                    int u = q[qi];
                    if (dep[u] >= left) continue;
                    for (int k = u * 4; k < u * 4 + 4; k++) {
                        int x = mv[k];
                        if (x >= 0 && par[x] == -2 && !used[x] && !occ[x] && freeat[x] == 0) {
                            par[x] = u;
                            dep[x] = dep[u] + 1;
                            if (wp[x]) {
                                hit = x;
                                break;
                            }
                            q.push_back(x);
                        }
                    }
                }
                if (hit < 0) break;
                std::vector<int> seg;
                for (int u = hit; u != cur; u = par[u]) seg.push_back(u);
                std::reverse(seg.begin(), seg.end());
                for (int x : seg) {
                    used[x] = true;
                    ext.push_back(x);
                }
                left -= (int)seg.size();
                cur = hit;
            }
        }
        if ((int)ext.size() > free - 1) ext.resize((size_t)(free - 1));
        if (esp) {
            // (8 Oct, ESP) past the free steps: on to an adjacent pearl while every paid step eats (length never below 2)
            bool used2[49];
            for (int i = 0; i < 49; i++) used2[i] = false;
            used2[v1] = true;
            for (int x : ext) used2[x] = true;
            int cur = ext.empty() ? v1 : ext.back();
            int eaten = wp[v1] ? 1 : 0;
            for (int x : ext)
                if (wp[x]) eaten++;
            int steps = 1 + (int)ext.size();
            while (steps < (int)P.ESP_STEPS) {
                const int paid = steps >= free ? steps - free + 1 : 0;   // paid steps after this one
                const int len_now = length + eaten - (paid > 0 ? paid - 1 : 0);
                if (paid > 0 && len_now < 3) break;                      // a paid step must leave 2 before its pearl
                int nx = -1;
                for (int k = cur * 4; k < cur * 4 + 4; k++) {
                    int x = mv[k];
                    if (x >= 0 && !used2[x] && !occ[x] && freeat[x] == 0 && wp[x] &&
                        !(paid > 0 && T(P.ESP_CONTEST) && !treach[x])) {   // (8 Oct, ESP2) a paid step only to deny a pearl
                        nx = x;
                        break;
                    }
                }
                if (nx < 0) break;
                used2[nx] = true;
                ext.push_back(nx);
                eaten++;
                steps++;
                cur = nx;
            }
        }
        if (ext.empty()) return false;
        double base_t = grade ? threat_w * tpen[v1] : 0.0;
        const int fsn = (T(P.FS_NEAR) && length >= P.FS_NEAR_LEN) ? (int)P.FS_NEAR : 0;
        int base_r = grade ? treach[v1] : 0;
        for (int n = (int)ext.size(); n >= 1; n--) {
            int fin = ext[(size_t)(n - 1)];
            const bool esp_paid = esp && 1 + n > free;                     // (8 Oct, ESP2) an eating sprint past its free steps
            if (esp_paid && T(P.ESP_RISK)) {
                if (grade && threat_w * tpen[fin] > base_t + P.ESP_RISK + 1e-9) continue;
            } else if (grade && (threat_w * tpen[fin] > base_t + 1e-9 || treach[fin] > base_r)) continue;
            if (fsn > 0 && std::max(std::abs(ROWS[fin] - 3), std::abs(COLS[fin] - 3)) > fsn) continue;   // (2 Oct, FS_NEAR)
            std::vector<int> tiles{v1};
            for (int i = 0; i < n; i++) tiles.push_back(ext[(size_t)i]);
            int eaten = 0;
            for (int x : tiles)
                if (wp[x]) eaten++;
            int sn = (int)tiles.size();
            int nl = length + eaten - (sn > free ? sn - free : 0);   // (8 Oct, ESP) paid steps cost a segment each
            std::vector<int> new_trail;
            for (int i = sn - 1; i >= 0 && (int)new_trail.size() < nl; i--) new_trail.push_back(wg[tiles[(size_t)i]]);
            int take = nl - sn;
            if (take < 0) take = 0;
            if (take > (int)trail.size()) take = (int)trail.size();
            for (int k = 0; k < take; k++) new_trail.push_back(trail[(size_t)k]);
            ba_scratch.clear();
            for (size_t k = 1; k < new_trail.size(); k++) ba_scratch.set(new_trail[k], nl - (int)k + 2);
            int need = need_depth <= nl + 4 ? need_depth : nl + 4;
            int got = survive(wg[fin], need, ba_scratch, 0, nl, &others, &pearls);
            if (got < need) continue;
            path.clear();
            path.push_back(d1);
            int u = v1;
            for (int i = 0; i < n; i++) {
                int x = ext[(size_t)i];
                for (int dd = 0; dd < 4; dd++)
                    if (mv[u * 4 + dd] == x) {
                        path.push_back(dd);
                        break;
                    }
                u = x;
            }
            if (on_route) {
                size_t k = 0;
                while (k < route.size()) {
                    bool walked = false;
                    for (int x : tiles)
                        if (wg[x] == route[k]) {
                            walked = true;
                            break;
                        }
                    if (!walked) break;
                    k++;
                }
                route.erase(route.begin(), route.begin() + (long)k);
            }
            return true;
        }
        return false;
    }

    // (v3q, Q_ESC_EXACT) a sprint along these tiles, step by step as the engine moves: a step into any segment of mine
    // (the tail too) kills; a pearl holds the tail back for that step; a step past the free ones sheds after the move
    bool sprint_ok(const std::vector<int>& steps, int length) {
        int free = 1;
        if (T(P.SPRINT_NEW)) free = (int)fdiv((long long)length + (long long)P.SPRINT_DIV - 1, (long long)P.SPRINT_DIV);
        std::deque<int> body;
        for (int k = 0; k < length && k < (int)trail.size(); k++) body.push_back(trail[(size_t)k]);
        std::vector<int> eaten;
        for (size_t j = 0; j < steps.size(); j++) {
            int g = steps[j];
            for (int b : body)
                if (b == g) return false;
            body.push_front(g);
            if (pearls.has(g) && std::find(eaten.begin(), eaten.end(), g) == eaten.end())
                eaten.push_back(g);
            else
                body.pop_back();
            if ((int)j >= free) {
                if (body.size() <= 2) return false;
                body.pop_back();
            }
        }
        return true;
    }

    // (v2.14) py: _escape_sprint (ESC_SPRINT, off)
    bool escape_sprint(const Scored& best, const int* mv, const int* freeat, int length, const double* tpen,
                       const int* treach, double threat_w, int need_depth, const int* wg, int hg, std::string& act,
                       int qfatal = -1) {
        const bool queen = qfatal >= 0;     // (v3q) the queen's escape (Q_ESC): free steps cost nothing
        int free = 1;
        double kmax, base, cost1;
        if (queen) {
            kmax = P.Q_ESC;
            if (T(P.SPRINT_NEW)) free = (int)fdiv((long long)length + (long long)P.SPRINT_DIV - 1, (long long)P.SPRINT_DIV);
            double lim = free + length - (P.Q_ESC_MIN > 2 ? P.Q_ESC_MIN : 2);   // (v3.9, Q_ESC_MIN) she keeps this many
            if (kmax > lim) kmax = lim;
            base = qfatal ? 1e9 : threat_w * tpen[best.v];
            cost1 = P.Q_ESC_COST;
        } else {
            kmax = P.ESC_SPRINT;
            if (T(P.ESC_FREE) && T(P.SPRINT_NEW)) {
                free = (int)fdiv((long long)length + (long long)P.SPRINT_DIV - 1, (long long)P.SPRINT_DIV);   // (2 Oct)
                double lim = free + length - 2;
                if (kmax > lim) kmax = lim;
            } else if (kmax > length - 1) kmax = length - 1;
            base = threat_w * tpen[best.v];
            cost1 = P.ESC_SPRINT_COST;
        }
        if (kmax < 2) return false;
        int hdist[49], hpar[49];
        for (int i = 0; i < 49; i++) {
            hdist[i] = -1;
            hpar[i] = -1;
        }
        hdist[24] = 0;
        std::vector<int> q{24};
        for (size_t qi = 0; qi < q.size(); qi++) {
            int u = q[qi];
            int nd = hdist[u] + 1;
            if (nd > kmax) break;
            for (int k = u * 4; k < u * 4 + 4; k++) {
                int v = mv[k];
                if (v >= 0 && hdist[v] < 0 && freeat[v] <= nd) {
                    hdist[v] = nd;
                    hpar[v] = k;
                    q.push_back(v);
                }
            }
        }
        std::vector<std::tuple<double, int, int>> cands;
        for (int v = 0; v < 49; v++) {
            int s = hdist[v];
            if (s < 2 || treach[v]) continue;
            int paid = s > free ? s - free : 0;
            double c = threat_w * tpen[v] + cost1 * paid;
            if (c + 1.0 < base) cands.push_back({c, s, v});
        }
        if (cands.empty()) return false;
        std::sort(cands.begin(), cands.end());
        size_t lim = cands.size() < 6 ? cands.size() : 6;
        for (size_t ci = 0; ci < lim; ci++) {
            int s = std::get<1>(cands[ci]), v = std::get<2>(cands[ci]);
            std::vector<int> path_t{v};
            int u = v;
            while (u != 24) {
                int k = hpar[u];
                u = (int)fdiv(k, 4);
                if (u != 24) path_t.push_back(u);
            }
            if (T(P.Q_ESC_EXACT)) {
                std::vector<int> steps;
                for (size_t i = path_t.size(); i-- > 0;) steps.push_back(wg[path_t[i]]);
                if (!sprint_ok(steps, length)) continue;
            }
            int nl = length - (s > free ? s - free : 0);
            std::vector<int> new_trail;
            if (queen || T(P.ESC_FREE)) {
                // (the body is the path's newest nl tiles, then what is left of the old one)
                for (size_t i = 0; i < path_t.size() && (int)i < nl; i++) new_trail.push_back(wg[path_t[i]]);
                long long take = (long long)nl - (long long)path_t.size();
                long long tsz = (long long)trail.size();
                if (take < 0) take = 0;
                if (take > tsz) take = tsz;
                for (long long k = 0; k < take; k++) new_trail.push_back(trail[(size_t)k]);
            } else {
                for (int i : path_t) new_trail.push_back(wg[i]);
                long long take = (long long)nl - (long long)path_t.size();   // trail[:nl - len(path_t)]
                long long tsz = (long long)trail.size();
                if (take < 0) take += tsz;      // (a negative slice end counts from the back)
                if (take < 0) take = 0;
                if (take > tsz) take = tsz;
                for (long long k = 0; k < take; k++) new_trail.push_back(trail[(size_t)k]);
            }
            ba_scratch.clear();
            for (size_t k = 1; k < new_trail.size(); k++) ba_scratch.set(new_trail[k], nl - (int)k + 2);
            int want = need_depth <= nl + 4 ? need_depth : nl + 4;
            int got = survive(wg[v], want, ba_scratch, 0, nl, &others, &pearls);
            if (got < want) continue;
            std::vector<int> path;
            u = v;
            while (u != 24) {
                int k = hpar[u];
                path.push_back((int)fmod(k, 4));
                u = (int)fdiv(k, 4);
            }
            std::reverse(path.begin(), path.end());
            act = emit_move(path, hg, queen ? "q-sprint" : "esc-sprint");
            return true;
        }
        return false;
    }
