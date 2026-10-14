#!/usr/bin/env bash
# Re-draws every replay picture in docs/images/replays from the replay files.
#   QUAL=<folder with the Qualifier M*.replay files> LADDER=<folder with M1501101.replay> bash tools/make_images.sh
# The Qualifier replays are the "Download all replays (.zip)" files of our six Qualifier battles on
# game.battlecode.au; they are not in this repo (about 260 MB).
set -euo pipefail
cd "$(dirname "$0")/.."
QUAL=${QUAL:?set QUAL to the folder with the Qualifier replays}
LADDER=${LADDER:-}
OUT=docs/images/replays
mkdir -p "$OUT"
R="python3 tools/render_replay.py"
CMO="Cache me outside,Sabotage-d"          # Round of 32: we were side B
LARP="larp stock zi balls,Sabotage-d"      # Round of 16: side B
TTT="TTT is All You Need,Sabotage-d"       # second chance: side B
MSD="Sabotage-d,Martin Shkreli’s Defense Team"   # Round of 64: side A

# Round of 32, game 7 (Troll Australia): the 4-3 decider, won on queen length
$R "$QUAL/M1587764.replay" --end --us B --names "$CMO" --title "Round of 32, game 7 · Troll Australia" \
   --caption "Qualifier · 10 Oct 2026" -o "$OUT/r32-g7-final.png"
$R "$QUAL/M1587764.replay" --gif 0:499:4 --ms 90 --scale 7 --us B --names "$CMO" \
   --title "R32 game 7 · Troll Australia" -o "$OUT/r32-g7-full.gif"
for r in 0 40 100; do
  $R "$QUAL/M1587764.replay" --round $r --scale 7 --us B --names "$CMO" --title "Troll Australia" -o "$OUT/r32-g7-r$r.png"
done

# ... and in round 498 a 4-long dragon of ours rams their 77-long dragon head-on
$R "$QUAL/M1587764.replay" --gif 492:499:1 --ms 650 --view 4,36,18,13 --scale 26 --us B --names "$CMO" \
   --title "R32 game 7 · the last ram" -o "$OUT/r32-g7-last-ram.gif"

# Round of 64, game 3 (Ouroboros): a 3-long enemy chases our 2-long queen for 15 rounds; she survives and ends at 66
$R "$QUAL/M1587606.replay" --gif 172:192:1 --ms 450 --view 54,21,16,15 --scale 28 --us A --names "$MSD" \
   --title "R64 game 3 · the queen runs" -o "$OUT/r64-g3-queen-chase.gif"

# Second chance, game 4 (Hive): the late gathering - from round 460 our dragons feed one dragon
$R "$QUAL/M1587936.replay" --gif 440:499:1 --ms 120 --scale 9 --us B --names "$TTT" \
   --title "Second chance, game 4 · Hive" -o "$OUT/sc-g4-gathering.gif"

# Round of 16, game 5 (Detention): what beat us - one 195-long dragon
$R "$QUAL/M1587839.replay" --end --scale 11 --us B --names "$LARP" --title "Round of 16, game 5 · Detention" \
   -o "$OUT/r16-g5-final.png"

# Ladder, 8 Oct (Portals): the queen takes a portal into a one-tile pocket and dies on the kelp
if [ -n "$LADDER" ]; then
  $R "$LADDER/M1501101.replay" --gif 70:78:1 --ms 600 --scale 22 --us A --names "Sabotage-d,forgot to mention" \
     --title "Ladder · Portals" --mark 5,2,1.4 --mark 26,14,1.4 -o "$OUT/ladder-portals-pocket.gif"
fi
python3 tools/compose_strip.py "$OUT/r32-g7-r0.png" "$OUT/r32-g7-r40.png" "$OUT/r32-g7-r100.png" -o "$OUT/r32-g7-opening.png"
rm -f "$OUT/r32-g7-r0.png" "$OUT/r32-g7-r40.png" "$OUT/r32-g7-r100.png"
