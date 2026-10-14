// Sabotage-d / map_memory.hpp: the map as this dragon remembers it - neighbours over the wrapping grid (nb, plain_),
// portal tiles (ptile, ptile_mut), one edge into memory (edge_), a turn's edge grids into memory (record_edges),
// and a tile's index in the view (gi_get).
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    std::array<int, 4>& nb(int g) { return nbrs[(size_t)g]; }

    int plain_(int g, int d) const {
        if (d == 0) return g >= W ? g - W : g - W + N;
        if (d == 1) return (g % W) + 1 < W ? g + 1 : g + 1 - W;
        if (d == 2) return g + W < N ? g + W : g + W - N;
        return g % W ? g - 1 : g - 1 + W;
    }

    const PTile* ptile(int g) const {
        if (g < 0 || g >= N) return nullptr;
        int k = pt_idx[(size_t)g];
        return k < 0 ? nullptr : &pt_pool[(size_t)k];
    }

    PTile& ptile_mut(int g) {             // ptiles.setdefault(g, {})
        int k = pt_idx[(size_t)g];
        if (k < 0) {
            k = (int)pt_pool.size();
            pt_pool.emplace_back();
            pt_idx[(size_t)g] = k;
        }
        return pt_pool[(size_t)k];
    }

    void edge_(int kind, int g, const std::string& tok) {
        // Record a non-open edge. kind 0: north edge of g, kind 1: west edge of g.
        int key = kind * N + g;
        if (special.has(key)) return;
        route_dirty = true;                 // the way I planned may run through this edge
        if (tok == "w") {
            special.set(key, 1);
            if (sym >= 0 && !mirroring && P.SYM == 1) {
                mirroring = true;
                auto me = mir_edge(sym, kind, g);
                edge_(me.first, me.second, "w");
                mirroring = false;
            }
            if (kind == 0) {
                nb(g)[0] = -1;
                nb(plain_(g, 0))[2] = -1;
            } else {
                nb(g)[3] = -1;
                nb(plain_(g, 3))[1] = -1;
            }
            return;
        }
        int pid = (int)pyint(tok);
        special.set(key, 2 + pid);
        auto& lst = ports[pid];
        bool found = false;
        for (auto& e : lst)
            if (e.first == kind && e.second == g) found = true;
        if (!found) {
            lst.push_back({kind, g});
            portal_edges_seen++;
        }
        if (kind == 0) {
            ptile_mut(g).set(0, pid);
            ptile_mut(plain_(g, 0)).set(2, pid);
        } else {
            ptile_mut(g).set(3, pid);
            ptile_mut(plain_(g, 3)).set(1, pid);
        }
        if (lst.size() < 2) {
            // partner not seen yet: crossing is blind
            if (kind == 0) {
                nb(g)[0] = -3;
                nb(plain_(g, 0))[2] = -3;
            } else {
                nb(g)[3] = -3;
                nb(plain_(g, 3))[1] = -3;
            }
            return;
        }
        int g1 = lst[0].second;
        int g2 = lst[1].second;
        if (kind == 0) {
            // going N across the north edge of g1 lands north of g2; going S lands on g2
            nb(g1)[0] = plain_(g2, 0);
            nb(plain_(g1, 0))[2] = g2;
            nb(g2)[0] = plain_(g1, 0);
            nb(plain_(g2, 0))[2] = g1;
        } else {
            nb(g1)[3] = plain_(g2, 3);
            nb(plain_(g1, 3))[1] = g2;
            nb(g2)[3] = plain_(g1, 3);
            nb(plain_(g2, 3))[1] = g1;
        }
    }

    int gi_get(int g) const { return (g >= 0 && g < N) ? gi_arr[(size_t)g] : -1; }

    bool record_edges(const std::vector<std::string>& L, long long el0, const int* wg) {   // py: _record_edges
        auto el = [&](int r) -> const std::string& { return at(L, el0 + r); };
        bool plain = true;
        for (int r = 0; r < 8; r++) {
            const std::string& ln = el(r);
            if (ln != ALL_OPEN_H) {
                plain = false;
                auto toks = split_ws(ln);
                for (int c = 0; c < 7; c++) {
                    if (c >= (int)toks.size()) throw PyError("edge row too short");
                    const std::string& t = toks[(size_t)c];
                    if (t != ".") {
                        if (r < 7) edge_(0, wg[r * 7 + c], t);
                        else edge_(0, plain_(wg[42 + c], 2), t);
                    }
                }
            }
        }
        for (int r = 0; r < 7; r++) {
            const std::string& ln = el(8 + r);
            if (ln != ALL_OPEN_V) {
                plain = false;
                auto toks = split_ws(ln);
                for (int c = 0; c < 8; c++) {
                    if (c >= (int)toks.size()) throw PyError("edge row too short");
                    const std::string& t = toks[(size_t)c];
                    if (t != ".") {
                        if (c < 7) edge_(1, wg[r * 7 + c], t);
                        else edge_(1, plain_(wg[r * 7 + 6], 1), t);
                    }
                }
            }
        }
        return plain;
    }
