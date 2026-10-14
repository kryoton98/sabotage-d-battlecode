# The bot

This is our final general bot, the **smart-queen build**: C++, about 9,700 lines. It is the same source as our
`CPU_SMART_QUEEN` upload (also uploaded as `QUEEN-BOTH-RULES`); only the line endings were normalised. Every dragon
runs its own copy of this program, and the copies can only talk to each other by sonar.

The build that was active when submissions froze for the Qualifier (v3.9zq-exp) was this bot plus map rules for the
ladder maps. Those rules switch on only when the bot recognises one of the 18 ladder maps exactly, and every
Qualifier map was new, so in the Qualifier it played as the code in this folder. The ladder-only rules are not
published here; [docs/ladder.md](../docs/ladder.md) explains what they were.

## Build and run

```sh
g++ -O2 -std=c++20 -o bot *.cpp            # a local build; the judge compiles every .cpp and links them
pip install --upgrade unswbc               # the official toolkit (engine, maps, replay files)
unswbc run some_map.map bot bot            # a local game against itself; the replay goes to replays/
python ../tools/render_replay.py replays/<file>.replay --end -o final.png
```

The judge starts the bot with no arguments, so the defaults in `params.hpp` are what plays. For local tests, a JSON
object in the first argument overrides parameters:

| Key | Effect |
|---|---|
| `"NAME": v` | sets the parameter everywhere |
| `"A1100:NAME": v` | only on maps of 1,100 tiles or more |
| `"B1100:NAME": v` | only on maps under 1,100 tiles |
| `"M<k>:NAME": v` | only once ladder map k is recognised |

## How it is organised

`main.cpp` includes the `.hpp` files once, inside one namespace. They are pieces of one program, not separate
headers. Each file starts with a comment saying what it holds. The four other `.cpp` files hold the searches
that were added last.

**One turn** (`turn.hpp`, then the `turn_*.hpp` parts in this order):

| Part | File | What happens |
|---|---|---|
| 1 | `turn_input.hpp` | parse the round's input; tiles, pearls, countdowns and kelp/portal edges go into memory |
| 2 | `turn_dragons.hpp` | the dragons in view, queens seen or dead, the clocks, who is king, my own body |
| 3 | `turn_threats.hpp` | enemy reach: every tile an enemy head could ram before my next turn |
| 4 | `turn.hpp` | kill? feed? the late feed and the gathering at the "crown" |
| 5 | `turn_lanes_rooms.hpp` | friends' exits, never-trap, dead-end lanes, closed rooms, the queen's vault |
| 6 | `turn_split.hpp` | split? (the opening cascade and the routine split) |
| 7 | `turn_move_targets.hpp` | candidate steps, territory, food targets, dead-end pockets in view |
| 8 | `turn_move_roles.hpp` | the dragon's role: guard, king, the walk to the crown, the queen hunt |
| 9 | `turn_move_weights.hpp` | the weights and tables the scoring uses |
| 10 | `turn_move_score.hpp` | one score for every candidate step (food, survival depth, threat, room, portals, queen terms) |
| 11 | `turn_move_choose.hpp` | route when nothing is worth eating, the queen's last checks, sprints, the move |

**The modules the turn calls:**

| File | What it does |
|---|---|
| `brain.hpp` | one dragon's whole memory (one process is one dragon) |
| `map_memory.hpp`, `navigate.hpp`, `search.hpp` | the remembered map, routes over it, survival and room searches |
| `food.hpp` | estimated spawn rate of every tile, from the countdowns seen |
| `fight.hpp` | when to ram (only trades that pay) |
| `split.hpp`, `sprint.hpp` | splitting rules; multi-step moves and escape sprints |
| `sonar.hpp` | the 64-bit words dragons send: the king's beacon, "a queen is dead", the team's contact estimate |
| `endgame_feed.hpp` | the endgame: short dragons die beside a long one so it eats the pearls they drop |
| `lanes.hpp` | dead-end lanes farmed as a relay |
| `queen.hpp` | the queen's portal rules and her "vault" (a sealed space to hide in) |
| `known_maps.hpp`, `known_maps_data.hpp` | recognising a ladder map exactly (used here only for Trophy's spawn rates and the Around UNSW opening) |
| `params.hpp`, `param_names.hpp`, `params_json.hpp` | every tunable with its default; the JSON overrides |

**The smart-queen layer**, added last:

| File | What it does |
|---|---|
| `queen_safety.cpp`, `queen_safety_bridge.hpp` | a queen-only check after the normal choice. With an enemy in view it searches for an escape (QUEEN_RETREAT). On maps of 1,100+ tiles she takes a portal only when no checked ordinary route is safe (QUEEN_PORTAL_LAST). |
| `body_search.cpp`, `body_search_bridge.hpp` | "after this step, can I keep moving, given where my own body will be?" Only a complete answer replaces the old survival estimate. |
| `traversal_search.cpp`, `traversal_sim.cpp`, `traversal_bridge.hpp` | ordinary worker travel can pick a different free path after the same first step |

These searches stop on node caps and on a 52–62 ms clock. In our judge-CPU measurements of this code (with the
ladder rules on top) the heaviest map averaged about 4M points a turn, with a peak of about 10M; the limit is 100M.
`BODY_TRACE` and `TRAVEL_TRACE` are on in this build: they only write debug notes into the replay and do not change
the play.
