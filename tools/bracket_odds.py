#!/usr/bin/env python3
"""How likely was each finish for one seed in the Qualifier bracket? A Monte Carlo over the real seeds.

    python tools/bracket_odds.py 18

Every match is best of 7, and each game is an Elo coin: P(a beats b) = 1 / (1 + 10^(-(Ra - Rb) / 400)).
The bracket is the standard seeded order in 512 slots (280 teams, the rest are byes). The 8 Round-of-16 losers
play a second-chance bracket reseeded by their original seed, and its 2 finalists also go to the Grand Final.
--shrink < 1 flattens the ratings (more upsets, as unseen maps might bring).
"""
import argparse
import json
import math
import os
import random

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def order(n):
    o = [1, 2]
    while len(o) < n:
        m = 2 * len(o)
        o = [x for s in o for x in (s, m + 1 - s)]
    return o


def bo7(p):
    return sum(math.comb(3 + k, k) * p ** 4 * (1 - p) ** k for k in range(4))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("seed", type=int)
    ap.add_argument("--runs", type=int, default=40000)
    ap.add_argument("--shrink", type=float, default=1.0)
    a = ap.parse_args()
    with open(os.path.join(ROOT, "data", "qualifier_seeds.json")) as f:
        elo = {int(k): v for k, v in json.load(f)["rating_by_seed"].items()}
    n_teams, size = len(elo), 512
    slots0 = [s if s <= n_teams else None for s in order(size)]
    cache = {}

    def pm(x, y):
        if (x, y) not in cache:
            cache[(x, y)] = bo7(1 / (1 + 10 ** (-(elo[x] - elo[y]) * a.shrink / 400)))
        return cache[(x, y)]

    rng = random.Random(1)
    us = a.seed
    hit = {"Round of 16": 0, "quarterfinal (Grand Final)": 0, "Grand Final via second chance": 0}
    for _ in range(a.runs):
        slots = slots0
        last16 = last8 = None
        while len(slots) > 1:
            nxt = []
            for i in range(0, len(slots), 2):
                x, y = slots[i], slots[i + 1]
                if x is None:
                    nxt.append(y)
                elif y is None:
                    nxt.append(x)
                else:
                    nxt.append(x if rng.random() < pm(x, y) else y)
            slots = nxt
            if len(slots) == 16:
                last16 = set(slots)
            if len(slots) == 8:
                last8 = set(slots)
        if us in last16:
            hit["Round of 16"] += 1
            if us in last8:
                hit["quarterfinal (Grand Final)"] += 1
            else:
                losers = sorted(last16 - last8)
                sl = [losers[i - 1] for i in order(8)]
                while len(sl) > 2:
                    sl = [sl[i] if rng.random() < pm(sl[i], sl[i + 1]) else sl[i + 1] for i in range(0, len(sl), 2)]
                if us in sl:
                    hit["Grand Final via second chance"] += 1
    print("seed %d (rating %d), %d runs, shrink %.2f" % (us, elo[us], a.runs, a.shrink))
    for k, v in hit.items():
        print("  %-32s %5.1f%%" % (k, 100.0 * v / a.runs))
    gf = hit["quarterfinal (Grand Final)"] + hit["Grand Final via second chance"]
    print("  %-32s %5.1f%%" % ("Grand Final, either way", 100.0 * gf / a.runs))


if __name__ == "__main__":
    main()
