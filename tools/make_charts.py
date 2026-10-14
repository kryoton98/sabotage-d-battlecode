#!/usr/bin/env python3
"""Draw the charts in docs/images/charts from the files in data/.

    python tools/make_charts.py

Colours: our team is always blue, opponents orange, a draw or an older version of our bot gray.
"""
from __future__ import annotations

import csv
import json
import os
from datetime import datetime

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.patches import FancyBboxPatch  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, "data")
OUT = os.path.join(ROOT, "docs", "images", "charts")

SURFACE = "#fcfcfb"
INK = "#0b0b0b"
INK2 = "#52514e"
MUTED = "#8a8984"
GRID = "#e6e5e0"
US = "#2a78d6"
THEM = "#eb6834"
GRAY = "#a3a29c"

plt.rcParams.update({
    "font.family": ["DejaVu Sans", "Noto Sans CJK JP"], "font.size": 10.5, "text.color": INK, "axes.labelcolor": INK2,
    "axes.edgecolor": GRID, "axes.linewidth": 1.0, "xtick.color": INK2, "ytick.color": INK2,
    "xtick.major.size": 0, "ytick.major.size": 0, "axes.titlesize": 12, "axes.titleweight": "bold",
    "axes.titlelocation": "left", "axes.titlepad": 10, "figure.facecolor": SURFACE, "axes.facecolor": SURFACE,
    "savefig.facecolor": SURFACE, "legend.frameon": False,
})


def tidy(ax, grid_axis="y"):
    for s in ("top", "right", "left"):
        ax.spines[s].set_visible(False)
    ax.spines["bottom"].set_color(GRID)
    if grid_axis:
        ax.grid(axis=grid_axis, color=GRID, linewidth=1.0)
        ax.set_axisbelow(True)


def save(fig, name):
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name)
    fig.savefig(path, dpi=150, bbox_inches="tight", pad_inches=0.25)
    plt.close(fig)
    print("wrote", os.path.relpath(path, ROOT))


def games():
    with open(os.path.join(DATA, "qualifier_games.json"), encoding="utf-8") as f:
        return json.load(f)


SHORT_ROUND = {"Round of 256": "R256", "Round of 128": "R128", "Round of 64": "R64", "Round of 32": "R32",
               "Round of 16": "R16", "Second chance, quarterfinal": "2nd chance"}
DECIDER = {"elimination": "elim.", "queen length": "queen", "longest dragon": "longest", "draw": "draw",
           "total length": "total"}


def chart_games_grid():
    G = games()
    rows = []
    for g in G:
        if not rows or rows[-1][0] != g["round"]:
            rows.append((g["round"], g["opponent"], g["opponent_seed"], []))
        rows[-1][3].append(g)
    fig, ax = plt.subplots(figsize=(11.5, 5.0))
    ax.set_xlim(0, 7)
    ax.set_ylim(len(rows), 0)
    ax.axis("off")
    for r, (rnd, opp, seed, gs) in enumerate(rows):
        w = sum(g["winner"] == "us" for g in gs)
        lo = sum(g["winner"] == "them" for g in gs)
        dr = sum(g["winner"] == "draw" for g in gs)
        score = "%d-%d" % (w, lo) + (", %d draw" % dr if dr else "")
        label = "%s  ·  %s (seed %d)  ·  %s" % (SHORT_ROUND[rnd], opp, seed, score)
        ax.text(-0.08, r + 0.5, label, ha="right", va="center", fontsize=10, color=INK)
        for c, g in enumerate(gs):
            col = US if g["winner"] == "us" else (THEM if g["winner"] == "them" else GRAY)
            ax.add_patch(FancyBboxPatch((c + 0.05, r + 0.08), 0.9, 0.84, boxstyle="round,pad=0,rounding_size=0.08",
                                        linewidth=0, facecolor=col))
            ax.text(c + 0.5, r + 0.40, g["map_title"], ha="center", va="center", fontsize=8.6, color="white",
                    fontweight="bold")
            ax.text(c + 0.5, r + 0.68, DECIDER[g["decided_by"]], ha="center", va="center", fontsize=7.8,
                    color="white")
    tw = sum(g["winner"] == "us" for g in G)
    tl = sum(g["winner"] == "them" for g in G)
    td = sum(g["winner"] == "draw" for g in G)
    ax.set_title("Every Qualifier game: %d won, %d lost, %d drawn" % (tw, tl, td), x=-0.62)
    handles = [plt.Rectangle((0, 0), 1, 1, color=c) for c in (US, THEM, GRAY)]
    ax.legend(handles, ["won", "lost", "draw"], loc="lower left", bbox_to_anchor=(-0.62, -0.13), ncol=3,
              fontsize=9.5, handlelength=1.0, handleheight=1.0, columnspacing=1.4)
    ax.text(7.0, len(rows) + 0.42, "small text: what decided the game", ha="right", va="center", fontsize=8.5,
            color=INK2)
    save(fig, "qualifier-games.png")


def chart_decided_by():
    G = games()
    top = {"Round of 32", "Round of 16", "Second chance, quarterfinal"}
    groups = [("Rounds of 256, 128 and 64 (seeds 82 to 239), plus 1 draw", [g for g in G if g["round"] not in top]),
              ("Top-16 opponents: seeds 15, 2 and 11", [g for g in G if g["round"] in top])]
    cats = ["queen length", "longest dragon", "elimination"]
    fig, axes = plt.subplots(1, 2, figsize=(11, 3.4), sharex=True)
    for ax, (title, gs) in zip(axes, groups):
        tidy(ax, "x")
        for i, cat in enumerate(cats):
            w = sum(1 for g in gs if g["decided_by"] == cat and g["winner"] == "us")
            lo = sum(1 for g in gs if g["decided_by"] == cat and g["winner"] == "them")
            y = len(cats) - 1 - i
            if w:
                ax.barh(y, w - 0.06, left=0, height=0.34, color=US)
            if lo:
                ax.barh(y, lo - 0.06, left=w + 0.06 if w else 0, height=0.34, color=THEM)
            ax.text(w + lo + 0.35, y, "%d-%d" % (w, lo), va="center", fontsize=10, color=INK)
        ax.set_yticks(range(len(cats)))
        ax.set_yticklabels(list(reversed(cats)))
        ax.set_title(title, fontsize=11)
        ax.set_xlim(0, 14)
        ax.set_xlabel("games (won-lost)")
    handles = [plt.Rectangle((0, 0), 1, 1, color=c) for c in (US, THEM)]
    fig.legend(handles, ["won", "lost"], loc="upper right", ncol=2, fontsize=9.5, handlelength=1.0,
               bbox_to_anchor=(0.99, 1.04))
    fig.suptitle("How the Qualifier games were decided", x=0.01, ha="left", fontweight="bold", fontsize=13, y=1.06)
    fig.tight_layout()
    save(fig, "decided-by.png")


def chart_game7():
    with open(os.path.join(DATA, "qualifier_timelines.json")) as f:
        tl = json.load(f)
    cols = tl["columns"]
    rows = tl["games"]["1587764"]
    ix = {c: i for i, c in enumerate(cols)}
    r = [row[ix["round"]] for row in rows]
    fig, (a1, a2) = plt.subplots(2, 1, figsize=(10, 6.0), sharex=True, gridspec_kw={"hspace": 0.45})
    for ax in (a1, a2):
        tidy(ax)
    a1.plot(r, [row[ix["our_queen"]] for row in rows], color=US, lw=2)
    a1.plot(r, [row[ix["their_queen"]] for row in rows], color=THEM, lw=2)
    a1.set_title("Queen length (a dead queen counts 0)")
    a1.set_ylim(-0.3, 8)
    g7 = next(g for g in games() if g["game"] == 1587764)
    dead = g7["their_queen"]["died_round"]
    a1.annotate("their queen dies\n(round %d, head-on)" % dead, xy=(dead, 0), xytext=(dead + 30, 5.4),
                fontsize=9, color=INK2, arrowprops=dict(arrowstyle="-", color=MUTED, lw=1))
    a1.annotate("round 500: queens 4 v 0,\nso we win the game", xy=(499, 4), xytext=(350, 6.4), fontsize=9,
                color=INK2, arrowprops=dict(arrowstyle="-", color=MUTED, lw=1))
    a2.plot(r, [row[ix["our_total"]] for row in rows], color=US, lw=2)
    a2.plot(r, [row[ix["their_total"]] for row in rows], color=THEM, lw=2)
    a2.set_title("Total length of all dragons")
    a2.set_xlabel("round")
    a2.set_xlim(0, 500)
    a2.text(499, rows[-1][ix["our_total"]] - 30, "217", color=INK, fontsize=9, ha="right")
    a2.text(499, rows[-1][ix["their_total"]] + 12, "249", color=INK, fontsize=9, ha="right")
    fig.legend([plt.Line2D([], [], color=US, lw=2), plt.Line2D([], [], color=THEM, lw=2)],
               ["Sabotage-d", "Cache me outside"], loc="upper right", ncol=2, fontsize=9.5, bbox_to_anchor=(0.99, 0.965))
    fig.suptitle("Round of 32, game 7 (Troll Australia): less length, but the queen lived", x=0.01, ha="left",
                 fontweight="bold", fontsize=13, y=0.99)
    save(fig, "r32-g7-timeline.png")


def chart_longest_top16():
    G = [g for g in games() if g["round"] in ("Round of 32", "Round of 16", "Second chance, quarterfinal")]
    fig, ax = plt.subplots(figsize=(10, 7.2))
    tidy(ax, "x")
    n = len(G)
    for i, g in enumerate(G):
        y = n - 1 - i
        a, b = g["us"]["longest"], g["them"]["longest"]
        ax.plot([a, b], [y, y], color=GRID, lw=2.4, zorder=1, solid_capstyle="round")
        ax.scatter([b], [y], s=58, color=THEM, zorder=3, edgecolors=SURFACE, linewidths=2)
        ax.scatter([a], [y], s=58, color=US, zorder=3, edgecolors=SURFACE, linewidths=2)
        mark = {"us": "won", "them": "lost", "draw": "draw"}[g["winner"]]
        how = g["decided_by"]
        ax.text(max(a, b) + 4, y, "%s · %s" % (mark, how), va="center", fontsize=8.6,
                color=INK if how == "longest dragon" else MUTED)
    ax.set_yticks(range(n))
    ax.set_yticklabels(["%s g%d  %s" % (SHORT_ROUND[g["round"]], g["game_no"], g["map_title"]) for g in reversed(G)],
                       fontsize=9)
    ax.set_xlim(0, 260)
    ax.set_xlabel("longest dragon at the end of the game")
    ax.scatter([], [], s=58, color=US, label="Sabotage-d")
    ax.scatter([], [], s=58, color=THEM, label="opponent")
    ax.legend(loc="lower right", fontsize=9.5)
    longer = sum(1 for g in G if g["them"]["longest"] > g["us"]["longest"])
    ax.set_title("Against the top-16 seeds, their longest dragon was longer in %d of %d games" % (longer, len(G)), x=-0.2)
    save(fig, "longest-vs-top16.png")


def chart_ladder():
    t, e = [], []
    with open(os.path.join(DATA, "ladder_rating_hourly.csv")) as f:
        for row in csv.reader(line for line in f if not line.startswith("#")):
            if row[0] == "hour_utc":
                continue
            when = datetime.strptime(row[0], "%Y-%m-%dT%H:%MZ")
            if when < datetime(2026, 10, 2):
                continue
            t.append(when)
            e.append(int(row[1]))
    fig, ax = plt.subplots(figsize=(11, 4.2))
    tidy(ax)
    seg_t, seg_e = [t[0]], [e[0]]
    for i in range(1, len(t)):
        if (t[i] - t[i - 1]).total_seconds() > 3 * 3600:
            ax.plot(seg_t, seg_e, color=US, lw=2)
            seg_t, seg_e = [], []
        seg_t.append(t[i])
        seg_e.append(e[i])
    ax.plot(seg_t, seg_e, color=US, lw=2)
    marks = [(datetime(2026, 10, 9, 0, 59), 2334, "peak 2334", (-62, 4)),
             (datetime(2026, 10, 9, 9, 0), 2252, "2252 at seeding\n(seed 18 of 280)", (10, 34))]
    for when, val, text, off in marks:
        ax.scatter([when], [val], s=40, color=US, zorder=3, edgecolors=SURFACE, linewidths=2)
        ax.annotate(text, xy=(when, val), xytext=off, textcoords="offset points", fontsize=9, color=INK2)
    for when, text in [(datetime(2026, 10, 9, 6, 0), "submissions freeze"),
                       (datetime(2026, 10, 10, 7, 0), "Qualifier")]:
        ax.axvline(when, color=MUTED, lw=1)
        ax.text(when, 1830, " " + text, fontsize=8.6, color=INK2, rotation=90, va="bottom", ha="right")
    ax.set_ylim(1800, 2400)
    ax.set_ylabel("ladder rating (Elo)")
    ax.set_title("Our ladder rating, 2 to 14 Oct 2026 (UTC)")
    import matplotlib.dates as mdates
    ax.xaxis.set_major_locator(mdates.DayLocator(interval=2))
    ax.xaxis.set_major_formatter(mdates.DateFormatter("%d Oct"))
    save(fig, "ladder-rating.png")


def chart_queen():
    G = games()
    top = [g for g in G if g["round"] in ("Round of 32", "Round of 16", "Second chance, quarterfinal")]
    fig, (a1, a2) = plt.subplots(1, 2, figsize=(11, 3.8), gridspec_kw={"width_ratios": [1, 1.15], "wspace": 0.35})
    tidy(a1)
    tidy(a2)
    # local A/B test: data/queen_ab_test.csv
    with open(os.path.join(DATA, "queen_ab_test.csv")) as f:
        ab = list(csv.DictReader(line for line in f if not line.startswith("#")))
    n_ab = sum(int(r["games"]) for r in ab)
    tot = lambda k: sum(int(r[k]) for r in ab)  # noqa: E731
    vals = [("queen alive at\nthe end of the game", tot("previous_alive"), tot("smart_queen_alive")),
            ("games won", tot("previous_wins"), tot("smart_queen_wins"))]
    for i, (lab, old, new) in enumerate(vals):
        a1.bar(i - 0.17, old, width=0.3, color=GRAY)
        a1.bar(i + 0.17, new, width=0.3, color=US)
        a1.text(i - 0.17, old + 1.5, str(old), ha="center", fontsize=9.5)
        a1.text(i + 0.17, new + 1.5, str(new), ha="center", fontsize=9.5)
    a1.set_xticks(range(len(vals)))
    a1.set_xticklabels([v[0] for v in vals])
    a1.set_ylim(0, 82)
    a1.set_title("Local A/B test, %d paired games" % n_ab, fontsize=11)
    a1.legend([plt.Rectangle((0, 0), 1, 1, color=c) for c in (GRAY, US)], ["ladder bot of 8 Oct", "smart-queen bot"],
              fontsize=9, loc="upper left")
    groups = [("all 42 Qualifier games", G), ("21 games vs top-16 seeds", top)]
    for i, (lab, gs) in enumerate(groups):
        ours = sum(g["our_queen"]["alive"] for g in gs)
        theirs = sum(g["their_queen"]["alive"] for g in gs)
        a2.bar(i - 0.17, ours, width=0.3, color=US)
        a2.bar(i + 0.17, theirs, width=0.3, color=THEM)
        a2.text(i - 0.17, ours + 0.5, "%d of %d" % (ours, len(gs)), ha="center", fontsize=9.5)
        a2.text(i + 0.17, theirs + 0.5, "%d of %d" % (theirs, len(gs)), ha="center", fontsize=9.5)
    a2.set_xticks(range(len(groups)))
    a2.set_xticklabels([g[0] for g in groups])
    a2.set_ylim(0, 24)
    a2.set_title("Qualifier: queen alive at the end of the game", fontsize=11)
    a2.legend([plt.Rectangle((0, 0), 1, 1, color=c) for c in (US, THEM)], ["Sabotage-d", "opponents"],
              fontsize=9, loc="upper right")
    fig.suptitle("A safer queen", x=0.01, ha="left", fontweight="bold", fontsize=13, y=1.03)
    save(fig, "queen-safety.png")


if __name__ == "__main__":
    chart_games_grid()
    chart_decided_by()
    chart_game7()
    chart_longest_top16()
    chart_ladder()
    chart_queen()
