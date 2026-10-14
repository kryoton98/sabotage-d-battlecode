# How we tested

Most of our ideas did not work. The ones that did were found by testing everything the same way.

## Paired A/B tests, and enough of them

- **Every change goes in behind a switch.** Each rule is a parameter, so a new rule can be turned on and off
  without touching the code.
- **Paired games.** The new version and the old one play the same maps, with the same seeds, from the same sides,
  against the same opponent. We compare them game by game.
- **Several opponents.** The sparring pool held older versions of our own bot and a few other bots. A change that
  only beats one opponent is usually exploiting that opponent.
- **Two kinds of maps.** Most tests ran on the ladder maps and on generated maps the bot had never seen. The
  tournaments are played on unseen maps.
- **Enough games.** One 80-game run moves about ±11 points either way by chance alone. Finding a gain of 2 or 3
  points needs many hundreds of games, more than a thousand for the smallest effects. Typical runs: 132-68 over 200 games (v2.9), 223-177 over 400 (v2.10),
  471-409 over 880 (v2.13).

## The harness

- **The official engine.** It runs locally (`pip install unswbc`), so local games follow the judge's rules.
- **Our own runner.** It plays hundreds of games in parallel on cloud machines and writes one JSON line per game:
  the map, seed and side, who won and how, both queens, every length.
- **A resumable job queue.** Cloud machines get restarted, so the runner skips any game that is already in its
  output file.
- **Small comparison scripts** read those files. One compares wins game by game. Another compares queen safety:
  how often our queen is alive at the end, and the average round she dies.

## Known maps against unseen maps

Some of the most useful results were the ones that went the wrong way:

- **v2.4** had tables for particular maps, such as Trophy's pearl schedule, and climbed the ladder. **v2.5** took
  every map table out and still went 79-81 against v2.4. The tables were worth rating, not strength.
- **v2.12** beat v2.11 274-166 (62.3%) on the known maps, but went 57-63 on unseen maps.
- **On 4 Oct** compiling the official map layouts into the bot won 76.9% of local games on those maps. We took it
  out. The main bot should not depend on the ladder maps, and the layouts cannot help on unseen maps.

## CPU

- **The limit.** The judge prices every turn in CPU points, up to 100M. The engine's sandbox mode
  (`unswbc run --sandbox`) prices local games the same way.
- **The house rule** we settled on: ship only if the worst turn stays under 50M on the maps that cost the most CPU
  (Big Empty, Schooltime and Trauma), with the zip under 4 MB.
- **Python.** Every executed line cost about 1,500 points, so 100M bought about 3,000 nodes of breadth-first
  search, and every search had a cap. v2.15 averaged 20.9M a turn and peaked at 62.1M on Slithery Fight, too close
  to the limit for comfort.
- **The C++ port** (27 Sept) averaged 3.0M a turn, with a peak of 4.5M. Before switching we checked it reply by
  reply. On 586,984 recorded turn inputs the C++ and Python bots gave the same answer, on the judge's wasm build
  too. The 16 test games they played were identical.

## Replays

- **Downloading.** We downloaded our ladder games with the site's API. The Qualifier replays come from the "Download
  all replays" button.
- **Analysis.** [tools/replay.py](../tools/replay.py) and [tools/rstate.py](../tools/rstate.py) rebuild a whole game.
  On top of them, small scripts answered questions like "where does our queen die, and to what?" and "which maps
  do we lose, and how?".
- **Debugging.** The state rebuild can re-create the exact input a dragon saw on any turn. A bad move in a ladder
  replay can be fed back into the bot and debugged on its own.
- **Pictures.** [tools/render_replay.py](../tools/render_replay.py) draws the replay pictures in this write-up,
  and [tools/make_charts.py](../tools/make_charts.py) draws the charts.

## The ladder as a test

The ladder is noisy, and opponents change their bots all the time. We read it through per-map performance
ratings over hundreds of games, and compared the same maps before and after a change; [ladder.md](ladder.md) shows
how.
