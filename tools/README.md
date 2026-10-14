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

## Drawing replays

| File | What it does |
|---|---|
| `render_replay.py` | Draws one round, the final board, or an animated GIF of a range of rounds, of the whole map or a window of it. Our team is blue and the other team orange. A gold ring marks each queen, a white cross marks a dragon that died in the last four rounds, and the yellow dots are pearls. Kelp is drawn green and portals violet. Needs Pillow. |
| `compose_strip.py` | Puts a few frames side by side. |
| `make_images.sh` | Re-draws every replay picture in `docs/images/replays` (the replays themselves are not in the repo). |

```sh
python tools/render_replay.py game.replay --end -o final.png
python tools/render_replay.py game.replay --round 150 --view 10,30,24,20 --scale 24 -o closeup.png
python tools/render_replay.py game.replay --gif 440:499:2 --ms 120 -o ending.gif
python tools/render_replay.py game.replay --list        # per round: dragons, longest, total, queen length
```

`--us A|B` says which side we were (ours is drawn blue), `--names "A name,B name"` labels the teams, and
`--mark x,y` draws a white ring to point something out.
