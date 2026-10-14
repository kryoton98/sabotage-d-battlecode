#!/usr/bin/env python3
"""Draw UNSW Battlecode 2026 replays as PNG frames or animated GIFs.

    python tools/render_replay.py game.replay --round 480 -o frame.png
    python tools/render_replay.py game.replay --gif 440:500:2 -o ending.gif
    python tools/render_replay.py game.replay --round 150 --view 10,30,24,20 --scale 24 -o closeup.png
    python tools/render_replay.py game.replay --list          # one line per round, to pick frames

Our team is always blue and the other team orange (--us A|B says which side we were).
A gold ring marks each team's queen, a white cross marks a dragon that died in the last few rounds.
Needs Pillow; reads replays with replay.py / rstate.py from this folder.
"""
from __future__ import annotations

import argparse
import os
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from replay import load  # noqa: E402
from rstate import Game  # noqa: E402

# colours (RGB)
WATER = (13, 22, 38)
GRID = (21, 33, 54)
KELP = (70, 168, 98)
PORTAL = (196, 128, 255)
PEARL = (255, 214, 102)
US = (57, 135, 229)          # blue
THEM = (235, 104, 52)        # orange
QUEEN = (255, 215, 0)
DEATH = (245, 245, 245)
TEXT = (240, 239, 236)
TEXT2 = (176, 182, 196)
PANEL = (9, 15, 27)

FONT_DIR = "/usr/share/fonts/truetype/dejavu"


def font(size, bold=False):
    name = "DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf"
    for path in (os.path.join(FONT_DIR, name), name):
        try:
            return ImageFont.truetype(path, size)
        except OSError:
            pass
    return ImageFont.load_default()


def lighter(c, k=0.45):
    return tuple(int(v + (255 - v) * k) for v in c)


def darker(c, k=0.45):
    return tuple(int(v * (1 - k)) for v in c)


class Snap:
    """The board at the start of one round."""
    __slots__ = ("rnd", "dragons", "pearls", "deaths", "queens")

    def __init__(self, rnd, dragons, pearls, deaths, queens):
        self.rnd = rnd
        self.dragons = dragons      # list of (id, team, body)
        self.pearls = pearls        # set of (x, y)
        self.deaths = deaths        # list of (round, team, (x, y), reason)
        self.queens = queens        # team -> queen id

    def team_stats(self, team):
        lens = [len(b) for (i, t, b) in self.dragons if t == team]
        qid = self.queens.get(team)
        qlen = next((len(b) for (i, t, b) in self.dragons if i == qid), 0)
        return {"dragons": len(lens), "longest": max(lens) if lens else 0, "total": sum(lens), "queen": qlen}


def snapshots(path, want=None, sprint_div=4, keep_deaths=4):
    """Return (game, {round: Snap}) for the rounds in `want` (all rounds if None) plus the final board
    under key 'end'. Death crosses stay on the board for `keep_deaths` rounds."""
    rp = load(path)
    g = Game(rp, check=False, sprint_div=sprint_div)
    want = None if want is None else set(want)
    snaps = {}
    deaths = []
    cur = None
    queens = None

    def take(rnd):
        recent = [d for d in deaths if rnd - keep_deaths < d[0] <= rnd]
        return Snap(rnd, [(d.id, d.team, list(d.body)) for d in g.dragons.values()], set(g.pearls), recent, queens)

    for tv in g.turns():
        if queens is None:
            queens = {g.all[i].team: i for i in (0, 1) if i in g.all}   # the queens are dragons 0 and 1
        if tv.rnd != cur:
            cur = tv.rnd
            if want is None or cur in want:
                snaps[cur] = take(cur)
        if tv.died or tv.victims:                     # remember where dragons die, for the crosses
            actor = g.dragons.get(tv.did)
            if tv.died and actor is not None:
                deaths.append((tv.rnd, actor.team, actor.body[0], tv.reason))
            for vid in tv.victims:
                v = g.dragons.get(vid)
                if v is not None:
                    deaths.append((tv.rnd, v.team, v.body[0], "hit"))
    snaps["end"] = take(cur + 1 if cur is not None else 0)
    snaps["end"].rnd = cur + 1 if cur is not None else 0
    return g, snaps


class Painter:
    def __init__(self, gmap, scale=None, view=None, us="A", names=("A", "B"), title="", width=720):
        self.m = gmap
        W, H = gmap.W, gmap.H
        self.view = view or (0, 0, W, H)                # x0, y0, w, h (wraps around the torus)
        vw, vh = self.view[2], self.view[3]
        self.s = scale or max(5, min(24, width // vw))
        self.us = us
        self.names = {"A": names[0], "B": names[1]}
        self.title = title
        self.header = 84
        self.pad = 12
        self.footer = 30
        self.img_w = max(vw * self.s + 2 * self.pad, 460)
        self.img_h = vh * self.s + self.header + self.footer
        self.f_title = font(17, bold=True)
        self.f_text = font(13)
        self.f_small = font(11)
        self._base = None
        # wide enough for the longest header line
        probe = ImageDraw.Draw(Image.new("RGB", (8, 8)))
        sample = "64 dragons · longest 888 · total 8888 · queen dead"
        need = max(probe.textlength(n, font=self.f_text) for n in self.names.values())
        need = int(self.pad + 18 + need + 10 + probe.textlength(sample, font=self.f_small) + self.pad)
        self.img_w = max(self.img_w, need)

    def color(self, team):
        return US if team == self.us else THEM

    def to_view(self, x, y):
        x0, y0, vw, vh = self.view
        vx, vy = (x - x0) % self.m.W, (y - y0) % self.m.H
        if vx >= vw or vy >= vh:
            return None
        return vx, vy

    def px(self, vx, vy):
        return self.pad + vx * self.s, self.header + vy * self.s

    def center(self, vx, vy):
        x, y = self.px(vx, vy)
        return x + self.s / 2, y + self.s / 2

    def base(self):
        """The map without anything that moves: water, faint grid, kelp and portals."""
        if self._base is not None:
            return self._base.copy()
        s = self.s
        img = Image.new("RGB", (self.img_w, self.img_h), PANEL)
        d = ImageDraw.Draw(img)
        x0, y0, vw, vh = self.view
        d.rectangle([self.pad, self.header, self.pad + vw * s - 1, self.header + vh * s - 1], fill=WATER)
        if s >= 8:
            for i in range(1, vw):
                X = self.pad + i * s
                d.line([(X, self.header), (X, self.header + vh * s - 1)], fill=GRID)
            for j in range(1, vh):
                Y = self.header + j * s
                d.line([(self.pad, Y), (self.pad + vw * s - 1, Y)], fill=GRID)
        kw = max(2, s // 4)
        pw = max(2, s // 3)
        for vy in range(vh):
            for vx in range(vw):
                x, y = (x0 + vx) % self.m.W, (y0 + vy) % self.m.H
                X, Y = self.px(vx, vy)
                for side in ("N", "W"):
                    kind, _pid = self.m.edge(x, y, side)
                    if kind == 0:
                        continue
                    col, w = (KELP, kw) if kind == 1 else (PORTAL, pw)
                    if side == "N":
                        d.line([(X, Y), (X + s, Y)], fill=col, width=w)
                    else:
                        d.line([(X, Y), (X, Y + s)], fill=col, width=w)
                # the east / south edges of the last column / row of a partial view
                if vx == vw - 1 and vw < self.m.W:
                    kind, _ = self.m.edge(x, y, "E")
                    if kind:
                        d.line([(X + s, Y), (X + s, Y + s)], fill=KELP if kind == 1 else PORTAL, width=kw)
                if vy == vh - 1 and vh < self.m.H:
                    kind, _ = self.m.edge(x, y, "S")
                    if kind:
                        d.line([(X, Y + s), (X + s, Y + s)], fill=KELP if kind == 1 else PORTAL, width=kw)
        self._base = img
        return img.copy()

    def draw(self, snap, caption=None, marks=()):
        img = self.base()
        d = ImageDraw.Draw(img)
        s = self.s
        # pearls
        r = max(1.5, s * 0.2)
        for (x, y) in snap.pearls:
            v = self.to_view(x, y)
            if v:
                cx, cy = self.center(*v)
                d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=PEARL)
        # dragons: the other team first, ours on top; queens last
        order = sorted(snap.dragons, key=lambda t: (t[1] == self.us, t[0] in snap.queens.values()))
        bw = max(2, int(s * 0.62))
        for did, team, body in order:
            col = self.color(team)
            runs, run, prev = [], [], None
            for (x, y) in body:
                v = self.to_view(x, y)
                if v is None:
                    if run:
                        runs.append(run)
                    run, prev = [], None
                    continue
                if prev is not None and abs(v[0] - prev[0]) + abs(v[1] - prev[1]) != 1:
                    runs.append(run)
                    run = []
                run.append(self.center(*v))
                prev = v
            if run:
                runs.append(run)
            for rr in runs:
                if len(rr) > 1:
                    d.line(rr, fill=col, width=bw, joint="curve")
                    for (cx, cy) in rr:
                        d.ellipse([cx - bw / 2, cy - bw / 2, cx + bw / 2, cy + bw / 2], fill=col)
                else:
                    cx, cy = rr[0]
                    d.ellipse([cx - bw / 2, cy - bw / 2, cx + bw / 2, cy + bw / 2], fill=col)
            hv = self.to_view(*body[0])
            if hv:
                cx, cy = self.center(*hv)
                hr = max(2.5, s * 0.46)
                d.ellipse([cx - hr, cy - hr, cx + hr, cy + hr], fill=lighter(col, 0.35), outline=darker(col, 0.5),
                          width=max(1, s // 10))
                if did in snap.queens.values():
                    qr = max(4.0, s * 0.85)
                    d.ellipse([cx - qr, cy - qr, cx + qr, cy + qr], outline=QUEEN, width=max(2, s // 6))
        # recent deaths
        cr = max(3, s * 0.42)
        for (rnd, team, pos, why) in snap.deaths:
            if pos is None:
                continue
            v = self.to_view(*pos)
            if v:
                cx, cy = self.center(*v)
                w = max(2, s // 6)
                d.line([(cx - cr, cy - cr), (cx + cr, cy + cr)], fill=DEATH, width=w)
                d.line([(cx - cr, cy + cr), (cx + cr, cy - cr)], fill=DEATH, width=w)
        # extra rings, for pointing at something in a write-up
        for (x, y, rad) in marks:
            v = self.to_view(x, y)
            if v:
                cx, cy = self.center(*v)
                rr = rad * s
                d.ellipse([cx - rr, cy - rr, cx + rr, cy + rr], outline=(255, 255, 255), width=max(2, s // 5))
        self.header_text(d, snap, caption)
        return img

    def header_text(self, d, snap, caption):
        rnd = snap.rnd
        title = self.title or self.m.name
        line1 = "%s  ·  round %d" % (title, min(rnd, 500))
        d.text((self.pad, 8), line1, font=self.f_title, fill=TEXT)
        if caption:
            tw = d.textlength(caption, font=self.f_small)
            d.text((self.img_w - self.pad - tw, 12), caption, font=self.f_small, fill=TEXT2)
        y = 36
        for team in (self.us, "B" if self.us == "A" else "A"):
            st = snap.team_stats(team)
            x = self.pad
            d.rounded_rectangle([x, y + 3, x + 11, y + 14], radius=2, fill=self.color(team))
            x += 18
            name = self.names[team]
            d.text((x, y), name, font=self.f_text, fill=TEXT)
            x += d.textlength(name, font=self.f_text) + 10
            q = ("queen %d" % st["queen"]) if st["queen"] else "queen dead"
            info = "%d dragons · longest %d · total %d · %s" % (st["dragons"], st["longest"], st["total"], q)
            d.text((x, y + 1), info, font=self.f_small, fill=TEXT2)
            y += 21
        self.legend(d)

    def legend(self, d):
        y = self.img_h - self.footer + 9
        x = self.pad
        f = self.f_small

        def label(text):
            nonlocal x
            d.text((x, y - 1), text, font=f, fill=TEXT2)
            x += d.textlength(text, font=f) + 16

        d.ellipse([x, y, x + 12, y + 12], outline=QUEEN, width=2)
        x += 17
        label("queen")
        d.line([(x, y + 1), (x + 10, y + 11)], fill=DEATH, width=2)
        d.line([(x, y + 11), (x + 10, y + 1)], fill=DEATH, width=2)
        x += 15
        label("died")
        d.ellipse([x + 3, y + 3, x + 9, y + 9], fill=PEARL)
        x += 15
        label("pearl")
        d.line([(x, y + 6), (x + 14, y + 6)], fill=KELP, width=3)
        x += 19
        label("kelp")
        d.line([(x, y + 6), (x + 14, y + 6)], fill=PORTAL, width=4)
        x += 19
        label("portal")


def parse_view(text):
    if not text:
        return None
    a = [int(v) for v in text.split(",")]
    if len(a) != 4:
        raise SystemExit("--view needs x0,y0,width,height")
    return tuple(a)


def save_gif(frames, path, ms=110, hold=1600):
    pal = [f.convert("P", palette=Image.ADAPTIVE, colors=64) for f in frames]
    durations = [ms] * len(pal)
    durations[-1] = hold
    pal[0].save(path, save_all=True, append_images=pal[1:], duration=durations, loop=0, optimize=True, disposal=1)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("replay")
    ap.add_argument("-o", "--out", default=None, help="output .png or .gif")
    ap.add_argument("--round", type=int, action="append", help="round to draw (repeat for several PNGs)")
    ap.add_argument("--end", action="store_true", help="draw the final board")
    ap.add_argument("--gif", default=None, help="first:last:step rounds for an animated GIF (ends on the final board "
                    "when last reaches the end of the game)")
    ap.add_argument("--ms", type=int, default=110, help="GIF frame time in ms")
    ap.add_argument("--view", default=None, help="x0,y0,width,height: draw part of the map (wraps)")
    ap.add_argument("--scale", type=int, default=None, help="pixels per tile")
    ap.add_argument("--width", type=int, default=720, help="target map width in pixels when --scale is not set")
    ap.add_argument("--us", default="A", choices=["A", "B"], help="which side was ours (drawn blue)")
    ap.add_argument("--names", default="A,B", help="team names for sides A,B")
    ap.add_argument("--title", default="", help="title shown before the round number")
    ap.add_argument("--caption", default=None, help="small text at the top right")
    ap.add_argument("--mark", action="append", default=[], help="x,y[,radius]: draw a white ring there")
    ap.add_argument("--list", action="store_true", help="print one summary line per round")
    a = ap.parse_args(argv)

    want = None
    if a.gif:
        lo, hi, st = (int(v) for v in a.gif.split(":"))
        want = list(range(lo, hi + 1, st))
    elif a.round:
        want = a.round
    elif not a.list:
        a.end = True
        want = []
    g, snaps = snapshots(a.replay, want if not a.list else None)
    if a.list:
        for k in sorted(k for k in snaps if k != "end"):
            sn = snaps[k]
            sa, sb = sn.team_stats("A"), sn.team_stats("B")
            print("r%3d  A: %2d dragons longest %3d total %3d queen %3d   B: %2d dragons longest %3d total %3d queen %3d"
                  % (k, sa["dragons"], sa["longest"], sa["total"], sa["queen"],
                     sb["dragons"], sb["longest"], sb["total"], sb["queen"]))
        return
    names = a.names.split(",", 1) if "," in a.names else (a.names, "B")
    marks = []
    for mk in a.mark:
        p = [float(v) for v in mk.split(",")]
        marks.append((int(p[0]), int(p[1]), p[2] if len(p) > 2 else 1.6))
    painter = Painter(g.map, scale=a.scale, view=parse_view(a.view), us=a.us, names=names, title=a.title,
                      width=a.width)
    if a.gif:
        frames = [painter.draw(snaps[r], a.caption, marks) for r in want if r in snaps]
        if not frames:
            raise SystemExit("no rounds in range")
        if hi >= snaps["end"].rnd - 1:                     # the range reaches the last round: end on the final board
            frames.append(painter.draw(snaps["end"], a.caption, marks))
        out = a.out or "replay.gif"
        save_gif(frames, out, ms=a.ms)
        print("wrote", out, len(frames), "frames")
        return
    keys = (["end"] if a.end else []) + [r for r in (a.round or []) if r in snaps]
    for k in keys:
        img = painter.draw(snaps[k], a.caption, marks)
        out = a.out or "round_%s.png" % k
        if len(keys) > 1:
            root, ext = os.path.splitext(out)
            out = "%s_%s%s" % (root, k, ext or ".png")
        img.save(out, optimize=True)
        print("wrote", out)


if __name__ == "__main__":
    main()
