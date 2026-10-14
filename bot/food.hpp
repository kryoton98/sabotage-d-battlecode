// Sabotage-d / food.hpp: where the pearls are - a tile's estimated spawn rate (pos_set, pos_obs), dense water
// (dense), the route planner over the pearls in view (plan_best), and the helpers of farm & flip (ff_*).
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    // (v3) a tile's estimated rate changed: keep the 5x5 sums around it up to date (py: _pos_set)
    void pos_set(int g, double rate) {
        double old = pos_rate[(size_t)g];
        if (old == rate) return;
        pos_rate[(size_t)g] = rate;
        if (rate > pos_maxr1) pos_maxr1 = rate;
        double delta;
        int dn;
        if (old < 0) {
            delta = rate;
            dn = 1;
            pos_cnt += 1;
            pos_sum += rate;
        } else {
            delta = rate - old;
            dn = 0;
            pos_sum += delta;
        }
        int x = g % W, y = g / W;
        double mx = pos_maxr5;
        for (int oy = -2; oy <= 2; oy++) {
            int yy = y + oy;
            if (yy < 0) yy += H;
            else if (yy >= H) yy -= H;
            int row = yy * W;
            for (int ox = -2; ox <= 2; ox++) {
                int xx = x + ox;
                if (xx < 0) xx += W;
                else if (xx >= W) xx -= W;
                size_t v = (size_t)(row + xx);
                pos_r5[v] += delta;
                pos_n5[v] += dn;
                if (pos_r5[v] > mx) mx = pos_r5[v];
            }
        }
        pos_maxr5 = mx;
    }

    // (v3) what one look at tile g says of its rate (py: _pos_obs)
    void pos_obs(int g, int pearl, int cd, int rnd, int horizon) {
        if (pearl) {
            if (pos_rate[(size_t)g] < 0) pos_set(g, P.POS_PEARL);
            return;
        }
        if (cd < 0 || cd > horizon) {
            pos_set(g, 0.0);
            return;
        }
        int due = rnd + cd;
        int prev = pos_due[(size_t)g];
        if (prev == due) return;
        pos_due[(size_t)g] = due;
        double nw;
        if (prev != INT_MIN && prev <= rnd && rnd - prev <= 2) nw = 1.0 / (cd + (rnd - prev) + 1);
        else nw = 1.0 / (2 * cd + 2);
        double old = pos_rate[(size_t)g];
        pos_set(g, old < 0 ? nw : 0.5 * (old + nw));
    }

    // (v2.11) py: _dense - is tile u inside a dense spawn field?
    bool dense(int u) const {
        int ux = u % W, uy = u / W;
        int need = (int)P.STOCK_N;
        int miss = 24 - need;
        int n = 0, bad = 0;
        bool opt = T(P.STOCK_OPT);
        for (auto& o : DENSE_OFF) {
            int vx = ux + o.first;
            if (vx < 0) vx += W;
            else if (vx >= W) vx -= W;
            int vy = uy + o.second;
            if (vy < 0) vy += H;
            else if (vy >= H) vy -= H;
            int v = vy * W + vx;
            if (hot.has(v) || (opt && seen[(size_t)v] == UNSEEN)) {
                n++;
                if (n >= need) return true;
            } else {
                bad++;
                if (bad > miss) return false;
            }
        }
        return n >= need;
    }

    // (v3, PLAN_W) most of the targets one route can eat within T moves (py: _plan_best)
    int plan_best(const std::vector<int>& ptg, const int* pav, const std::vector<std::array<int, 49>>& pdd,
                  const int* dist, int T_) {
        int n = (int)ptg.size();
        int best = 0;
        std::vector<int> first((size_t)n, -1);
        for (int j = 0; j < n; j++) {
            int t = ptg[(size_t)j];
            int dt_ = dist[t];
            if (dt_ < 0) continue;
            int a = dt_ + 1;
            if (a < pav[t]) a = pav[t];
            first[(size_t)j] = a <= T_ ? a : -1;
        }
        std::function<void(int, int, int, int)> rec = [&](int pos, int tm, int used, int cnt) {
            if (cnt > best) best = cnt;
            if (cnt + n - __builtin_popcount((unsigned)used) <= best) return;
            const auto& pd_ = pdd[(size_t)pos];
            for (int j = 0; j < n; j++) {
                if (used >> j & 1) continue;
                int t = ptg[(size_t)j];
                int dd = pd_[(size_t)t];
                if (dd < 0) continue;
                int a = tm + dd;
                if (a < pav[t]) a = pav[t];
                if (a > T_) continue;
                rec(j, a, used | (1 << j), cnt + 1);
            }
        };
        for (int j = 0; j < n; j++) {
            int a = first[(size_t)j];
            if (a < 0) continue;
            rec(j, a, 1 << j, 1);
            if (best == n) break;
        }
        return best;
    }

    // (8 Oct, FF) remembered pearls / fountains in the 5 x 5 block centred on u
    int ff_food(int u) const {
        const int ux = u % W, uy = u / W;
        int n = 0;
        for (int oy = -2; oy <= 2; oy++) {
            const int row = (((uy + oy) % H + H) % H) * W;
            for (int ox = -2; ox <= 2; ox++) {
                const int v = row + ((ux + ox) % W + W) % W;
                if (hot.has(v) || founts.has(v)) n++;
            }
        }
        return n;
    }

    bool ff_dense(int u) const { return (double)ff_food(u) >= P.FF_DENSE; }

    // the way out of dense water at u: a direction whose step (an open edge or a known portal) leads to water that is not
    // dense - with FF_GAP through a gap (kelp beside u across that way) or a portal. -1: u is no exit
    int ff_exit_dir(int u) {
        if (u < 0 || !ff_dense(u)) return -1;
        const auto nl = nb(u);
        const PTile* pt = ptile(u);
        for (int d = 0; d < 4; d++) {
            const int v = nl[(size_t)d];
            if (v < 0 || ff_dense(v)) continue;
            const bool port = pt != nullptr && pt->has[d];
            if (port && !T(P.FF_PORT)) continue;
            const bool narrow = nl[(size_t)((d + 1) & 3)] == -1 || nl[(size_t)((d + 3) & 3)] == -1;
            if (T(P.FF_GAP) && !narrow && !port) continue;
            return d;
        }
        return -1;
    }

    // the first step of the shortest way (remembered water; my body and the dragons in view block) to the nearest exit
    // within FF_REACH steps; -1: none (or I am on one)
    int ff_step(int hg) {
        if ((int)ff_mark.size() != N) {
            ff_mark.assign((size_t)N, 0);
            ff_first.assign((size_t)N, -1);
        }
        ff_stamp++;
        std::vector<int> q, dist;
        ff_mark[(size_t)hg] = ff_stamp;
        const auto nh = nb(hg);
        for (int d = 0; d < 4; d++) {
            const int g2 = nh[(size_t)d];
            if (g2 < 0 || ff_mark[(size_t)g2] == ff_stamp || body_at.has(g2) || others.has(g2)) continue;
            ff_mark[(size_t)g2] = ff_stamp;
            ff_first[(size_t)g2] = d;
            q.push_back(g2);
            dist.push_back(1);
        }
        for (size_t i = 0; i < q.size() && i < 400; i++) {
            const int u = q[i];
            if (ff_exit_dir(u) >= 0) return ff_first[(size_t)u];
            if ((double)dist[i] >= P.FF_REACH) continue;
            for (int v : nb(u)) {
                if (v < 0 || ff_mark[(size_t)v] == ff_stamp || body_at.has(v) || others.has(v)) continue;
                ff_mark[(size_t)v] = ff_stamp;
                ff_first[(size_t)v] = ff_first[(size_t)u];
                q.push_back(v);
                dist.push_back(dist[i] + 1);
            }
        }
        return -1;
    }
