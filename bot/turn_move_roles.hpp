// Sabotage-d / turn_move_roles.hpp, part 8 of the turn: what this dragon is for right now - late in the game a
// short dragon stays by a long friend and leaves it the food (guard and king), the walk to the crown (MG, the
// gathering), the king's call, and their queen: where her head was seen, and who hunts her.
// Statements of Brain::turn_(), in the order they run: included once from turn.hpp, inside the function. This part
// reads what the parts before it declared and declares what the parts after it use.

        // late game: a short dragon that sees a long friend stays near it and leaves it the food
        int king_i_ = -1;
        double gmax = Pp.GUARD_MAXLEN;
        if (T(Pp.GUARD_ALL_LATE) && rnd >= Pp.FEED_LATE) gmax = 999;
        if (kid_ >= 0 && !lead_ && (Pp.GUARD_ALL == 1 || (Pp.GUARD_ALL == 2 && (king_hungry & 2)))) gmax = 999;
        if (T(Pp.GUARD_W) && rnd >= guard_round && length <= gmax && !friends.empty() && !assn_ &&
            !(is_queen && T(Pp.Q_NOROLE))) {
            if (kid_ >= 0) {
                if (!lead_) king_i_ = king_i;
            } else {
                int kb2 = (int)Pp.GUARD_KING - 1;
                for (auto& h : heads)
                    if (!h.en && seg_n.get(h.did, 0) > kb2) {
                        kb2 = seg_n.get(h.did, 0);
                        king_i_ = h.i;
                    }
            }
        }
        // (v2.9.1) where the king is heading (FEED_AHEAD, off)
        int ahead_r = -1, ahead_c = -1;
        if (king_i_ >= 0 && T(Pp.FEED_AHEAD) && T(Pp.FEED_AHEAD_D)) {
            int kf = -1;
            for (auto& h : heads)
                if (h.i == king_i_) {
                    kf = head_face.get(h.did, -1);
                    break;
                }
            if (kf >= 0) {
                ahead_r = ROWS[king_i_] + DYS[kf] * (int)Pp.FEED_AHEAD_D;
                ahead_c = COLS[king_i_] + DXS[kf] * (int)Pp.FEED_AHEAD_D;
            }
        }
        rdv_goal = -1;                     // (8 Oct, RDV) no king known or in view, late, big map: meet at the centre
        if (T(Pp.RDV) && rnd >= Pp.RDV_ROUND && rnd < 499 && (double)W * H >= Pp.RDV_AREA && !is_queen && kid_ < 0 &&
            king_i_ < 0 && !lead_)
            rdv_goal = (H / 2) * W + W / 2;
        // a guard that has no king in view goes to the one it heard from (see _navigate)
        guard_now = T(Pp.GUARD_W) && rnd >= guard_round && length <= gmax && king_i_ < 0 && !(kid_ >= 0 && lead_) &&
                    !assn_ && !(is_queen && T(Pp.Q_NOROLE));
        if (!guard_now && kid_ >= 0 && !lead_ && king_i_ < 0 && T(Pp.LEASH_R) && rnd >= Pp.LEASH_ROUND &&
            !(is_queen && T(Pp.Q_NOROLE))) {
            int dx = iabs((int)fmod(kp->a, W) - hx), dy = iabs((int)fdiv(kp->a, W) - hy);
            if (dx > W - dx) dx = W - dx;
            if (dy > H - dy) dy = H - dy;
            if (dx + dy > Pp.LEASH_R) guard_now = true;
        }
        if (guard_now && kid_ >= 0 && T(Pp.GUARD_TIME)) {
            // (v2.9) ... unless it cannot get there before the end
            int dx = iabs((int)fmod(kp->a, W) - hx), dy = iabs((int)fdiv(kp->a, W) - hy);
            if (dx > W - dx) dx = W - dx;
            if (dy > H - dy) dy = H - dy;
            if (dx + dy > 499 - rnd) guard_now = false;
        }
        bool mg_pull = false;
        if (T(Pp.DEBUG) && mg_now && !lead_ && !is_queen && kid_ < 0) dbg_note += "mgnok ";
        if (mg_now && kid_ >= 0 && !lead_ && !is_queen) {
            // (6 Oct, MG) the gathering: to the crown - in view, its pull; out of view, its last known place
            if (king_i >= 0) {
                king_i_ = king_i;
                mg_pull = true;
                guard_now = false;
            } else {
                int dx = iabs((int)fmod(kp->a, W) - hx), dy = iabs((int)fdiv(kp->a, W) - hy);
                if (dx > W - dx) dx = W - dx;
                if (dy > H - dy) dy = H - dy;
                if (Pp.MG_SLACK < 0 || 499 - rnd <= dx + dy + length + Pp.MG_SLACK) {
                    guard_now = true;
                    mg_walk = true;
                }
            }
            if (T(Pp.DEBUG)) dbg_note += mg_pull ? "mgsee " : (mg_walk ? "mgwalk " : "mgwait ");
        }
        // pearls next to the head of a friend that shows more segments than I have are its
        int defer_i = -1;
        if ((T(Pp.FEED_DEFER) && rnd >= feed_round && !friends.empty()) ||
            (T(Pp.CAP_DEFER) && T(Pp.CAP_MARGIN) && rnd >= Pp.CAP_ROUND && ucount >= limit - Pp.CAP_MARGIN &&
             !friends.empty())) {
            if (kid_ >= 0) {
                if (!lead_) defer_i = king_i;
            } else {
                int kb2 = length;
                for (auto& h : heads)
                    if (!h.en && seg_n.get(h.did, 0) > kb2) {
                        kb2 = seg_n.get(h.did, 0);
                        defer_i = h.i;
                    }
            }
        }
        // a long dragon late in a game that is going the distance keeps to its feeders (KING_STAY_W, KING_HOME)
        double king_stay = 0.0;
        bool i_am_king = false;
        bool is_king = qk ? (rnd >= guard_round && length >= Pp.GUARD_KING && lead_)
                                    : (rnd >= guard_round && length >= Pp.GUARD_KING && length >= friend_max && (kid_ < 0 || lead_));
        double king_exit = (king_i >= 0 && !lead_ && rnd >= guard_round) ? Pp.KING_EXIT : 0.0;
        if (is_king && !friends.empty() && (king_hungry_ || !T(Pp.KING_HUNGRY) || rnd >= Pp.FEED_LATE)) {
            i_am_king = true;
            king_stay = Pp.KING_STAY_W;
        }
        if (T(Pp.BEACON) && rnd >= beacon_round_) {
            bool call;
            if (qk) call = lead_ && length >= Pp.Q_BEACON_LEN &&   // (v3q) the queen ...
                           !(T(Pp.Q_KING_AREA) && is_queen && sealed_area(hg, (int)Pp.QVAULT_MAX) > 0);   // (v3.9) not sealed in: she cannot grow
            else if (T(Pp.BEACON64)) call = length >= Pp.BEACON_LEN && lead_;
            else call = length >= Pp.BEACON_LEN && length >= friend_max && length >= king_len - 2;
            if (call) {
                long long hbits = (call_hungry ? 1 : 0) | (king_hungry_ ? 2 : 0) |
                                  ((T(Pp.ECON_DANGER) && (contact_avg >= Pp.ECON_DANGER ||
                                                          (T(Pp.DANGER_MEM) && rnd - danger_heard <= Pp.DANGER_MEM)))
                                       ? 4
                                       : 0);
                beacon_out = beacon_val(hg, length, 0, 0, hbits, id);   // I am the king as far as I know
            } else {
                int big = -1;
                int bseg = (int)Pp.HUNT_SEG - 1;
                double hunt_from = T(Pp.HUNT_ROUND) ? Pp.HUNT_ROUND : beacon_round_;
                if (T(Pp.BEACON_HUNT) && rnd >= hunt_from) {
                    for (auto& h : heads)
                        if (h.en && seg_n.get(h.did, 0) > bseg) {
                            bseg = seg_n.get(h.did, 0);
                            big = h.i;
                        }
                }
                if (big < 0 && use_ebody && !ebody.empty() && T(Pp.BEACON_HUNT) && rnd >= hunt_from) {
                    // (v2.12) no big head in view, but a long enemy body is: report its front-most segment
                    for (auto& e : ebody) {
                        int did = e.first;
                        auto& segs = e.second;
                        int n = (int)segs.size();
                        bool in_heads = false;
                        for (auto& h : heads)
                            if (h.did == did) in_heads = true;
                        if (n <= bseg || in_heads) continue;
                        bool mine_i[49] = {false};
                        for (auto& sf : segs) mine_i[sf.first] = true;
                        for (auto& sf : segs) {
                            int nx_i = mv[sf.first * 4 + sf.second];
                            if (nx_i < 0 || !mine_i[nx_i]) {
                                bseg = n;
                                big = sf.first;
                                break;
                            }
                        }
                    }
                }
                if (big >= 0 && T(Pp.ASSN)) {
                    if (!prey || bseg >= prey->c || rnd - prey->b > 3) prey = Tri{wg[big], rnd, bseg};
                }
                if (big >= 0) {
                    beacon_out = beacon_val(wg[big], bseg, 0, 1);    // an enemy king: come and get it
                } else if (relay) {
                    beacon_out = relay;                              // pass a beacon on
                } else if (kid_ >= 0 && T(Pp.BEACON_GOSSIP) && rnd - kp->b < Pp.BEACON_GOSSIP) {
                    beacon_out = beacon_val(kp->a, kp->c, rnd - kp->b + 1, 0, king_hungry > 0 ? king_hungry : 0,
                                            kid_);
                }
            }
        }
        // a short dragon that knows of an enemy king nearby goes hunting it (see _navigate)
        {
            double hunt_from = T(Pp.HUNT_ROUND) ? Pp.HUNT_ROUND : beacon_round_;
            hunt_now = assn_ || (T(Pp.BEACON_HUNT) && rnd >= hunt_from && length <= Pp.HUNT_MAXLEN && prey &&
                                 rnd - prey->b <= Pp.HUNT_MEM && enemies.empty() && !(is_queen && T(Pp.Q_NOROLE)));
        }
        bool qdn_ = false;                // (8 Oct, QDEAD_NOHUNT) their queen known dead and no queen head of theirs in view
        if (T(Pp.QDEAD) && T(Pp.QDEAD_NOHUNT) && tq_dead && T(Pp.Q_HUNT_CALL) && !is_queen) {
            qdn_ = true;
            for (auto& h : heads)
                if (h.en && (h.did == 0 || h.did == 1)) qdn_ = false;   // seen alive after all: hunt as before
        }
        if (qdn_) {
            qhunt = false;
            qprey_g = -1;
        } else if (T(Pp.Q_HUNT_CALL) && !is_queen && (!T(Pp.Q_HUNT_AREA) || (double)W * H >= Pp.Q_HUNT_AREA) &&
            (rnd >= Pp.Q_HUNT_FROM || T(Pp.Q_HUNT_LONG))) {
            // (v3q) the enemy queen's head in view: say where (any round); else pass a call on. Short dragons that know
            // where it was lately go for it
            const bool qlive = rnd >= Pp.Q_HUNT_FROM;
            int qseen = -1, qsid = -1;
            for (auto& h : heads)
                if (h.en && (h.did == 0 || h.did == 1)) {
                    qseen = h.i;
                    qsid = h.did;
                    break;
                }
            if (qseen >= 0) {
                if (T(Pp.DEBUG)) dbg_note += "qsee ";
                qprey_g = wg[qseen];
                qprey_r = rnd;
                if (qlive || Pp.QH_NEAR > 0) beacon_out = beacon_val(wg[qseen], seg_n.get(qsid, 0), 0, 1, 2);   // (9 Oct, QH_NEAR) from round 0
            } else if ((qlive || Pp.QH_NEAR > 0) && qrelay && qrelay_r == rnd && !beacon_out) {
                beacon_out = qrelay;
            } else if (qlive && T(Pp.Q_HUNT_LONG) && qprey_g >= 0 && !beacon_out && rnd - qprey_r > Pp.HUNT_MEM &&
                       rnd - qprey_r <= Pp.Q_HUNT_LONG) {
                beacon_out = beacon_val(qprey_g, 0, 7, 1, 2,      // (6 Oct, Q_HUNT_LONG) an old sighting: she keeps to her home
                                        T(Pp.QH_AGE) ? (long long)std::min(rnd - qprey_r, 65535) : 0);   // (8 Oct, QH_AGE) its true age
            }
            if (qseen < 0 && qprey_g >= 0 && T(Pp.Q_HUNT_LONG) && gi_get(qprey_g) >= 0) {
                if (T(Pp.QH_NOREINF)) {                          // (8 Oct, QH_NOREINF) remember what I found empty
                    qdrop_g = qprey_g;
                    qdrop_r = qprey_r;
                }
                qprey_g = -1;                                    // (Q_HUNT_LONG) the spot is in view and she is not there
            }
            const double qmem = T(Pp.Q_HUNT_LONG) ? Pp.Q_HUNT_LONG : Pp.HUNT_MEM;
            const bool army_ = T(Pp.ARMY) && oq_dead && rnd >= Pp.ARMY_FROM && !tq_dead &&
                               qprey_g >= 0 && rnd - qprey_r <= Pp.ARMY_SEEN;   // (8 Oct, ARMY) all in on her
            qhunt = qlive && qprey_g >= 0 && rnd - qprey_r <= qmem &&
                    (length <= Pp.Q_HUNT_MAXLEN || (army_ && (double)length <= Pp.ARMY_MAXLEN)) &&
                    ((long long)(id >> 1) % (long long)Pp.Q_HUNT_FRAC == 0 || army_) && qprey_g != hg &&
                    (army_ || !T(Pp.Q_HUNT_LEAD) || (qk && T(Pp.QK_HUNT)) ||   // (8 Oct, QK_HUNT) our queen is our king: hers decides
                     (rnd - king_len_round <= 30 && (double)king_len >= elong + Pp.Q_HUNT_LEAD_M)) &&
                    !(T(Pp.Q_HUNT_NOVAULT) && sealed_area(qprey_g, (int)Pp.QVAULT_MAX) > 0);
            if (!qhunt && Pp.QH_NEAR > 0 && qprey_g >= 0 && qprey_g != hg && (double)(rnd - qprey_r) <= Pp.QH_NEAR_AGE &&
                (double)length <= Pp.QH_NEAR_LEN) {
                // (9 Oct, QH_NEAR) she was seen close by a moment ago: go for her now, at any round
                int qdx = iabs(qprey_g % W - hx), qdy = iabs(qprey_g / W - hy);
                if (qdx > W - qdx) qdx = W - qdx;
                if (qdy > H - qdy) qdy = H - qdy;
                if ((double)(qdx + qdy) <= Pp.QH_NEAR) qhunt = true;
            }
            if (qhunt) {
                if (T(Pp.DEBUG)) dbg_note += "qhunt ";
                prey = Tri{qprey_g, qprey_r, 99};
                hunt_now = true;
            }
        }
