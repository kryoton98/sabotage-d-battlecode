"""Rebuild the full game state from a .replay, turn by turn, and re-create the exact
input block every dragon was given. Checked turn by turn against games played on the local engine.

    from rstate import Game
    g = Game(load(path))
    for tv in g.turns():          # one TurnView per dragon turn, state = what that dragon saw
        tv.rnd, tv.did, tv.team, tv.length, tv.kind, tv.arg, tv.block()   ...
"""
from __future__ import annotations

from replay import load  # noqa: F401  (re-export)

DX = {"N": 0, "E": 1, "S": 0, "W": -1}
DY = {"N": -1, "E": 0, "S": 1, "W": 0}


class GameMap:
    def __init__(self, text):
        self.tiles = {}
        self.edges = {}
        self.start = []
        self.name = "?"
        self.sym = "?"
        for line in text.split("\n"):
            p = line.split()
            if not p:
                continue
            k = p[0]
            if k == "MAP":
                self.W, self.H = int(p[1]), int(p[2])
            elif k == "MAP_NAME":
                self.name = " ".join(p[1:])
            elif k == "SYMMETRY":
                self.sym = p[1]
            elif k == "TILE":
                self.tiles[(int(p[1]), int(p[2]))] = (int(p[3]), int(p[4]))
            elif k == "EDGE":
                self.edges[int(p[1])] = (int(p[2]), int(p[3]))
            elif k == "DRAGON":
                n = int(p[2])
                c = list(map(int, p[3:3 + 2 * n]))
                self.start.append((int(p[1]), [(c[i], c[i + 1]) for i in range(0, 2 * n, 2)]))
        W = self.W
        self.portal = {}
        by_id = {}
        for idx, (kind, pid) in self.edges.items():
            if kind == 2:
                by_id.setdefault(pid, []).append(idx)
        for pid, idxs in by_id.items():
            if len(idxs) == 2:
                self.portal[idxs[0]] = idxs[1]
                self.portal[idxs[1]] = idxs[0]

    def edge_index(self, x, y, d):
        """index of the edge on side d of tile (x, y)"""
        W, H = self.W, self.H
        if d == "N":
            return (2 * y) * (W + 1) + x
        if d == "S":
            return (2 * ((y + 1) % H)) * (W + 1) + x
        if d == "W":
            return (2 * y + 1) * (W + 1) + x
        return (2 * y + 1) * (W + 1) + (x + 1) % W

    def edge(self, x, y, d):
        return self.edges.get(self.edge_index(x, y, d), (0, -1))

    def dir_between(self, a, b):
        """direction of the single step a -> b, ignoring portals; None if not adjacent"""
        for d in "NESW":
            if ((a[0] + DX[d]) % self.W, (a[1] + DY[d]) % self.H) == b:
                return d
        return None

    def seg_dirs(self, body, facing, fallback=None):
        out = [facing]
        for k in range(1, len(body)):
            d = self.dir_between(body[k], body[k - 1])
            if d is None:
                d = fallback[k] if fallback and k < len(fallback) else facing
            out.append(d)
        return out

    def step(self, x, y, d):
        """tile reached by moving d from (x, y); None if kelp"""
        W, H = self.W, self.H
        idx = self.edge_index(x, y, d)
        kind, pid = self.edges.get(idx, (0, -1))
        if kind == 1:
            return None
        if kind == 2 and idx in self.portal:
            j = self.portal[idx]
            r, c = divmod(j, W + 1)
            if r % 2 == 0:            # horizontal edge = north edge of tile (c, r//2)
                ty = r // 2
                return (c, (ty - 1) % H) if d == "N" else (c, ty)
            ty = r // 2               # vertical edge = west edge of tile (c, ty)
            return ((c - 1) % W, ty) if d == "W" else (c, ty)
        return ((x + DX[d]) % W, (y + DY[d]) % H)


class Dragon:
    __slots__ = ("id", "team", "body", "facing", "born", "parent", "dirs")

    def __init__(self, did, team, body, facing, born=0, parent=None, dirs=None):
        self.id, self.team, self.body, self.facing, self.born, self.parent = did, team, body, facing, born, parent
        self.dirs = dirs          # dirs[k] = direction segment k travels next (dirs[0] = facing)


class TurnView:
    """State at the moment dragon `did` is asked for its action, plus what it answered."""
    __slots__ = ("g", "rnd", "did", "team", "length", "kind", "arg", "died", "reason", "ate", "victims", "ei")

    def block(self):
        return self.g.block(self.did)


class Game:
    def __init__(self, rp, check=True, sprint_div=None):
        self.rp = rp
        self.map = GameMap(rp.map_text)
        self.check = check
        self.sprint_div = sprint_div      # (2 Oct) unswbc 1.2+: the first ceil(L / sprint_div) sprint steps are free
        self.problems = []
        self.reset()

    def reset(self):
        m = self.map
        self.pearls = set()
        self.cd = {}              # tile -> (n, round when set)
        self.dragons = {}
        self.all = {}             # every dragon ever (id -> Dragon), for team lookups after death
        self.rnd = 0
        self.deaths = []          # (round, id, team, reason, length, killer_id or None)
        for k, (did, body) in enumerate(m.start):
            pass

    # ------------------------------------------------------------------ replay
    def turns(self):
        """Generator of TurnView. The Game object holds the state *before* the action while
        the consumer looks at the view; the action is applied when the generator resumes."""
        self.reset()
        m = self.map
        ev = self.rp.events
        n = len(ev)
        i = 0
        started = False
        team_of_start = {}
        for k, (tid, body) in enumerate(m.start):
            team_of_start[k] = "AB"[tid] if tid in (0, 1) else "?"
        start_bodies = {k: list(body) for k, (tid, body) in enumerate(m.start)}
        while i < n:
            e = ev[i]
            t = e["t"]
            if t == "countdown":
                self.cd[e["tile"]] = (e["n"], self.rnd if started else -1)
            elif t == "tile":
                if e["pearl"]:
                    self.pearls.add(e["tile"])
                else:
                    self.pearls.discard(e["tile"])
            elif t == "round":
                self.rnd = e["round"]
                started = True
            elif t == "update":
                d = self.dragons.get(e["id"])
                if d is None and not started:
                    did = e["id"]
                    body = start_bodies.get(did)
                    if body is None or body[0] != e["head"]:
                        # find the start body whose head matches
                        for k, b in start_bodies.items():
                            if b[0] == e["head"]:
                                body = b
                                break
                    team = None
                    for k, (tid, b) in enumerate(m.start):
                        if b == body:
                            team = "AB"[tid]
                    d = Dragon(did, team, list(body), e["facing"])
                    d.dirs = m.seg_dirs(d.body, d.facing)
                    self.dragons[did] = d
                    self.all[did] = d
                elif d is not None and d.id not in self._dying:
                    d.facing = e["facing"]
                    j = i + 1
                    final = True
                    while j < n and ev[j]["t"] not in ("turn", "round"):
                        if ev[j]["t"] == "update" and ev[j]["id"] == d.id:
                            final = False
                            break
                        j += 1
                    if self.check and final and (d.body[0] != e["head"] or d.body[-1] != e["tail"]):
                        self.problems.append((self.rnd, d.id, "update mismatch", d.body[0], d.body[-1], e["head"],
                                              e["tail"]))
            elif t == "split":
                p = self.dragons.get(e["parent"])
                opp = {"N": "S", "S": "N", "E": "W", "W": "E"}
                rev = None
                if p is not None:
                    old_dirs = p.dirs
                    np_ = len(e["parent_body"])
                    # reversed rear part: child segment k was parent segment (L-1-k); it now travels the
                    # opposite way of what the segment behind it used to
                    L = len(p.body)
                    rev = [None] + [opp[old_dirs[L - k]] if 0 < L - k < min(L, len(old_dirs)) else None
                                    for k in range(1, L - np_)]
                    p.body = list(e["parent_body"])
                    p.dirs = old_dirs[:np_]
                c = Dragon(e["child"], e["team"], list(e["child_body"]), e["facing"], self.rnd, e["parent"])
                c.dirs = m.seg_dirs(c.body, c.facing, rev)
                self.dragons[c.id] = c
                self.all[c.id] = c
            elif t == "death":
                d = self.dragons.pop(e["id"], None)
                self.deaths.append((self.rnd, e["id"], d.team if d else "?", e["reason"], len(d.body) if d else 0,
                                    self._actor))
            elif t == "turn":
                did = e["id"]
                d = self.dragons.get(did)
                # the action is the next 'action' event for this id (always directly after)
                act = ev[i + 1] if i + 1 < n and ev[i + 1]["t"] == "action" else None
                tv = TurnView()
                tv.g = self
                tv.rnd = self.rnd
                tv.did = did
                tv.team = d.team if d else "?"
                tv.length = len(d.body) if d else 0
                tv.kind = act["kind"] if act else None
                tv.arg = act["arg"] if act else None
                # look ahead: did the actor die in this turn / who else died
                j = i + 2
                died = False
                reason = None
                victims = []
                while j < n and ev[j]["t"] not in ("turn", "round"):
                    if ev[j]["t"] == "death":
                        if ev[j]["id"] == did:
                            died = True
                            reason = ev[j]["reason"]
                        else:
                            victims.append(ev[j]["id"])
                    j += 1
                tv.died, tv.reason, tv.victims = died, reason, victims
                tv.ei = i
                self._actor = did
                self._dying = set(victims) | ({did} if died else set())
                yield tv
            elif t == "action":
                j = i + 1
                eaten = 0
                while j < n and ev[j]["t"] not in ("turn", "round"):
                    if ev[j]["t"] == "tile" and not ev[j]["pearl"]:
                        eaten += 1
                    j += 1
                self._apply_action(e, eaten)
            i += 1

    _actor = None
    _dying = frozenset()

    def _apply_action(self, e, eaten):
        d = self.dragons.get(e["id"])
        if d is None or e["kind"] != "move":
            return
        m = self.map
        steps = e["arg"]
        x, y = d.body[0]
        body = list(d.body)
        dirs = list(d.dirs)
        for s in steps:
            nxt = m.step(x, y, s)
            if nxt is None:
                break                     # died on kelp; a death event follows
            x, y = nxt
            dirs[0] = s
            body.insert(0, nxt)
            dirs.insert(0, s)
        if self.sprint_div:
            L0 = len(d.body)
            paid = len(steps) - (L0 + self.sprint_div - 1) // self.sprint_div
            if paid < 0:
                paid = 0
            new_len = L0 + eaten - paid
        else:
            new_len = len(d.body) + eaten - (len(steps) - 1)
        if new_len < 1:
            new_len = 1
        d.body = body[:new_len]
        d.dirs = dirs[:new_len]
        d.facing = steps[-1] if steps else d.facing

    # ------------------------------------------------------------------ observation
    def pearl_in(self, tile):
        c = self.cd.get(tile)
        if c is None:
            return -1
        n, r0 = c
        if n < 0:
            return -1
        return n - (self.rnd - r0)

    def block(self, did, msgs=()):
        m = self.map
        W, H = m.W, m.H
        d = self.dragons[did]
        hx, hy = d.body[0]
        team_count = sum(1 for o in self.dragons.values() if o.team == d.team)
        out = ["ROUND %d" % self.rnd, "DIR " + d.facing, "LENGTH %d" % len(d.body),
               "UNIT_COUNT %d" % team_count, "NUM_MSGS %d" % len(msgs)]
        out.extend(msgs)
        win = {}
        for j in range(7):
            y = (hy - 3 + j) % H
            for i in range(7):
                x = (hx - 3 + i) % W
                win[(x, y)] = 1
                out.append("%d %d %d %d" % (x, y, 1 if (x, y) in self.pearls else 0, self.pearl_in((x, y))))
        segs = []
        for oid in sorted(self.dragons):
            o = self.dragons[oid]
            for k, (x, y) in enumerate(o.body):
                if (x, y) in win:
                    segs.append("%s %d %d %d %s %d" % (o.team, o.id, x, y, o.dirs[k], 1 if k == 0 else 0))
        out.append("DRAGON_BODIES %d" % len(segs))
        out.extend(segs)

        def sym(edge):
            kind, pid = edge
            return "." if kind == 0 else ("w" if kind == 1 else str(pid))

        for j in range(8):
            row = []
            if j < 7:
                y = (hy - 3 + j) % H
                for i in range(7):
                    row.append(sym(m.edge((hx - 3 + i) % W, y, "N")))
            else:
                y = (hy + 3) % H
                for i in range(7):
                    row.append(sym(m.edge((hx - 3 + i) % W, y, "S")))
            out.append(" ".join(row))
        for j in range(7):
            y = (hy - 3 + j) % H
            row = []
            for i in range(7):
                row.append(sym(m.edge((hx - 3 + i) % W, y, "W")))
            row.append(sym(m.edge((hx + 3) % W, y, "E")))
            out.append(" ".join(row))
        return out
