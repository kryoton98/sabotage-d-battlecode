// Sabotage-d / queen.hpp: the queen (dragon 0 or 1; her length decides round 500). Here: the rounds she keeps off
// portals (q_noport, q_noblind) and the sealed space around a tile (sealed_area, QVAULT). Most of her rules are
// conditions inside the turn, tagged by name where they stand:
//   turn_dragons.hpp       QDEAD, ARMY
//   turn_threats.hpp       Q_TAIL, Q_MEM, QG
//   turn_lanes_rooms.hpp   QTRAP, QVAULT
//   turn_move_roles.hpp    Q_HUNT_CALL, QDEAD_NOHUNT
//   turn_move_weights.hpp  Q_LOOK, Q_JAM, Q_CLEAR
//   turn_move_score.hpp    Q_HOME, Q_FRESH, Q_TUNNEL, Q_RUN, Q_CLEAR, Q_LOOK, Q_SOLO, QTRAP, QVAULT
//   turn_move_choose.hpp   QG, Q_HARD, QSAVE, Q_ESC, Q_END_STILL
// Her parameters are the Q* names in params.hpp. In the code `is_queen` is "I am the queen" and `qs` is "and her
// safety options apply this turn" (Q_AREA / Q_LATE).
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    bool q_noport(int rnd) const {   // (6 Oct, Q_NOPORT; 8 Oct, Q_NOPORT_UNITS) the queen keeps off portals now
        return T(P.Q_NOPORT) && rnd >= P.Q_NOPORT_FROM && !(T(P.Q_NOPORT_UNITS) && (double)ucount_now < P.Q_NOPORT_UNITS);
    }

    // (7 Oct) Q_NOBLIND applies this round (Q_NOBLIND_FROM / Q_NOBLIND_AREA)
    bool q_noblind(int rnd) const {
        return T(P.Q_NOBLIND) && !(T(P.Q_NOBLIND_FROM) && rnd < P.Q_NOBLIND_FROM) &&
               !(T(P.Q_NOBLIND_AREA) && (double)W * H >= P.Q_NOBLIND_AREA);
    }

    // (v3.9, QVAULT) the tiles of the space around g that kelp closes off, if it has at most cap tiles and all of it is known
    // (an edge I have not seen is open water; a portal whose far side I have not seen leads out): else -1
    int sealed_area(int g, int cap) {
        if (g < 0 || cap < 1 || cap > 60) return -1;
        int q[64];
        int n = 0;
        q[n++] = g;
        for (int i = 0; i < n; i++) {
            const auto& a = nb(q[i]);
            for (int d = 0; d < 4; d++) {
                const int t = a[(size_t)d];
                if (t == -1) continue;          // kelp
                if (t < 0) return -1;           // a portal I cannot see through
                bool have = false;
                for (int j = 0; j < n; j++)
                    if (q[j] == t) {
                        have = true;
                        break;
                    }
                if (have) continue;
                if (n >= cap) return -1;
                q[n++] = t;
            }
        }
        return n;
    }
