# Version history

From the first Python bot in September to the build that played the Qualifier. Each version was tested against
the one before it (see [testing.md](testing.md)), and the numbers are the ones we wrote down at the time. Dates are
as logged: up to v3.8 they are UTC, from v3.9 on mostly Sydney time.

## Python: the swarm (to 23 Sept)

| Version | Date | What changed | Result |
|---|---|---|---|
| v1 | by 22 Sept | eat, split 2+2 at length 4, ram only good trades, a dead-end search, its own map memory | 216-24 on the ladder, #3 at the time |
| v2 | 22 Sept | the first endgame: feed the longest dragon from round 450; guards | 62% against v1 over 128 games |
| v2.2-v2.4 | 22 Sept | map tables (Trophy), fixed pearl schedules, a rush to the centre | 43-17 in v2.4's first 60 ladder games |
| v2.5 | 23 Sept | **every map table removed**, on purpose | 79-81 against v2.4: the tables were rating, not strength |
| v2.6, v2.7 | 23 Sept | memory fixes; the pocket run (eat into a dead end, split out backwards) | 85-55 against v1; v2.7 85-75 against v2.6 |

## Python: the king economy (24-26 Sept)

| Version | Date | What changed | Result |
|---|---|---|---|
| v2.8 | 24 Sept | **the king:** feeding from round 380, a sonar beacon for the king, a hunger rule; routes through mazes | 150-50 (75%) against v2.7 |
| v2.9 | 24 Sept | one king for the whole team: a 64-bit beacon with the king's id | 132-68 (66%) against v2.8 |
| v2.10 | 25 Sept | the opening rush back; a safer king | 223-177 (55.8%) over 400 |
| v2.11 | 25 Sept | "fountain runs": a dead-end pocket with 3+ food tiles is worth a run | 241-199 (54.8%) |
| v2.12 | 25 Sept | an early king economy when the team is big | 274-166 on known maps, but 57-63 on unseen maps |
| v2.13-v2.15 | 26 Sept | short dragons trade when outnumbering; a boxed dragon splits off its body | 471-409, then 579-499, then 471-447 |

## C++ and the clock switch (27-29 Sept)

| Version | Date | What changed | Result |
|---|---|---|---|
| v2.15 in C++ | 27 Sept | **the port**, function by function, built on Python's arithmetic | identical to the Python bot on 586,984 replies; 827-457 in its first 1,284 ladder games |
| v2.16-v2.18 | 28-29 Sept | **the clock switch:** the team measures how much the two swarms have met. Where they mix, everyone eats until late and the king is fed for the last 50 rounds; where they never meet, the king starts at round 200 | v2.18: 469-191 (71.1%) in its first 660 ladder games |

The team was #7 on the ladder (2001) on 27 Sept. At the **Sprint** tournament on 1 Oct we were seeded 18th of 591,
won three rounds (7-0, 7-0, 5-2), and lost 3-4 to Check Raise in the Round of 64.

## The queen rules (1-5 Oct)

| Version | Date | What changed | Result |
|---|---|---|---|
| v3.0 | 1 Oct | **new rules:** dragons 0 and 1 are queens, the longer queen wins at round 500, the first ⌈L/4⌉ sprint steps are free. A queen-safety package for maps of 1,100+ tiles | 142-98 (59.2%) against v2.18 |
| v3.1 | 2 Oct | fix: on side-swapped maps our queen had played as an ordinary dragon | +2.8 points |
| v3.2-v3.4 | 3 Oct | portal landings; chained free sprints; the queen buds only while the team is under 20 | v3.4 +9.3 points against v2.18 |
| v3.5, v3.6 | 4 Oct | the official map layouts compiled in | 76.9% locally, **dropped**: no hard-coded maps in the main bot |
| v3.7 | 4 Oct | an opening pull towards the centre on small maps | 217 of 320 (67.8%) against v3.4 on small maps |
| v3.8 | 4 Oct | **lanes:** dead-end lanes farmed as a relay; no split on a newborn's first turn | 133 of 240 against v3.7 |
| v4.0, v4.2 | 5 Oct | a rewrite from scratch, from seven design documents | 26 of 312 (8.3%) against v3.8. **Abandoned** |

## The v3.9 series (6-9 Oct)

| Version | Date | What changed | Result |
|---|---|---|---|
| v3.9-v3.9c | 6 Oct | the rewrite's ideas re-tested as switches on v3.8; only the queen's tether passed | +27 over 360 against a sparring bot |
| v3.9d-v3.9j | 7 Oct | queen safety: no blind corridors, she never feeds, friends keep her exits open, the look-ahead; lanes on mid maps | small gains, one switch at a time |
| v3.9k-v3.9r | 8 Oct | queen guards, the queen keeps out of reach and off portals on small maps, no late growth splits | v3.9l 137 of 236 against v3.9k |
| v3.9s-v3.9v | 8 Oct | **map recognition** for the ladder maps; a few map rules on it | see [ladder.md](ladder.md) |
| v3.10, v3.11 | 9 Oct | general builds without map recognition: spawn rates learned from countdowns | about +19 over 104 |
| gathering | 8-9 Oct | **the late gathering** from round 460 | performance rating 2265 against 2229 |
| modules | 9 Oct | the bot split into `main.cpp` + modules | 12 of 12 games identical to the single-file bot |
| smart queen | 9 Oct | escape search, portals last, body-aware searches (`CPU_SMART_QUEEN`) | the bot in [bot/](../bot/) |
| v3.9zq-exp | 9 Oct | the smart queen plus the ladder-only map rules: **active at the Qualifier freeze** | on new maps it plays as the smart queen |

## Tried and dropped

- **Mirror symmetry.** Learning which mirror image a map is, then filling in the unseen half: 95-105. It decided
  right, but too late in the game.
- **Sharing the map by sonar.** 62-88: the swarm crowded together and starved. Threat warnings were dropped too,
  because a beam stops at the first dragon part in line.
- **Waiting at portals.** A portal "clock" lost every race through a portal (39%).
- **Avoiding even trades.** 44.5%: staying out of even trades gives the food away.
- **Spending CPU on deeper search.** Four times the search caps scored 49.2%, and a bigger route planner lost. Ten
  times the budget changed 4 games out of 198.
- **An early gathering** at round 300 went 34 of 168 against 69: an early merge kills the economy. Only the round-460
  gathering stayed.
- **The rewrite** (v4.0) lost 8.3% of its games against the bot it was meant to replace. Its ideas came back one at
  a time as switches, and most of them failed there too.
