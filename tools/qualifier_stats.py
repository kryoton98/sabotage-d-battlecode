#!/usr/bin/env python3
"""Per-game facts for our Qualifier matches, rebuilt from the replays.

    python tools/qualifier_stats.py REPLAY_DIR            # REPLAY_DIR holds M<game id>.replay files
        -> data/qualifier_games.json, data/qualifier_games.csv, data/qualifier_timelines.json

The battles and which side we played come from data/qualifier_bracket.json. For every game it records the
map, how the game ended, both teams' final dragons / longest / total, both queens (alive and her length, or
the round and reason she died) and what decided the game. At round 500 the order is: queen length (a dead
queen counts 0), then the longest dragon, then total length.
"""
from __future__ import annotations

import csv
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from replay import load  # noqa: E402
from rstate import Game  # noqa: E402

SIDE_A = {"First round", "Round of 256", "Round of 128", "Round of 64"}   # we were listed first (side A) here


def decided_by(end, us, them, q_us, q_them):
    if end == "eliminated":
        return "elimination"
    if q_us != q_them:
        return "queen length"
    if us["longest"] != them["longest"]:
        return "longest dragon"
    if us["total"] != them["total"]:
        return "total length"
    return "draw"


def game_facts(path, us_side):
    rp = load(path)
    g = Game(rp, check=False, sprint_div=4)
    them_side = "B" if us_side == "A" else "A"
    timeline = []
    cur = None
    queens = None
    for tv in g.turns():
        if queens is None:
            queens = {g.all[i].team: i for i in (0, 1)}
        if tv.rnd != cur:
            cur = tv.rnd
            row = [cur]
            for side in (us_side, them_side):
                lens = [len(d.body) for d in g.dragons.values() if d.team == side]
                q = g.dragons.get(queens[side])
                row += [len(lens), max(lens) if lens else 0, sum(lens), len(q.body) if q else 0]
            timeline.append(row)
    res = rp.result

    def queen(side):
        qid = queens[side]
        d = g.dragons.get(qid)
        death = next(((r, why) for (r, i, t, why, L, k) in g.deaths if i == qid), None)
        return {"alive": d is not None, "length": len(d.body) if d else 0,
                "died_round": death[0] if death else None, "died_how": death[1] if death else None}

    q_us, q_them = queen(us_side), queen(them_side)
    m = g.map
    facts = {
        "map": m.name, "size": "%dx%d" % (m.W, m.H), "our_side": us_side, "end": res["end"], "rounds": cur,
        "winner": "us" if res["winner"] == us_side else ("them" if res["winner"] == them_side else "draw"),
        "us": res[us_side], "them": res[them_side], "our_queen": q_us, "their_queen": q_them,
    }
    facts["decided_by"] = decided_by(res["end"], res[us_side], res[them_side], q_us["length"], q_them["length"])
    if facts["winner"] == "draw":
        facts["decided_by"] = "draw"
    return facts, timeline


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    rdir = sys.argv[1]
    bracket = json.load(open(os.path.join(ROOT, "data", "qualifier_bracket.json"), encoding="utf-8"))
    games, timelines = [], {}
    for match in bracket["our_path"]:
        if not match.get("battle"):
            continue
        side = "A" if match["round"] in SIDE_A else "B"
        for k, (map_name, _wl) in enumerate(match["games"]):
            gid = match["battle"] + k
            path = os.path.join(rdir, "M%d.replay" % gid)
            if not os.path.exists(path):
                print("missing", path)
                continue
            facts, tl = game_facts(path, side)
            facts.update({"game": gid, "round": match["round"], "opponent": match["opponent"],
                          "opponent_seed": match["seed"], "game_no": k + 1, "map_title": map_name})
            games.append(facts)
            timelines[str(gid)] = tl
            print("%s g%d %-16s %-5s %-14s queens %s/%s" % (match["round"], k + 1, map_name, facts["winner"],
                                                            facts["decided_by"], facts["our_queen"]["length"],
                                                            facts["their_queen"]["length"]))
    out = os.path.join(ROOT, "data")
    with open(os.path.join(out, "qualifier_games.json"), "w", encoding="utf-8") as f:
        json.dump(games, f, ensure_ascii=False, indent=1)
    with open(os.path.join(out, "qualifier_timelines.json"), "w", encoding="utf-8") as f:
        json.dump({"columns": ["round", "our_dragons", "our_longest", "our_total", "our_queen",
                               "their_dragons", "their_longest", "their_total", "their_queen"],
                   "games": timelines}, f, separators=(",", ":"))
    with open(os.path.join(out, "qualifier_games.csv"), "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["game", "round", "opponent", "game_no", "map", "size", "result", "decided_by", "end", "rounds",
                    "our_dragons", "our_longest", "our_total", "their_dragons", "their_longest", "their_total",
                    "our_queen_len", "our_queen_died", "their_queen_len", "their_queen_died"])
        for gm in games:
            oq, tq = gm["our_queen"], gm["their_queen"]
            w.writerow([gm["game"], gm["round"], gm["opponent"], gm["game_no"], gm["map_title"], gm["size"],
                        gm["winner"], gm["decided_by"], gm["end"], gm["rounds"],
                        gm["us"]["dragons"], gm["us"]["longest"], gm["us"]["total"],
                        gm["them"]["dragons"], gm["them"]["longest"], gm["them"]["total"],
                        oq["length"], "" if oq["alive"] else "r%d %s" % (oq["died_round"], oq["died_how"]),
                        tq["length"], "" if tq["alive"] else "r%d %s" % (tq["died_round"], tq["died_how"])])
    print("wrote %d games" % len(games))


if __name__ == "__main__":
    main()
