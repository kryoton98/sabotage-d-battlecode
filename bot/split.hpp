// Sabotage-d / split.hpp: splitting - when the early economy runs (early_round_, early_units_), whether to split
// at all (want_split), the child's length (split41), room for a child at the tail (tail_room), the splits that get
// a dragon out of trouble (escape_split, reverse_split) and what is left when no step is (boxed_in). Step 2 of the
// turn is in turn_split.hpp.
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    double early_round_() {   // py: _early_round
        // (v3, SEP_ROUND) the round the early economy starts from for me: EARLY_ROUND, or SEP_ROUND while the team's contact
        // estimate is below SEP_T (once started, the round it started from)
        if (early_on && T(P.SEP_ROUND)) return early_e;
        if (T(P.SEP_ROUND) && T(P.REG_R) && reg_est >= 0.0 && reg_est < P.SEP_T) return P.SEP_ROUND;
        return P.EARLY_ROUND;
    }

    double early_units_() {   // py: _early_units
        // (v3, SEP_UNITS) the team size the early economy needs: EARLY_UNITS, or SEP_UNITS while the contact estimate is below SEP_T
        if (T(P.SEP_UNITS) && T(P.REG_R) && reg_est >= 0.0 && reg_est < P.SEP_T) return P.SEP_UNITS;
        return P.EARLY_UNITS;
    }

    bool want_split(int rnd, int length, double late_round, int ucount) {   // py: _want_split
        if (qs && P.Q_SPLIT >= 0 && P.Q_SPLIT <= ucount) return false;   // (v3q) the queen keeps its length
        if (is_queen && T(P.Q_SPLIT_UNTIL) && rnd >= P.Q_SPLIT_UNTIL) return false;   // (7 Oct, QSU) nor late in the game
        if (T(P.BIGKEEP_LEN) && rnd >= P.BIGKEEP_FROM && length >= P.BIGKEEP_LEN) return false;   // (7 Oct, BIGKEEP)
        if (mg_now && T(P.MG_NOSPLIT) && !lead) return false;             // (6 Oct, MG) the gathering: no new mouths to bring home
        double stop = P.STOP_SPLIT_ROUND;
        if (reg_late && T(P.REG_STOP)) stop = P.REG_STOP;
        if (rnd >= stop && !(enough && T(P.ENOUGH_SPLIT))) return false;
        if (T(P.KING_START) && (id >> 1) < P.KING_START && rush_ok != 1) return false;
        double klen = T(P.KSPLIT_LEN) ? P.KSPLIT_LEN : P.BEACON_LEN;
        if (reg_late && T(P.REG_KLEN)) klen = P.REG_KLEN;
        if (T(P.BEACON64) && T(ksplit_round) && rnd >= ksplit_round && lead && length >= klen)
            return false;               // (v2.9) the king keeps every segment
        if (T(P.CAP_MARGIN) && ucount >= limit - P.CAP_MARGIN && length >= P.CAP_KING_RATIO * friend_max &&
            rnd >= P.CAP_ROUND)
            return false;
        int fm = friend_max;
        if (king_len > fm && rnd - king_len_round <= 20) fm = king_len;
        if (rnd >= late_round && 2 * length >= fm) return false;
        return true;
    }

    // (8 Oct, SPLIT_41) the child of a growth split of an L-long dragon as a 4k+1 length (c0: the size otherwise)
    int split41(int L, int c0) const {
        if (P.SPLIT_41 >= 2) {
            if ((double)L >= P.SPLIT_41_LONG) return L - 5 >= 2 ? 5 : c0;
            int best = -1;
            for (int c = 4 * (int)P.SPLIT_41_KMAX + 1; c >= 5; c -= 4) {
                if (L - c < 2) continue;
                if (best < 0) best = c;
                if ((L - c) % 4 == 1) return c;      // both parts 4k+1
            }
            return best >= 0 ? best : c0;
        }
        int best = -1, bd = 1 << 30;
        for (int c = 5; c <= L - 2; c += 4) {
            const int d = 2 * (c > c0 ? c - c0 : c0 - c) + (((L - c) % 4 == 1) ? 0 : 1);
            if (d < bd) {
                bd = d;
                best = c;
            }
        }
        return best >= 0 ? best : c0;
    }

    // (v2.11) py: _tail_room - how many tiles a child born at my tail could reach, through what I remember
    int tail_room(int tg, int need, const int* occ) {
        mine_set.clear();
        for (int g : trail) mine_set.add(g);
        std::vector<int> q{tg};
        room_dist[(size_t)tg] = 1;
        int n = 0;
        int ret = -1;
        for (size_t qi = 0; qi < q.size() && ret < 0; qi++) {
            int u = q[qi];
            for (int v : nbrs[(size_t)u]) {
                if (v < 0 || room_dist[(size_t)v] || mine_set.has(v)) continue;
                int i = gi_get(v);
                if (i >= 0 && occ[i]) continue;
                room_dist[(size_t)v] = 1;
                q.push_back(v);
                n++;
                if (n >= need) {
                    ret = n;
                    break;
                }
            }
        }
        for (int g : q) room_dist[(size_t)g] = 0;
        mine_set.clear();
        return ret >= 0 ? ret : n;
    }

    // py: _escape_split - hunted at the head: SPLIT (length - 2) if the tail end can get away
    bool escape_split(int length, int tn, const int* mv, const int* occ, const std::vector<Head>& heads, int rnd,
                      std::string& act) {
        if (length < 4) return false;
        if (tn >= length && T(P.ESC_ROOM)) {
            double need = P.ESC_ROOM;
            if (T(P.ESC_ROOM_LEFT) && 500 - rnd < need) need = 500 - rnd;
            if (need > 0 && tail_room(trail[(size_t)(length - 1)], (int)need, occ) < need) return false;
        }
        if (tn >= length) {
            int tg = trail[(size_t)(length - 1)];
            int ti = gi_get(tg);
            if (ti >= 0) {
                bool any = false;
                for (int k = ti * 4; k < ti * 4 + 4; k++) {
                    int v = mv[k];
                    if (v == -2 || (v >= 0 && !occ[v])) {
                        any = true;
                        break;
                    }
                }
                if (!any) return false;
                int tr = ROWS[ti], tc = COLS[ti];
                for (auto& h : heads)
                    if (h.en && iabs(ROWS[h.i] - tr) + iabs(COLS[h.i] - tc) <= 2) return false;
            } else if (T(P.ESC_TAIL)) {
                // the tail is out of view: from memory it needs ESC_TAIL neighbours that are neither kelp nor mine
                int free = 0;
                for (int v : nb(tg)) {
                    if (v >= 0 && std::find(trail.begin(), trail.end(), v) == trail.end()) free++;
                }
                if (free < P.ESC_TAIL) return false;
            }
        }
        act = "SPLIT " + std::to_string(length - 2);
        if (T(P.DEBUG)) act += "\nINDICATOR escape";
        return true;
    }

    // py: _reverse_split - stuck head first in a dead end: split off everything but two segments
    bool reverse_split(int length, int ucount, int tn, const int* mv, const int* occ, std::string& act) {
        if (length < 4 || ucount >= limit || tn < length) return false;
        int ti = gi_get(trail[(size_t)(length - 1)]);
        if (ti >= 0) {
            bool any = false;
            for (int k = ti * 4; k < ti * 4 + 4; k++) {
                int v = mv[k];
                if (v == -2 || (v >= 0 && !occ[v])) {
                    any = true;
                    break;
                }
            }
            if (!any) return false;
        }
        act = "SPLIT " + std::to_string(length - 2);
        return true;
    }

    // py: _boxed_in ("" = None)
    std::string boxed_in(const int* mv, const int* occ, const std::vector<Head>& heads, int length, int ucount,
                         int mydir, int tn) {
        int best = -1;
        for (int d = 0; d < 4; d++) {
            int v = mv[d + 96];
            if (v >= 0 && occ[v] == 2) {
                for (auto& h : heads)
                    if (h.i == v && h.en) best = d;
            }
        }
        if (best >= 0 && ucount >= 2 && length < P.BOXED_RAM_MAX && !qnr)
            return std::string("MOVE ") + DCH[best];
        double br = P.BOXED_REV;
        if (T(br) && tn < length && length >= br && ucount < limit) return "SPLIT " + std::to_string(length - 2);
        int rnd = cur_rnd;
        int run = shed_last == rnd - 1 ? shed_run : 0;
        double bra = P.BOXED_REV_AFTER;
        if (T(bra) && run >= bra && length >= P.BOXED_REV_LEN && ucount < limit && tn < length) {
            shed_run = 0;
            return "SPLIT " + std::to_string(length - 2);
        }
        if (length >= 4 && ucount < limit) {
            shed_run = run + 1;
            shed_last = rnd;
            return "SPLIT 2";           // a split is the only way to stand still
        }
        if (mydir < 0) return "";
        int order[4] = {mydir, (int)fmod(mydir + 1, 4), (int)fmod(mydir + 3, 4), (int)fmod(mydir + 2, 4)};
        for (int d : order) {
            int v = mv[d + 96];
            if (v >= 0 && occ[v] == 2) continue;   // at least do not take a friend with me
            return std::string("MOVE ") + DCH[d];
        }
        return std::string("MOVE ") + DCH[mydir];
    }
