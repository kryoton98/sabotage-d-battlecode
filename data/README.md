# Data

| File | What it is | Where it came from |
|---|---|---|
| `qualifier_bracket.json` | Our path through the Qualifier (every game's map and result), the main bracket from the Round of 32, the second-chance bracket and the ten Grand Final qualifiers | the bracket on game.battlecode.au, read 14 Oct 2026 |
| `qualifier_games.json`, `qualifier_games.csv` | One row per Qualifier game (42): map, size, how it ended, both teams' final dragons / longest / total, both queens (alive and her length, or when and how she died) and what decided the game | `tools/qualifier_stats.py` over the 42 replays |
| `qualifier_timelines.json` | Per game and round: each team's dragons, longest, total length and queen length | the same script |
| `ladder_rating_hourly.csv` | Our ladder rating and rank, last value in each UTC hour, 1 to 14 Oct | the rating history on our team page |
| `queen_ab_test.csv` | The local A/B test of the smart-queen bot against our ladder bot of 8 Oct | our local test runs, 9 Oct |

At round 500 a game goes to the longer queen (a dead queen counts 0), then the longest dragon, then the total
length. Applied to the final boards in `qualifier_games.json`, that order gives the official winner in all 33 games
that went the distance. The other 9 ended early: 8 by elimination and 1 draw in which both teams were eliminated.
