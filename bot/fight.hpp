// Sabotage-d / fight.hpp: the kill (try_kill) - step 1 of the turn ("1. kill?" in turn.hpp). What the enemies in
// view can reach is worked out before it, in turn_threats.hpp.
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    // py: _try_kill (true + act: a move)
    bool try_kill(std::vector<Enemy> enemies, const int* mv, const int* freeat, int length, int rnd, int hg,
                  const int* occ, int nf, std::string& act) {
        // breadth-first from my head over tiles I may enter, as far as I can sprint
        int hdist[49], hpar[49];
        for (int i = 0; i < 49; i++) {
            hdist[i] = -1;
            hpar[i] = -1;
        }
        hdist[24] = 0;
        std::vector<int> q{24};
        int budget = length - 1;        // steps I can pay for in one turn
        if (T(P.SPRINT_NEW))            // (v3q) free steps first, then a segment a step
            budget = (int)fdiv((long long)length + (long long)P.SPRINT_DIV - 1, (long long)P.SPRINT_DIV) + length - 2;
        for (size_t qi = 0; qi < q.size(); qi++) {
            int u = q[qi];
            int nd = hdist[u] + 1;
            if (nd >= budget) break;
            for (int k = u * 4; k < u * 4 + 4; k++) {
                int v = mv[k];
                if (v >= 0 && hdist[v] < 0 && freeat[v] <= nd) {
                    hdist[v] = nd;
                    hpar[v] = k;
                    q.push_back(v);
                }
            }
        }
        // --- (v3q) the enemy queen within reach: ram it, whatever it costs (a dead queen counts 0 at round 500)
        if (T(P.Q_HUNT) && !is_queen) {
            for (auto& en : enemies) {
                if (!(en.did == 0 || en.did == 1)) continue;   // (2 Oct, QFIX) an enemy with id 0 / 1 is their queen
                int bd = 99, bk = -1;
                for (int d = 0; d < 4; d++) {
                    int u = mv[en.ei * 4 + d];
                    if (u < 0) continue;
                    int du = hdist[u];
                    if (du < 0 || du + 1 > budget || du + 1 >= bd) continue;
                    int k = u * 4 + OPP[d];
                    if (mv[k] == en.ei) {
                        bd = du + 1;
                        bk = k;
                    }
                }
                if (bk >= 0) {
                    std::vector<int> path{(int)fmod(bk, 4)};
                    int u = (int)fdiv(bk, 4);
                    while (u != 24) {
                        int k = hpar[u];
                        path.push_back((int)fmod(k, 4));
                        u = (int)fdiv(k, 4);
                    }
                    std::reverse(path.begin(), path.end());
                    act = emit_move(path, hg, "qram");
                    if (T(P.QDEAD) && !tq_dead) {   // (7 Oct, QDEAD) a ram into her head: she dies with me
                        tq_dead = true;
                        tq_learned = cur_rnd;
                        tq_hops = 0;
                        if (T(P.DEBUG)) dbg_note += "tqd-ram ";
                    }
                    return true;
                }
            }
        }
        // --- free kill: an enemy head with a single way out. Sprint across that tile.
        if (T(P.TRAP) && 3 <= length && length <= P.FIGHT_MAX_LEN && !qnr) {
            for (auto& en : enemies) {
                int ei = en.ei;
                int only = -1, n_ex = 0;
                for (int k = ei * 4; k < ei * 4 + 4; k++) {
                    int x = mv[k];
                    if (x <= -2) n_ex += 2;
                    else if (x >= 0 && !occ[x]) {
                        n_ex += 1;
                        only = x;
                    }
                }
                if (n_ex != 1) continue;
                int k1 = hdist[only];
                if (k1 < 1 || k1 + 1 > budget || k1 > en.ln) continue;
                for (int dd = 0; dd < 4; dd++) {
                    int y = mv[only * 4 + dd];
                    if (y < 0 || y == ei || freeat[y] > k1 + 1 || fdiv(hpar[only], 4) == y) continue;
                    bool ok = false;
                    for (int kk = y * 4; kk < y * 4 + 4; kk++) {
                        int z = mv[kk];
                        if (z <= -2 || (z >= 0 && z != only && !occ[z])) {
                            ok = true;
                            break;
                        }
                    }
                    if (!ok) continue;
                    std::vector<int> path{dd};
                    int u = only;
                    while (u != 24) {
                        int k = hpar[u];
                        path.push_back((int)fmod(k, 4));
                        u = (int)fdiv(k, 4);
                    }
                    std::reverse(path.begin(), path.end());
                    act = emit_move(path, hg, "trap");
                    return true;
                }
            }
        }
        // --- head-on ram: both die, so it has to be worth it
        if (qnr) return false;     // (v3q) never with the queen
        double margin = P.RAM_MARGIN;
        if (T(P.KING_NO_GUESS) && length >= 12 && 2 * length >= friend_max) {
            for (auto& en : enemies) en.ln = vis_n.get(en.did, en.ln);
        }
        bool even_ok = true;
        if (T(P.EVEN_NEEDS_FRIENDS) && rnd < 490) {
            if (P.EVEN_NEEDS_FRIENDS == 2) even_ok = nf >= (int)enemies.size() - 1;
            else even_ok = nf > (int)enemies.size() - 1;
        }
        // (9 Oct, CT) trades by head count: both of us die and the pearls go to the heads that are near, so what
        // matters is who is there. Heads in view within CT_R steps of mine - ours with me, and theirs
        const bool ct = T(P.CT) && rnd < 490;
        int ct_ours = 1, ct_theirs = 0;
        if (ct) {
            for (int f : ct_fr)
                if (iabs(ROWS[f] - 3) + iabs(COLS[f] - 3) <= P.CT_R) ct_ours++;
            for (auto& en : enemies)
                if (iabs(ROWS[en.ei] - 3) + iabs(COLS[en.ei] - 3) <= P.CT_R) ct_theirs++;
        }
        bool have = false;
        int best_sc = 0, best_k = -1;
        for (auto& en : enemies) {
            int ln = en.ln;
            const bool qg_hit = T(P.QG_RAM) && en.ei == qg_ei && qg_ei >= 0;   // (8 Oct, QG) the lurker at our queen: any trade
            if (ct && !qg_hit) {
                if (ct_theirs > ct_ours) continue;                // outnumbered here: start nothing
                // at its head: our other heads and theirs within CT_NEAR, and whose is the nearest (it eats first)
                int f3 = 0, e3 = 0, df = 99, de = 99;
                for (int f : ct_fr) {
                    const int dd = iabs(ROWS[f] - ROWS[en.ei]) + iabs(COLS[f] - COLS[en.ei]);
                    if (dd <= P.CT_NEAR) f3++;
                    if (dd < df) df = dd;
                }
                for (auto& o : enemies) {
                    if (o.ei == en.ei) continue;
                    const int dd = iabs(ROWS[o.ei] - ROWS[en.ei]) + iabs(COLS[o.ei] - COLS[en.ei]);
                    if (dd <= P.CT_NEAR) e3++;
                    if (dd < de) de = dd;
                }
                bool go = false;
                if (length < ln) go = f3 >= e3;                   // it is the longer: a good trade unless they collect it
                else if (length <= ln + P.CT_TOL && length < P.CT_MAXLEN)
                    go = ct_ours > ct_theirs || (f3 > e3 && df <= de) ||
                         (length <= P.CT_LUNGE && f3 >= 1 && f3 >= e3 && df <= de);
                if (!go) continue;
            } else {
            if (ln < length + margin && !qg_hit) {
                if (!(T(P.RAM_DOWN) && ln >= length - P.RAM_DOWN && length <= 4 && even_ok && rnd < P.RAM_DOWN_ROUND))
                    continue;
            }
            if (ln == length && !even_ok && !qg_hit) continue;
            }
            int bd = 99, bk = -1;
            for (int d = 0; d < 4; d++) {
                int u = mv[en.ei * 4 + d];
                if (u < 0) continue;
                int du = hdist[u];
                if (du < 0 || du + 1 > budget || du + 1 >= bd) continue;
                int k = u * 4 + OPP[d];
                if (mv[k] == en.ei) {
                    bd = du + 1;
                    bk = k;
                }
            }
            if (bk < 0) continue;
            int sc = ln * 10 - bd;
            if (!have || sc > best_sc) {
                have = true;
                best_sc = sc;
                best_k = bk;
            }
        }
        if (!have) return false;
        int k = best_k;
        std::vector<int> path{(int)fmod(k, 4)};
        int u = (int)fdiv(k, 4);
        while (u != 24) {
            k = hpar[u];
            path.push_back((int)fmod(k, 4));
            u = (int)fdiv(k, 4);
        }
        std::reverse(path.begin(), path.end());
        act = emit_move(path, hg, "ram");
        return true;
    }
