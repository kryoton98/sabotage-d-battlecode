// Sabotage-d / brain.hpp: struct Brain - everything one dragon knows and does. One process is one dragon, so this is
// its whole memory: the data members and the constructor are here, the member functions are in the files included at
// the end. A new data member goes with the others, above the constructor (their order is the order they are built).
// A piece of main.cpp: included once, inside its namespace. Not a header to include from anywhere else.

// ------------------------------------------------------------------------------------------------ the brain
struct Brain {
    int id;
    std::string team;
    int tbit;
    int W, H, N;
    int limit;
    Params P;
    // --- what I remember of the map
    std::vector<int> seen;
    OrderedMap hot;                       // tile -> round at which a pearl is / will be there
    std::vector<std::array<int, 4>> nbrs;  // tile, -1 kelp, -3 blind portal (built for every tile at once)
    OrderedMap special;                   // (kind, tile) = kind * N + tile -> 1 kelp / 2 + portal id
    std::map<int, std::vector<std::pair<int, int>>> ports;   // portal id -> [(kind, tile)]
    std::vector<int> pt_idx;              // tile -> index into pt_pool (-1: none)
    std::vector<PTile> pt_pool;
    std::map<int, int> pbusy;             // portal id -> round until which somebody is threading through
    // --- my own body
    std::vector<int> trail;
    bool has_plan = false;
    std::vector<int> plan;
    long long work = 0;
    std::map<int, std::pair<int, int>> elen;
    SmallMap vis_n;
    int friend_max = 0;
    int last_enemy = -1000;
    bool early_on = false;
    double early_e = 0;                   // (v3, SEP_ROUND) the round my early economy started from
    int cur_rnd = 0;
    int shed_run = 0;
    double contact_avg = 0.0;
    std::vector<int> reg_hist;            // (v3, REG_R) rounds on which my view showed reachable contact (last REG_WIN)
    int reg_born = -1;                    // ... my first round
    double reg_est = -1.0;                // ... my estimate of the team's contact rate (-1: none yet)
    bool reg_on = false;                  // ... the late clock is on for me
    uint64_t reg_out = 0;                 // ... the value to send this turn
    long long reg_kelp = 0;               // ... kelp edges seen in my window, summed over my life (REG_OPEN)
    long long reg_edges = 0;              // ... edges looked at
    double emax = 0;                      // (v3, ENOUGH_M) the longest enemy I know of ...
    int emax_round = -1;                  // ... and when it was seen (-1: never)
    bool enough = false;                  // ... our king is far enough ahead of it: no feeding, no guarding (this turn)
    bool reg_late = false;                // (v3) the late clock of REG is on for me this turn
    bool mg_now = false;                  // (6 Oct, MG) the gathering is on this turn
    bool mg_walk = false;                 // (6 Oct, MG) ... and I walk to the crown
    int danger_heard = -1000;
    int shed_last = -10;
    bool seen_pearl = false;
    int rush_ok = -1;
    int oe_nr = 0;                        // (6 Oct, OE_NR) 1: I was a starter at round 0 and was not picked to rush
    int oe_near = 0;                      // (7 Oct, OE_STUCK) rounds in a row near the centre without eating
    bool oe_off = false;                  //   the pull is off for me
    int lure_until = -1;                  // (7 Oct, LURE) unexplored portals are cheap until this round
    bool rush_rich = false;
    int rush_home = -1;
    int rush_back_round = 0;
    int rush_end_round = 0;
    int rush_len0 = 0;
    std::vector<int> zone;                // tile -> appearances (zone.get(g, 0))
    std::vector<int> pseen;               // tile -> last round seen holding a pearl (pseen.get(g, -9))
    std::vector<int> mate_rnd;            // (MT) tile -> last round a friendly dragon's head was seen within MT_R of it
    std::vector<std::pair<int, int>> gossip_recent;
    int gossip_i = 0;
    uint64_t gossip_out = 0;              // 0: None
    std::vector<int> gossip_told;         // gossip_told.get(g, -1)
    bool quiet = false;
    int friend_max_round = -999;
    int pref = -1;
    int pref2 = -1;
    int pref_round = 0;
    int last_nav = -1;
    int my_len = 0;
    int spr_r = -10, spr_k = 0, spr_l0 = 0, spr_e = 0;   // (SPRINT_LEARN) my last sprint (spr_r -10: none)
    std::vector<int> spr_pearls;                         // (SPRINT_LEARN) pearls in view this turn
    int my_hg = -1;
    bool guard_now = false;
    int rdv_goal = -1;                    // (8 Oct, RDV) the map's centre while I know of no king late on a big map
    // --- the route I am following
    bool has_route = false;               // route is not None
    std::vector<int> route;
    int route_goal = -1;
    int route_kind = 0;
    bool route_rush = false;
    bool has_nav_friends = false;
    int qg_ei = -1;                       // (8 Oct, QG) this turn: the enemy head (window index) nearest our queen's head
    std::vector<int> nav_friends;
    double nav_w = 0.5;
    bool nav_noport = false;   // (Q_NOPORT) this navigate call does not cross portals
    int ucount_now = 0;        // (8 Oct, Q_NOPORT_UNITS) our team's dragon count this turn
    std::vector<int> ct_fr;    // (9 Oct, CT) this turn: the window indices of the friendly heads in view
    int nav_hold = -1;
    bool stock_now = false;
    TileSet founts;
    bool cheated = false;
    int rush_target = 0;
    int nav_hold_m = 0;
    int route_held = 0;
    int route_round = -999;
    bool route_dirty = false;
    double nav_credit = 0;
    long long nav_nodes = 0;
    std::optional<Tri> king_seen;
    int sym = -1;
    int sym_ok[3] = {1, 1, 1};
    int sym_round = -1;
    // (v3) POS_LEARN: spawn rates estimated from the countdowns seen, and their 5x5 sums
    bool pos_learn = false;
    int ora_state = 0;                    // (8 Oct, ORA) 0 checking, 1 recognised (ora_map), -1 not a known map (for good)
    int ora_map = -1;
    int ora_info = 0;                     //   kelp / portal edges matched so far (each counted once)
    bool ora_rates_on = false;            //   POS rates are the map's true ones
    bool ora_pos0 = false;                //   pos_learn before the map was recognised
    std::vector<int> ora_cand;
    std::vector<std::vector<uint8_t>> ora_kinds, ora_his;
    std::vector<uint8_t> ora_counted;
    std::optional<Params> ora_P0;         //   my parameters before the map's own (restored on a mismatch)
    std::vector<double> pos_rate;         // -1: never seen
    std::vector<int> pos_due;             // the due round of the draw a tile's estimate last took in (INT_MIN: none)
    std::vector<double> pos_r5;
    std::vector<int> pos_n5;
    double pos_sum = 0.0;
    int pos_cnt = 0;
    double pos_maxr5 = 0.0;
    double pos_maxr1 = 0.0;
    bool pos_mirrored = false;
    bool mirroring = false;
    bool route_king = false;
    bool route_part = false;
    int route_kpos = -1;
    int last_goal = -1;
    int last_eat = -1;
    int last_pearl_seen = 0;              // (8 Oct, QHHS) the last round a pearl was in my view
    std::vector<int> stuck_hist;          // (3 Oct, STUCK_N) my last head tiles
    int stuck_until = -1;
    int home_x = -1, home_y = -1;         // (3 Oct, Q_HOME) where I first stood
    bool q_units_on = false, q_units_rehome = false;   // (v3.9, Q_SAFE_UNITS)
    int portal_edges_seen = 0;           // (7 Oct, Q_PMAP) distinct portal edges I have seen
    bool reg_units_on = false;            // (v3.9, REG_UNITS) we were dominant: the late clock is off for good
    int chase_id = -1, chase_from = -1000;   // (v3.9, CHASE) the enemy I am closing on, since when
    std::map<int, int> chase_cool;        // (v3.9, CHASE) enemy id -> the round until which I leave it alone
    std::vector<int> viewed;              // (3 Oct, Q_FRESH) tile -> last round it was in my view (the queen only)
    int eat2 = -1;
    int prev_eat = -1;
    int food_goal = -1;
    uint64_t beacon_out = 0;              // 0: None
    int qid = 0;                          // (v3q) our queen's id (0 for team A, 1 for B); the enemy's is 1 - qid
    int eqid = 1;
    bool is_queen = false;
    double beacon_round = 0;
    double ksplit_round = 0;
    double guard_round_eff = 0;
    int king_len = 0;
    int king_len_round = -999;
    double elong = 0;                     // (Q_HUNT_LEAD) the longest enemy seen lately (own view or an enemy-king beacon)
    int elong_r = -999;
    int king_hungry = -1;
    std::optional<Tri> king_pos;
    int king_id = -1;
    int king_i = -1;
    bool lead = false;
    int kid = -1;
    bool king_far = false;
    std::optional<Tri> prey;
    bool hunt_now = false;
    int qprey_g = -1, qprey_r = 0;       // (v3q) where and when the enemy queen's head was last seen (-1: never)
    int qdrop_g = -1, qdrop_r = -1;      // (8 Oct, QH_NOREINF) the sighting I found empty: its spot and its round
    uint64_t qrelay = 0;                 // (v3q) a queen call to pass on (heard in round qrelay_r)
    int qrelay_r = -1;
    bool qhunt = false;                  // (v3q) I am answering a queen call
    std::string dbg_note;                // (6 Oct) DEBUG: words for an INDICATOR line this turn
    bool surv_seen = false;              // (v3q, Q_SURV) this turn's dead-end searches count unseen water as walls
    bool qs = false;                     // (v3q) the queen's safety options apply this turn (Q_AREA / Q_LATE)
    bool qnr = false;                    // (v3q) the queen trades itself with nobody this turn (Q_SAFE where qs, or Q_NORAM_ALL)
    std::map<int, std::array<int, 3>> emem;   // (v3q, Q_MEM) enemy id -> (tile, round, estimated length) when last seen
    uint64_t tw_out = 0;                       // (7 Oct, TW) the warning to send this turn (0: none)
    bool tq_dead = false;                      // (7 Oct, QDEAD) their queen is known dead (sticky)
    int ff_out_until = -1000;                  // (8 Oct, FF) after a flip: keep out of dense water until then
    int ff_walk_dir = -1;                      //   this turn: the first step to the exit of the dense water I am in
    std::vector<int> ff_mark, ff_first;        //   the exit search: visit stamps, first step
    int ff_stamp = 0;
    int tq_learned = -1000, tq_hops = 0;       //   when I learned it, and how many relays it took to reach me
    int tq_seen_g = -1, tq_seen_r = -1000, tq_seen_cd = 0;   //   their queen's head where I last saw it (and that tile's countdown)
    std::vector<int> tq_seen_tiles;            //   every tile of hers I saw then
    bool oq_dead = false;                      // (8 Oct, ARMY) OUR queen is known dead (sticky)
    int oq_learned = -1000, oq_hops = 0;
    int oq_seen_g = -1, oq_seen_r = -1000;     //   our queen's head where I last saw it
    std::vector<int> oq_seen_tiles;
    int prevw_r = -1000;                       //   last turn's window: tile, pearl, countdown (new pearls = drops)
    int prevw_g[49], prevw_p[49], prevw_cd[49];
    uint64_t tw_relay = 0;                     //   a warning heard this turn, to pass on
    std::vector<std::array<int, 3>> tw_mem;    //   warnings heard: (tile, round seen, estimated length)
    bool qk = false;                     // (v3q) Q_KING applies this turn (off once the queen is presumed dead: Q_SILENT)
    int q_last = -1000;                  // (v3q) the last round I heard of or saw our queen
    int scout_goal = -1;
    bool assn = false;
    std::vector<int> banned;              // banned.get(g, -1)
    // --- this turn (set in _turn, read by _feed)
    int cur_wp[49];
    bool feed_gate = false;
    // --- scratch space (the Python dicts / sets of one call)
    std::vector<int> gi_arr;              // global tile -> window index (-1: not in view)
    int gi_wg[49];
    bool gi_set = false;
    TileSet others;                       // tiles of other dragons in view
    TileSet pearls;                       // tiles with a pearl in view
    TileSet pexit_w;                      // (v3.9, SPLIT_PEXIT 2) walls for the parent after a split
    TileSet kid_w;                        // (7 Oct, SPLIT_KID) walls for the newborn after a split
    TileSet room_set;                     // (7 Oct, ROOM_GUEST) the closed room my head is in
    IntMap room_d;                        // (7 Oct, ROOM_GUEST) room tile -> steps to its nearest exit tile
    bool room_guest_now = false;
    int room_owner_tiles = 0;             // (7 Oct, ROOM_CAP) > 0: I own the closed room my head is in (its tile count)
    std::vector<std::pair<int, int>> room_avoid;   // (7 Oct, ROOM_AVOID) (portal id, until round) I left an owned room by
    IntMap kid_bat;                       // (7 Oct, SPLIT_KID) the newborn's own body: tile -> step it frees up
    IntMap body_at;
    IntMap ba_scratch;                    // _escape_sprint's body map
    std::vector<int> surv_used;           // _survive: tile -> step (0: not on the path)
    std::vector<int> room_dist;           // _room / _pocket_mem / _tail_room: 0 = not reached
    std::vector<int> nav_par;             // _navigate: tile -> parent (NOVAL: not reached)
    std::vector<int> nav_par_keys;
    std::vector<int> nav_bctx;
    std::vector<int> nav_bctx_keys;
    TileSet nav_blocked;
    TileSet nav_soft;                     // (7 Oct, NAV_BODY_DELAY) other dragons' bodies: passable later
    TileSet mine_set;
    // (4 Oct, LANE) the dead-end lanes of the known map: ln_depth[g] = steps in from the network (0: not in a lane)
    std::vector<int> ln_deg, ln_depth, ln_q;
    int turns_played = 0;                 // (4 Oct, NB_NOSPLIT) turns this process has answered before this one
    std::vector<int> ln_occ_seen;         // (4 Oct, LANE) tile -> the last round I saw another dragon on it (-1000: free since)
    std::vector<char> ln_br;
    // this turn's view, for lane_food (set in _turn)
    const int* ln_wp = nullptr;
    const int* ln_wcd = nullptr;
    const int* ln_occ = nullptr;
    int ln_rnd = 0;
    // (island_area's marks)
    std::vector<uint8_t> isl_mark;

    Brain(int did, const std::string& tm, int w, int h, int lim, const Params& params)
        : id(did), team(tm), tbit(tm == "B" ? 1 : 0), W(w), H(h), N(w * h), limit(lim), P(params) {
        // (RNG 1, always) the LCG seeded by my id
        g_rs = (uint64_t)(int64_t)did * 2654435761ULL + 12345ULL;
        // (2 Oct, QFIX) the queens are ids 0 and 1, but the ladder swaps a map's sides at random (and with them which team
        // gets id 0): a queen is whoever has id 0 or 1; everyone else tells ours from theirs by the team letter in view
        is_queen = did == 0 || did == 1;
        qid = is_queen ? did : tbit;      // a guess until a queen is seen (unused by the checks below)
        eqid = 1 - qid;
        for (auto& o : P.area_ovr) {   // (7 Oct) area overrides, now that the map size is known
            const double a = (double)w * h;
            if (o.below ? a < o.area : a >= o.area) P.*(o.field) = o.v;
        }
        if (T(P.ORA)) {   // (8 Oct, ORA) the known maps of my map's size
            for (int k = 0; k < N_KMAPS; k++)
                if (KMAPS[k].W == w && KMAPS[k].H == h) {
                    ora_cand.push_back(k);
                    ora_kinds.push_back(b64dec(KMAPS[k].kinds));
                    ora_his.push_back(b64dec(KMAPS[k].his));
                }
            if (ora_cand.empty()) ora_state = -1;
            else ora_counted.assign((size_t)(2 * N), 0);
        } else {
            ora_state = -1;
        }
        int n = N;
        seen.assign((size_t)n, UNSEEN);
        hot.init(n);
        nbrs.resize((size_t)n);
        for (int g = 0; g < n; g++) {
            int x = g % W;
            int e = x + 1 < W ? g + 1 : g + 1 - W;
            int so = g + W < n ? g + W : g + W - n;
            nbrs[(size_t)g] = {g >= W ? g - W : g - W + n, e, so, x ? g - 1 : g - 1 + W};
        }
        special.init(2 * n);
        pt_idx.assign((size_t)n, -1);
        zone.assign((size_t)n, 0);
        pseen.assign((size_t)n, -9);
        mate_rnd.assign((size_t)n, -100000);
        gossip_told.assign((size_t)n, -1);
        founts.init(n);
        rush_target = (int)(fdiv(H, 2) * W + fdiv(W, 2));
        nav_credit = P.NAV_MAX;
        beacon_round = P.BEACON_ROUND;
        ksplit_round = P.KING_SPLIT_ROUND;
        guard_round_eff = P.GUARD_ROUND;
        banned.assign((size_t)n, -1);
        gi_arr.assign((size_t)n, -1);
        others.init(n);
        pearls.init(n);
        pexit_w.init(n);
        kid_w.init(n);
        room_set.init(n);
        room_d.init(n);
        kid_bat.init(n);
        body_at.init(n);
        ba_scratch.init(n);
        surv_used.assign((size_t)n, 0);
        room_dist.assign((size_t)n, 0);
        nav_par.assign((size_t)n, NOVAL);
        nav_bctx.assign((size_t)n, NOVAL);
        nav_blocked.init(n);
        nav_soft.init(n);
        mine_set.init(n);
        for (int i = 0; i < 49; i++) cur_wp[i] = 0;
        pos_learn = T(P.POS_W) && T(P.POS_LEARN);   // (POS_ORACLE is harness-only Python)
        if (pos_learn) {
            pos_rate.assign((size_t)n, -1.0);
            pos_due.assign((size_t)n, INT_MIN);
            pos_r5.assign((size_t)n, 0.0);
            pos_n5.assign((size_t)n, 0);
        }
    }

    // ---- what the brain does, by topic. Each file is a run of member functions (their order does not matter).
#include "map_memory.hpp"    // the remembered map: neighbours, portal tiles, edges
#include "symmetry.hpp"      // (SYM, off) the map as a mirror image of itself
#include "known_maps.hpp"    // (ORA) recognising a ladder map
#include "food.hpp"          // spawn rates, dense water, the route planner, farm & flip
#include "sonar.hpp"         // the words sent and read: beacon, gossip, warnings, queen deaths
#include "lanes.hpp"         // (LANE) dead-end lanes; ROOM_AVOID
#include "body_search_bridge.hpp"
#include "search.hpp"        // survive, room, pockets, islands
#include "navigate.hpp"      // the route when nothing in view is worth eating
#include "fight.hpp"         // the ram (try_kill)
#include "endgame_feed.hpp"  // feed, late_feed, merge_feed: one dragon gets the length
#include "split.hpp"         // when and how to split
#include "sprint.hpp"        // free steps and escape sprints
#include "queen.hpp"         // the queen: portals, the vault, and where her other rules are
#include "traversal_bridge.hpp"
#include "queen_safety_bridge.hpp"
#include "turn.hpp"          // turn(), fallback(), emit_move() and turn_() - the decision, in turn_*.hpp
};
