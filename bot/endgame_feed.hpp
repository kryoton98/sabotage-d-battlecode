// Sabotage-d / endgame_feed.hpp: giving one dragon the length at the end - dying beside a longer friend so that it
// eats the drops (feed), the same on big maps (late_feed, LF) and the gathering at the crown (merge_feed, MG). The
// walk to the crown is in turn_move_roles.hpp; steps 1a', 1a'' and 1b of the turn (turn.hpp) call these.
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    // py: _feed - at the very end a dragon next to a much longer friend runs into its own neck and the friend
    // eats its way along the body (true + act: the move)
    bool feed(const std::vector<Head>& heads, const SmallMap& seg_n, const int* mv, const int* occ, int length,
              int rnd, int hg, const int* wg, const SmallMap* head_face, std::string& act) {
        if (trail.size() < 2) return false;
        if ((T(P.Q_KING) && is_queen) || qnr || (is_queen && T(P.Q_NOFEED))) return false;   // (v3q) the queen never feeds
        if (T(P.FEED_LONG_ROUND) && length >= P.FEED_LONG_LEN && rnd < P.FEED_LONG_ROUND) return false;
        int left = 499 - rnd;           // turns the friend still has after this one
        int drops[49];                  // window index -> trail index (-1: not a drop)
        for (int i = 0; i < 49; i++) drops[i] = -1;
        int ndrops = 0;
        for (size_t k = 0; k < trail.size(); k += 2) {
            int i = gi_get(trail[k]);
            if (i >= 0) {
                if (drops[i] < 0) ndrops++;
                drops[i] = (int)k;
            }
        }
        if (!ndrops) return false;
        bool mine[49] = {false};
        for (int g : trail) {
            int i = gi_get(g);
            if (i >= 0) mine[i] = true;
        }
        int best_gain = 0;
        double top = 0;
        double ft = P.FEED_TOP;
        if (T(ft)) {
            for (auto& h : heads)
                if (!h.en && seg_n.get(h.did, 0) > top) top = seg_n.get(h.did, 0);
            if (ft >= 2 && friend_max - 1 > top) top = friend_max - 1;
        }
        std::optional<Tri> kp = king_pos;
        int kid_ = kid;
        const bool qany = qk && T(P.QK_ANYLEN) && length < P.QK_ANYLEN_MAX;   // (8 Oct, LQ) the queen-king, whatever our lengths
        if (kid_ >= 0) {
            if (lead) return false;
            if (king_far) {
                kid_ = -1;
                kp.reset();
            }
            if (kid_ >= 0 && !qany && (kp->c < length + (length >= P.FEED_KING_MARGIN_LEN ? P.FEED_KING_MARGIN : 0) ||
                                       kp->c < P.FEED_RATIO_KING * length || rnd - kp->b > P.FEED_FRESH))
                return false;
            if (kid_ >= 0 && qany && rnd - kp->b > P.FEED_FRESH) return false;
            if (kid_ >= 0 && T(P.ECON_DANGER) && king_hungry > 0 && (king_hungry & 4) &&
                (T(P.DANGER_LATE) || rnd < P.FEED_LATE))
                return false;           // (v2.15) the king says it is in the thick of the enemy: not now
        } else if (kp && rnd - kp->b > 4) {
            kp.reset();
        }
        double floc = P.FEED_LOCAL;
        if (T(floc) && kid_ < 0 && !feed_gate) return false;
        for (auto& h : heads) {
            if (h.en) continue;
            int hi = h.i, did = h.did;
            if (qk && !(did == 0 || did == 1)) continue;     // (v3q) only the queen is fed
            int n = seg_n.get(did, 0);
            if (kid_ >= 0) {
                if ((did & 0xFFFF) != kid_) continue;
                if (T(floc)) {
                    int kr = ROWS[hi], kc = COLS[hi];
                    int nfood = 0;
                    for (int i = 0; i < 49; i++)
                        if (cur_wp[i] && iabs(ROWS[i] - kr) + iabs(COLS[i] - kc) <= 2) nfood++;
                    if (nfood >= floc) continue;
                    int myd = iabs(3 - kr) + iabs(3 - kc);
                    bool nearer = false;
                    for (auto& f : heads) {
                        if (f.en || f.i == hi) continue;
                        int fd = iabs(ROWS[f.i] - kr) + iabs(COLS[f.i] - kc);
                        if (fd < myd || (fd == myd && f.did < id)) {
                            nearer = true;
                            break;
                        }
                    }
                    if (nearer) continue;
                }
            } else if (kp) {
                // the king's beacon says where its head is: that dragon, and no local look-alike
                int gh = wg[hi];
                int dx = iabs(gh % W - (int)fmod(kp->a, W)), dy = iabs(gh / W - (int)fdiv(kp->a, W));
                if (dx > W - dx) dx = W - dx;
                if (dy > H - dy) dy = H - dy;
                if (dx + dy > 3 || (!qany && kp->c < P.FEED_RATIO * length)) continue;
            } else if (!qany && (n < P.FEED_KING_MIN || n < P.FEED_RATIO * length || n < top)) {
                continue;
            }
            // steps from its head to the nearest pearl-to-be (my body turns into open water)
            bool ahead = T(P.FEED_AHEAD) && kid_ >= 0 && head_face != nullptr && head_face->has(did);
            int kf = 0, kr = 0, kc = 0;
            double aw = 0;
            if (ahead) {
                kf = head_face->get(did, 0);
                kr = ROWS[hi];
                kc = COLS[hi];
                aw = P.FEED_AHEAD_W;
            }
            int dist[49];
            for (int i = 0; i < 49; i++) dist[i] = -1;
            dist[hi] = 0;
            std::vector<int> q{hi};
            int dmin = -1, vfirst = -1;
            for (size_t qi = 0; qi < q.size(); qi++) {
                int u = q[qi];
                int nd = dist[u] + 1;
                if (nd > P.FEED_DIST) break;
                for (int k = u * 4; k < u * 4 + 4; k++) {
                    int v = mv[k];
                    if (v < 0 || dist[v] >= 0) continue;
                    if (occ[v] && !mine[v]) continue;
                    dist[v] = nd;
                    if (drops[v] >= 0) {
                        if (ahead) {
                            int dr = ROWS[v] - kr, dc = COLS[v] - kc;
                            int fwd, lat;
                            if (kf == 0) {
                                fwd = -dr;
                                lat = dc;
                            } else if (kf == 1) {
                                fwd = dc;
                                lat = dr;
                            } else if (kf == 2) {
                                fwd = dr;
                                lat = dc;
                            } else {
                                fwd = -dc;
                                lat = dr;
                            }
                            if (fwd < 1 || (lat >= 0 ? lat : -lat) > aw) continue;
                        }
                        dmin = nd;
                        vfirst = v;
                        break;
                    }
                    q.push_back(v);
                }
                if (dmin >= 0) break;
            }
            if (dmin < 0 || dmin > left) continue;
            if (T(P.FEED_FIRST) && kid_ >= 0) {
                int vr = ROWS[vfirst], vc = COLS[vfirst];
                bool beaten = false;
                for (auto& f : heads) {
                    if (f.en || f.i == hi) continue;
                    if (iabs(ROWS[f.i] - vr) + iabs(COLS[f.i] - vc) <= dmin) {
                        beaten = true;
                        break;
                    }
                }
                if (beaten) continue;
            }
            int gain = 1 + (int)fdiv(left - dmin, 2);
            if (gain > ndrops) gain = ndrops;
            if (gain > best_gain) best_gain = gain;
        }
        if (best_gain < P.FEED_MIN_GAIN) return false;
        int neck = trail[1];
        auto& nl = nb(hg);
        for (int d = 0; d < 4; d++) {
            if (nl[(size_t)d] == neck) {
                act = std::string("MOVE ") + DCH[d];
                if (T(P.DEBUG)) act += "\nINDICATOR feed";
                return true;
            }
        }
        return false;
    }

    // (6 Oct, LF) die where a clearly longer friend in view can eat my drops (one pearl per two segments) within LF_REACH
    bool late_feed(const std::vector<Head>& heads, const SmallMap& seg_n, const int* mv, const int* occ, int length, int hg,
                   std::string& act) {
        if (trail.size() < 2) return false;
        int drops[49];
        for (int i = 0; i < 49; i++) drops[i] = -1;
        bool mine[49] = {false};
        int ndrops = 0;
        for (size_t k = 0; k < trail.size(); k++) {
            int i = gi_get(trail[k]);
            if (i < 0) continue;
            mine[i] = true;
            if (k % 2 == 0) {
                if (drops[i] < 0) ndrops++;
                drops[i] = (int)k;
            }
        }
        if (!ndrops) return false;
        const int need = (int)(P.LF_MIN > length + 2 ? P.LF_MIN : length + 2);
        const int reach = (int)P.LF_REACH;
        bool ok = false;
        bool qin = false;                 // (8 Oct, LF_QUEEN) our queen's head in view: she is the target
        if (T(P.LF_QUEEN))
            for (auto& h : heads)
                if (!h.en && (h.did == 0 || h.did == 1)) qin = true;
        if (T(P.LF_QONLY) && !qin) return false;   // (8 Oct, LF_QONLY) only into our queen
        for (auto& h : heads) {
            if (h.en) continue;
            const bool isq = h.did == 0 || h.did == 1;
            if (qin && !isq) continue;
            if (!(qin && isq) && seg_n.get(h.did, 0) < need) continue;
            int dist[49];
            for (int i = 0; i < 49; i++) dist[i] = -1;
            dist[h.i] = 0;
            std::vector<int> q{h.i};
            for (size_t qi = 0; qi < q.size() && !ok; qi++) {
                int u = q[qi];
                if (dist[u] >= reach) continue;
                for (int k = u * 4; k < u * 4 + 4; k++) {
                    int v = mv[k];
                    if (v < 0 || dist[v] >= 0) continue;
                    if (occ[v] && !mine[v]) continue;
                    dist[v] = dist[u] + 1;
                    if (drops[v] >= 0) {
                        ok = true;
                        break;
                    }
                    q.push_back(v);
                }
            }
            if (ok) break;
        }
        if (!ok) return false;
        int neck = trail[1];
        auto& nl = nb(hg);
        for (int d = 0; d < 4; d++) {
            if (nl[(size_t)d] == neck) {
                act = std::string("MOVE ") + DCH[d];
                if (T(P.DEBUG)) act += "\nINDICATOR latefeed";
                return true;
            }
        }
        return false;
    }

    // (6 Oct, MG) die where the crown's head (window index ci) reaches one of my drops (one pearl per two segments, head first)
    // within MG_REACH steps over open water and my own body, in time for it to eat there
    bool merge_feed(int ci, const int* mv, const int* occ, int hg, int rnd, std::string& act) {
        if (trail.size() < 2 || ci < 0) return false;
        int drops[49];
        for (int i = 0; i < 49; i++) drops[i] = -1;
        bool mine[49] = {false};
        int ndrops = 0;
        for (size_t k = 0; k < trail.size(); k++) {
            int i = gi_get(trail[k]);
            if (i < 0) continue;
            mine[i] = true;
            if (k % 2 == 0) {
                if (drops[i] < 0) ndrops++;
                drops[i] = (int)k;
            }
        }
        if (!ndrops) return false;
        const int reach = (int)P.MG_REACH;
        int dist[49];
        for (int i = 0; i < 49; i++) dist[i] = -1;
        dist[ci] = 0;
        std::vector<int> q{ci};
        int dmin = -1;
        for (size_t qi = 0; qi < q.size() && dmin < 0; qi++) {
            int u = q[qi];
            if (dist[u] >= reach) continue;
            for (int k = u * 4; k < u * 4 + 4; k++) {
                int v = mv[k];
                if (v < 0 || dist[v] >= 0) continue;
                if (occ[v] && !mine[v]) continue;
                dist[v] = dist[u] + 1;
                if (drops[v] >= 0) {
                    dmin = dist[v];
                    break;
                }
                q.push_back(v);
            }
        }
        if (dmin < 0 || dmin > 499 - rnd) return false;
        int neck = trail[1];
        auto& nl = nb(hg);
        for (int d = 0; d < 4; d++) {
            if (nl[(size_t)d] == neck) {
                act = std::string("MOVE ") + DCH[d];
                if (T(P.DEBUG)) act += "\nINDICATOR mergefeed";
                return true;
            }
        }
        return false;
    }
