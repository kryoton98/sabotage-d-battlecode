// Sabotage-d / turn_lanes_rooms.hpp, part 5 of the turn (part 4, the kill and the feeds, is in turn.hpp itself): the
// tiles other heads may step onto and every friend's real exits, never-trap (QTRAP), a newborn's first turn
// (NB_NOSPLIT), the dead-end lanes (LANE), closed rooms (ROOM_GUEST, ROOM_CAP), the queen's vault (QVAULT) and
// farm & flip (FF). Several of these return a split of their own.
// Statements of Brain::turn_(), in the order they run: included once from turn.hpp, inside the function. This part
// reads what the parts before it declared and declares what the parts after it use.

        // tiles another head could step onto before my next turn, and the real exits of every friendly head
        int headadj[49];
        for (int i = 0; i < 49; i++) headadj[i] = 0;
        std::vector<std::pair<int, std::vector<int>>> friend_exits;
        int qhead_i = -1;                 // (v3q) our queen's head in my view
        for (auto& h : heads) {
            int hi = h.i;
            if (!h.en && (h.did == 0 || h.did == 1)) qhead_i = hi;
            std::vector<int> ex;
            for (int k = hi * 4; k < hi * 4 + 4; k++) {
                int x = mv[k];
                if (x >= 0) {
                    headadj[x] += 1;
                    if (!occ[x] && !h.en) {
                        for (int kk = x * 4; kk < x * 4 + 4; kk++) {
                            int y = mv[kk];
                            if (y <= -2 || (y >= 0 && y != hi && !occ[y])) {
                                ex.push_back(x);
                                break;
                            }
                        }
                    }
                } else if (x == -2) {
                    ex.push_back(-2);
                }
            }
            if (!h.en) friend_exits.push_back({hi, ex});
        }
        bool qres_on = false;             // (8 Oct, QRES) late game: the pearls at our queen's head are hers
        if (T(Pp.QRES) && rnd >= Pp.QRES_ROUND && !is_queen && qhead_i >= 0) {
            for (auto& h : heads)
                if (h.i == qhead_i && (double)seg_n.get(h.did, 0) >= Pp.QRES_MINQ) qres_on = true;
        }

        // (v3.9, QTRAP) never-trap: the tiles our queen (in my view) can reach next turn - her free steps over free water
        const bool qtrap_on = T(Pp.QTRAP) && !is_queen && qhead_i >= 0 && !(T(Pp.QTRAP_FEED) && rnd >= feed_round) &&
                              !(T(Pp.QTRAP_CALM) && !enemies.empty());   // QTRAP_CALM: only with no enemy in my view
        uint8_t qreach[49] = {0};
        if (qtrap_on) {
            int qlen = 2;
            for (auto& h : heads)
                if (h.i == qhead_i && seg_n.get(h.did, 0) > qlen) qlen = seg_n.get(h.did, 0);
            const int qf = (qlen + 3) / 4;
            int qd[49];
            for (int i = 0; i < 49; i++) qd[i] = -1;
            qd[qhead_i] = 0;
            int qq[49];
            int qn = 0;
            qq[qn++] = qhead_i;
            for (int qi = 0; qi < qn; qi++) {
                const int u = qq[qi];
                if (qd[u] >= qf) continue;
                for (int k = u * 4; k < u * 4 + 4; k++) {
                    const int x = mv[k];
                    if (x < 0 || qd[x] >= 0 || occ[x]) continue;
                    qd[x] = qd[u] + 1;
                    qreach[x] = 1;
                    qq[qn++] = x;
                }
            }
        }

        // (4 Oct, NB_NOSPLIT) a newborn's tail touches its parent: a child of mine would face straight into it
        const bool lane_map = !(T(Pp.LANE_AREA) && (double)W * H >= Pp.LANE_AREA);   // (4 Oct, LANE_AREA)
        const bool nb_hold = T(Pp.NB_NOSPLIT) && lane_map && turns_played == 0 && rnd > 0;
        // (4 Oct, LANE) where I am in the dead-end lanes of the known map
        const bool lane_on = T(Pp.LANE) && !qs && lane_map && !(T(Pp.LANE_UNTIL) && rnd >= Pp.LANE_UNTIL);
        int lane_dir = 0;               // 1: walking deeper into a lane, -1: walking out of one, 0: neither
        int lane_ahead = -1;            // walking in: the most pearls ahead of me along one path
        bool lane_nosplit = false;      // no routine split this turn
        // (7 Oct, ROOM_GUEST) a small closed room (no way out but portals, every tile seen) with a friend of a lower id in it:
        // I am its guest - I head for the exit (the owner farms it alone) and make no growth split here
        room_guest_now = false;
        room_owner_tiles = 0;
        bool in_room = false;
        if ((T(Pp.ROOM_GUEST) || T(Pp.ROOM_CAP) || T(Pp.ROOM_HOLD)) && !pt_pool.empty() && !is_queen) {
            room_set.clear();
            room_d.clear();
            std::vector<int> rq{hg};
            room_set.add(hg);
            bool rok = true;
            std::vector<int> rex;
            for (size_t qi = 0; qi < rq.size() && rok; qi++) {
                const int u = rq[qi];
                if (seen[(size_t)u] == UNSEEN) {
                    rok = false;
                    break;
                }
                const PTile* pu = ptile(u);
                const auto& nl = nb(u);
                bool isexit = false;
                for (int d = 0; d < 4; d++) {
                    const int v = nl[(size_t)d];
                    if (v == -1) continue;
                    if (v == -3 || (pu != nullptr && pu->has[d])) {
                        isexit = true;
                        continue;
                    }
                    if (v < 0) continue;
                    if (!room_set.has(v)) {
                        room_set.add(v);
                        rq.push_back(v);
                        if ((int)rq.size() > (int)Pp.ROOM_MAX) {
                            rok = false;
                            break;
                        }
                    }
                }
                if (isexit) rex.push_back(u);
            }
            if (rok && !rex.empty() && (int)rq.size() >= (int)Pp.ROOM_MIN) {
                for (auto& h : heads)
                    if (!h.en && h.did < id && room_set.has(wg[h.i])) room_guest_now = true;
                in_room = true;
                if (!room_guest_now) room_owner_tiles = (int)rq.size();
                if (!T(Pp.ROOM_GUEST)) room_guest_now = false;   // (ROOM_CAP alone: nobody is sent out)
                if (room_guest_now) {
                    std::vector<int> dq;
                    for (int t : rex) {
                        room_d.set(t, 0);
                        dq.push_back(t);
                    }
                    for (size_t qi = 0; qi < dq.size(); qi++) {
                        const int u = dq[qi];
                        const PTile* pu = ptile(u);
                        const auto& nl = nb(u);
                        for (int d = 0; d < 4; d++) {
                            const int v = nl[(size_t)d];
                            if (v < 0 || (pu != nullptr && pu->has[d]) || !room_set.has(v) || room_d.has(v)) continue;
                            room_d.set(v, room_d.get(u) + 1);
                            dq.push_back(v);
                        }
                    }
                }
            }
            if (T(Pp.DEBUG) && room_guest_now) dbg_note += "roomguest ";
        }
        if (room_guest_now && T(Pp.ROOM_NOSPLIT)) lane_nosplit = true;
        if (in_room && T(Pp.ROOM_HOLD)) lane_nosplit = true;   // (7 Oct, ROOM_HOLD)
        if (lane_on) {
            if (ln_occ_seen.empty()) ln_occ_seen.assign((size_t)N, -1000);
            for (int i = 0; i < 49; i++) ln_occ_seen[(size_t)wg[i]] = (occ[i] == 1 || occ[i] == 2) ? rnd : -1000;
            lane_build();
            ln_wp = wp;
            ln_wcd = wcd;
            ln_occ = occ;
            ln_rnd = rnd;
            const int dh = ln_depth[(size_t)hg];
            if (dh > 0 && tn >= 2) {
                const int dn = ln_depth[(size_t)trail[1]];
                if (dn == 0 || dh > dn) lane_dir = 1;
                else if (dh < dn) lane_dir = -1;
            }
            if (lane_dir == 1) {
                bool busy = false;
                lane_ahead = lane_food(hg, 0, &busy, 60);
                lane_nosplit = true;    // walking in: the head part would go on deeper, shorter
                if (length >= 4 && ucount < limit && !nb_hold && (lane_ahead == 0 || (T(Pp.LANE_EARLY) && lane_ahead >= 2))) {
                    // nothing (or, LANE_EARLY, enough for the front two) left ahead: the rest walks out
                    std::string alt;
                    if (reverse_split(length, ucount, tn, mv, occ, alt)) return lane_note(alt, "lane-in");
                }
            } else if (tn >= length && length >= 4) {
                // my tail is in a lane, facing deeper: a child born there walks in
                const int tl = trail[(size_t)(length - 1)];
                const int tp = trail[(size_t)(length - 2)];
                const int dtl = ln_depth[(size_t)tl];
                if (dtl > 0 && dtl > ln_depth[(size_t)tp]) {
                    bool busy = false;
                    const int fb = lane_food(tl, 0, &busy, 60);
                    if (T(Pp.LANE_RELAY) && !busy && fb >= Pp.LANE_RELAY_F && ucount < limit && !nb_hold) {
                        // I stand still this turn: my head needs a way on that nobody can take first (as SPLIT_SAFE)
                        int safe = 0, free = 0;
                        for (int d = 0; d < 4; d++) {
                            const int v = mv[d + 96];
                            if (v == -2) safe++;
                            else if (v >= 0 && !occ[v]) {
                                free++;
                                if (headadj[v] == 0) safe++;
                            }
                        }
                        if (safe > 0 || free >= 2) return lane_note("SPLIT 2", "lane-relay");
                    }
                    if (T(Pp.LANE_BLOCK) && (busy || fb < 2)) lane_nosplit = true;
                }
            }
        }

        // (7 Oct, ROOM_CAP) the owner of a small closed room keeps short: the newborn of SPLIT 2 is a guest that walks out
        if (T(Pp.ROOM_CAP) && room_owner_tiles > 0 && length >= 4 && ucount < limit && tn >= length && !nb_hold) {
            int cap_len = (int)(room_owner_tiles / Pp.ROOM_CAP);
            if (cap_len < 3) cap_len = 3;
            if (length > cap_len) {
                int safe = 0, free = 0;   // I stand still this turn: my head needs a way on (as the lane relay)
                for (int d = 0; d < 4; d++) {
                    const int v = mv[d + 96];
                    if (v == -2) safe++;
                    else if (v >= 0 && !occ[v]) {
                        free++;
                        if (headadj[v] == 0) safe++;
                    }
                }
                if (safe > 0 || free >= 2) {
                    if (T(Pp.DEBUG)) return "INDICATOR roomcap\nSPLIT 2";
                    return "SPLIT 2";
                }
            }
        }

        // (v3.9, QVAULT) the queen in a sealed space: its tile count (-1: not sealed, too big or not all known). At length tiles - 1
        // (one free tile) she splits off 2 while the team has room: the child cannot get out and dies, she eats the drop
        const int vault_a = (T(Pp.QVAULT) && is_queen) ? sealed_area(hg, (int)Pp.QVAULT_MAX) : -1;
        if (vault_a > 0 && length >= 4 && length >= vault_a - 1 && ucount < limit) return "SPLIT 2";

        // (8 Oct, FF) farm & flip: at the exit of dense water, facing out, split off 2 - the newborn walks back in to farm
        bool ff_hold = false;
        ff_walk_dir = -1;
        if (T(Pp.FF) && !is_queen && !lead_ && !(T(Pp.FF_AREA) && (double)W * H >= Pp.FF_AREA) && rnd >= Pp.FF_FROM &&
            rnd < Pp.FF_UNTIL && length >= Pp.FF_MINLEN && mydir >= 0) {
            const int ed = ff_exit_dir(hg);
            bool at_exit = ed >= 0 && ed == mydir;
            if (!at_exit && tn >= 2 && !ff_dense(hg)) {
                const int pg = trail[1];
                const int ped = ff_exit_dir(pg);
                at_exit = ped >= 0 && ped == mydir && nb(pg)[(size_t)ped] == hg;   // I have just stepped out
            }
            if (at_exit && ucount < limit && tn >= length && !nb_hold && !qhthr && rnd >= ff_out_until) {   // (not again on my way out)
                const int ahead = nb(hg)[(size_t)mydir];   // my way on must be open, the newborn needs a free step
                bool ok = ahead >= 0 && !others.has(ahead) && !body_at.has(ahead);
                if (ok) {
                    const int ti = gi_get(trail[(size_t)(length - 1)]);
                    if (ti >= 0) {
                        ok = false;
                        for (int k = ti * 4; k < ti * 4 + 4; k++) {
                            const int v = mv[k];
                            if (v == -2 || (v >= 0 && !occ[v])) {
                                ok = true;
                                break;
                            }
                        }
                    }
                }
                if (ok) {
                    ff_out_until = rnd + (int)Pp.FF_OUT;
                    if (T(Pp.DEBUG)) return "INDICATOR ff-flip\nSPLIT 2";
                    return "SPLIT 2";
                }
            }
            if (!at_exit && ff_dense(hg)) {
                ff_walk_dir = ff_step(hg);              // in dense water: to the nearest exit (if there is one in reach)
                if (ff_walk_dir >= 0 && T(Pp.FF_HOLD)) ff_hold = true;
            }
        }
