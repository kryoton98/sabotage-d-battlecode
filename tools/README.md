# Tools

Python 3. Everything here is our own code. The reader needs nothing outside the standard library.

## Reading replays

| File | What it does |
|---|---|
| `replay.py` | Reads a `.replay` file (packed Cap'n Proto; the schema was worked out from the official viewer). Gives the map text, both bots' names, the result and the event list: rounds, turns, actions, splits, deaths, sonar. Handles the gzip-compressed files the ladder serves. |
| `rstate.py` | Rebuilds the whole game from those events, turn by turn: every dragon's body, the pearls, the countdowns, the deaths. It can also re-create the exact input block each dragon was given, so a turn seen in a replay can be fed back to the bot and debugged. Checked turn by turn against games played on the local engine. |

```python
from replay import load
from rstate import Game

g = Game(load("M1587764.replay"), sprint_div=4)   # sprint_div=4: the first ceil(L/4) sprint steps are free
for turn in g.turns():                           # one view per dragon turn, before its action is applied
    ...
print(g.deaths)                                  # (round, id, team, reason, length, killer)
```

The queens are dragons 0 and 1, but which team each belongs to changes from game to game. Look it up with
`g.all[0].team`.
