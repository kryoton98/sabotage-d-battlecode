// Sabotage-d / symmetry.hpp: (v2.8, SYM, off) every map is a mirror image of itself - the mirror of a tile and of an
// edge (mir, mir_edge, edge_class) and learning which mirror it is from what is seen (sym_learn).
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    int mir(int c, int g) const {        // py: _mir
        int x = g % W, y = g / W;
        if (c == 0) return y * W + (W - 1 - x);
        if (c == 1) return (H - 1 - y) * W + x;
        return (H - 1 - y) * W + (W - 1 - x);
    }

    std::pair<int, int> mir_edge(int c, int kind, int g) const {   // py: _mir_edge
        int x = g % W, y = g / W;
        if (kind == 0) {
            if (c == 0) return {0, y * W + (W - 1 - x)};
            if (c == 1) return {0, (int)fmod(H - y, H) * W + x};
            return {0, (int)fmod(H - y, H) * W + (W - 1 - x)};
        }
        if (c == 0) return {1, y * W + (int)fmod(W - x, W)};
        if (c == 1) return {1, (H - 1 - y) * W + x};
        return {1, (H - 1 - y) * W + (int)fmod(W - x, W)};
    }

    int edge_class(int kind, int g) const {   // py: _edge_class
        if (seen[(size_t)g] == UNSEEN && seen[(size_t)plain_(g, kind == 0 ? 0 : 3)] == UNSEEN) return -1;
        int key = kind * N + g;
        if (!special.has(key)) return 0;
        return special.get(key, 0) == 1 ? 1 : 2;
    }

    void sym_learn(int rnd, const int* wg, const int* wcd, const int* wp, const std::vector<int>& newt,
                   int horizon) {             // py: _sym_learn
        int s = sym;
        if (s < 0) {
            for (int c = 0; c < 3; c++) {
                if (!sym_ok[c]) continue;
                bool bad = false;
                for (int i = 0; i < 49; i++) {
                    int g = wg[i];
                    int m = mir(c, g);
                    if (m == g) continue;
                    int cd = wcd[i];
                    int j = gi_get(m);
                    if (j >= 0) {
                        int cm = wcd[j];
                        if ((cd < 0) != (cm < 0) || (cd >= 0 && cd != cm)) {
                            bad = true;
                            break;
                        }
                        continue;
                    }
                    int sm = seen[(size_t)m];
                    if (sm == UNSEEN) continue;
                    if (cd < 0) {
                        if (sm != BARREN_SEEN && hot.get(m, -1) > rnd) {
                            bad = true;
                            break;
                        }
                    } else if (sm == BARREN_SEEN) {
                        if (cd <= horizon) {
                            bad = true;
                            break;
                        }
                    } else {
                        if (hot.has(m)) {
                            int h = hot.get(m, 0);
                            if (h > rnd && h != rnd + cd) {
                                bad = true;
                                break;
                            }
                        }
                    }
                }
                if (!bad) {
                    for (int i : newt) {
                        int g = wg[i];
                        int ks[4][2] = {{0, g}, {1, g}, {0, plain_(g, 2)}, {1, plain_(g, 1)}};
                        for (auto& kt : ks) {
                            int a = edge_class(kt[0], kt[1]);
                            auto me = mir_edge(c, kt[0], kt[1]);
                            int b = edge_class(me.first, me.second);
                            if (b >= 0 && a != b) {
                                bad = true;
                                break;
                            }
                        }
                        if (bad) break;
                    }
                }
                if (bad) sym_ok[c] = 0;
            }
            int n = sym_ok[0] + sym_ok[1] + sym_ok[2];
            if (n == 0) {
                sym = -2;
                return;
            }
            if (n > 1) return;
            s = sym = sym_ok[0] ? 0 : (sym_ok[1] ? 1 : 2);
            sym_round = rnd;
            if (P.SYM >= 2) return;
            // what I remember so far, mirrored once
            auto hs = hot.items();
            for (auto& e : hs) {
                int m = mir(s, e.first);
                if (seen[(size_t)m] == UNSEEN && e.second > rnd) {
                    hot.set(m, e.second);
                    seen[(size_t)m] = rnd - (int)P.SYM_AGE;
                }
            }
            for (int g = 0; g < N; g++) {
                if (seen[(size_t)g] == BARREN_SEEN) {
                    int m = mir(s, g);
                    if (seen[(size_t)m] == UNSEEN) seen[(size_t)m] = BARREN_SEEN;
                }
            }
            mirroring = true;
            auto sp = special.items();
            for (auto& e : sp) {
                if (e.second == 1) {
                    int kind = e.first / N, g = e.first % N;
                    auto me = mir_edge(s, kind, g);
                    edge_(me.first, me.second, "w");
                }
            }
            mirroring = false;
        }
        // this view, mirrored
        if (P.SYM >= 2) return;
        int age = rnd - (int)P.SYM_AGE;
        bool dead = T(P.DEAD_TILES);
        for (int i = 0; i < 49; i++) {
            int g = wg[i];
            int m = mir(s, g);
            if (m == g || gi_get(m) >= 0) continue;
            int cd = wcd[i];
            if (cd < 0) {
                seen[(size_t)m] = BARREN_SEEN;
                hot.erase(m);
            } else if (cd <= horizon) {
                if (!wp[i] || hot.get(m, -1) < rnd) hot.set(m, rnd + cd);
                if (seen[(size_t)m] < age || seen[(size_t)m] == BARREN_SEEN) seen[(size_t)m] = age;
            } else if (dead) {
                seen[(size_t)m] = BARREN_SEEN;
                hot.erase(m);
            }
        }
    }
