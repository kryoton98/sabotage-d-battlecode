"""Reader for UNSW Battlecode .replay files (packed Cap'n Proto, schema recovered from the viewer).

    from replay import load
    rp = load("M13053.replay")
    rp.map_text, rp.bot_a, rp.bot_b, rp.result, rp.events   # events = list of dicts

No dependencies.
"""
from __future__ import annotations

import struct
import sys

DIRS = "NESW"
TEAMS = "AB"
DEATH = ["wall", "self", "body", "head2head", "noaction"]
END = ["eliminated", "roundlimit"]


def unpack(buf: bytes) -> bytes:
    out = bytearray()
    i = 0
    n = len(buf)
    while i < n:
        tag = buf[i]
        i += 1
        if tag == 0:
            cnt = buf[i]
            i += 1
            out += b"\0" * (8 * (cnt + 1))
        elif tag == 0xFF:
            out += buf[i:i + 8]
            i += 8
            cnt = buf[i]
            i += 1
            out += buf[i:i + 8 * cnt]
            i += 8 * cnt
        else:
            word = bytearray(8)
            for b in range(8):
                if tag >> b & 1:
                    word[b] = buf[i]
                    i += 1
            out += word
    return bytes(out)


class Msg:
    def __init__(self, raw: bytes):
        (cnt,) = struct.unpack_from("<I", raw, 0)
        cnt += 1
        sizes = struct.unpack_from("<%dI" % cnt, raw, 4)
        off = 4 + 4 * cnt
        off = (off + 7) & ~7
        self.segs = []
        for s in sizes:
            self.segs.append(memoryview(raw)[off:off + 8 * s])
            off += 8 * s

    # pointer resolution -> (seg, word_offset_of_target, pointer_word) following far pointers
    def _resolve(self, seg, woff):
        (p,) = struct.unpack_from("<Q", self.segs[seg], woff * 8)
        if p == 0:
            return None
        kind = p & 3
        if kind == 2:
            double = p >> 2 & 1
            pad = (p >> 3) & 0x1FFFFFFF
            tseg = p >> 32
            if not double:
                return self._resolve_landing(tseg, pad)
            (far,) = struct.unpack_from("<Q", self.segs[tseg], pad * 8)
            (tagw,) = struct.unpack_from("<Q", self.segs[tseg], pad * 8 + 8)
            fseg = far >> 32
            fpos = (far >> 3) & 0x1FFFFFFF
            return (fseg, fpos, tagw)
        off = (p >> 2) & 0x3FFFFFFF
        if off & 0x20000000:
            off -= 0x40000000
        return (seg, woff + 1 + off, p)

    def _resolve_landing(self, seg, woff):
        (p,) = struct.unpack_from("<Q", self.segs[seg], woff * 8)
        off = (p >> 2) & 0x3FFFFFFF
        if off & 0x20000000:
            off -= 0x40000000
        return (seg, woff + 1 + off, p)

    def struct_at(self, seg, woff):
        r = self._resolve(seg, woff)
        if r is None:
            return None
        tseg, tpos, p = r
        dw = (p >> 32) & 0xFFFF
        pc = (p >> 48) & 0xFFFF
        return St(self, tseg, tpos, dw, pc)

    def list_at(self, seg, woff):
        r = self._resolve(seg, woff)
        if r is None:
            return None
        tseg, tpos, p = r
        esz = (p >> 32) & 7
        cnt = (p >> 35) & 0x1FFFFFFF
        return (tseg, tpos, esz, cnt)

    def root(self):
        return self.struct_at(0, 0)


class St:
    __slots__ = ("m", "seg", "pos", "dw", "pc")

    def __init__(self, m, seg, pos, dw, pc):
        self.m, self.seg, self.pos, self.dw, self.pc = m, seg, pos, dw, pc

    def _get(self, fmt, boff, size):
        if boff + size > self.dw * 8:
            return 0
        return struct.unpack_from(fmt, self.m.segs[self.seg], self.pos * 8 + boff)[0]

    def i32(self, boff):
        return self._get("<i", boff, 4)

    def u32(self, boff):
        return self._get("<I", boff, 4)

    def u64(self, boff):
        return self._get("<Q", boff, 8)

    def u16(self, boff):
        return self._get("<H", boff, 2)

    def u8(self, boff):
        return self._get("<B", boff, 1)

    def bit(self, bitoff):
        return (self.u8(bitoff // 8) >> (bitoff % 8)) & 1

    def ptr_struct(self, idx):
        if idx >= self.pc:
            return None
        return self.m.struct_at(self.seg, self.pos + self.dw + idx)

    def ptr_list(self, idx):
        if idx >= self.pc:
            return None
        return self.m.list_at(self.seg, self.pos + self.dw + idx)

    def text(self, idx):
        l = self.ptr_list(idx)
        if l is None:
            return ""
        seg, pos, esz, cnt = l
        return bytes(self.m.segs[seg][pos * 8: pos * 8 + max(0, cnt - 1)]).decode("utf-8", "replace")

    def struct_list(self, idx):
        l = self.ptr_list(idx)
        if l is None:
            return []
        seg, pos, esz, cnt = l
        out = []
        if esz == 7:
            (tag,) = struct.unpack_from("<Q", self.m.segs[seg], pos * 8)
            n = (tag >> 2) & 0x3FFFFFFF
            dw = (tag >> 32) & 0xFFFF
            pc = (tag >> 48) & 0xFFFF
            step = dw + pc
            base = pos + 1
            for k in range(n):
                out.append(St(self.m, seg, base + k * step, dw, pc))
        else:
            raise ValueError("non-composite struct list (esz=%d)" % esz)
        return out

    def u16_list(self, idx):
        l = self.ptr_list(idx)
        if l is None:
            return []
        seg, pos, esz, cnt = l
        if esz != 3:
            raise ValueError("expected 2-byte list, got esz=%d" % esz)
        return list(struct.unpack_from("<%dH" % cnt, self.m.segs[seg], pos * 8))


def _pt(s):
    return (s.i32(0), s.i32(4)) if s is not None else None


def _event(e):
    w = e.u16(0)
    b = e.ptr_struct(0)
    if w == 0:
        return {"t": "round", "round": b.i32(0)}
    if w == 1:
        return {"t": "turn", "id": b.i32(0)}
    if w == 2:
        return {"t": "countdown", "tile": _pt(b.ptr_struct(0)), "n": b.i32(0)}
    if w == 3:
        return {"t": "tile", "tile": _pt(b.ptr_struct(0)), "pearl": bool(b.bit(0))}
    if w == 4:
        a = b.ptr_struct(0)
        if a is None:                      # no action at all (a bot that gave none, or timed out)
            return {"t": "action", "id": b.i32(0), "kind": "none", "arg": None}
        k = a.u16(0)
        if k == 0:
            act = ("move", "".join(DIRS[d] for d in a.u16_list(0)))
        elif k == 1:
            act = ("split", a.i32(4))
        else:
            act = ("suicide", None)
        return {"t": "action", "id": b.i32(0), "kind": act[0], "arg": act[1]}
    if w == 5:
        return {"t": "englog", "id": b.i32(0), "text": b.text(0)}
    if w == 6:
        return {"t": "log", "id": b.i32(0), "text": b.text(0)}
    if w == 7:
        return {"t": "indicator", "id": b.i32(0), "text": b.text(0)}
    if w == 8:
        return {"t": "draw", "id": b.i32(0)}
    if w == 9:
        return {"t": "update", "id": b.i32(0), "facing": DIRS[b.u16(4)], "head": _pt(b.ptr_struct(0)),
                "tail": _pt(b.ptr_struct(1))}
    if w == 10:
        return {"t": "split", "parent": b.i32(0), "child": b.i32(4), "team": TEAMS[b.u16(8)],
                "facing": DIRS[b.u16(10)],
                "parent_body": [_pt(p) for p in b.struct_list(0)],
                "child_body": [_pt(p) for p in b.struct_list(1)]}
    if w == 11:
        return {"t": "death", "id": b.i32(0), "reason": DEATH[b.u16(4)]}
    if w == 12:
        # (unswbc 1.0.0) the value is a u64 in the third data word and the fourth says what the
        # beam stopped on (2 nothing, 3.. dragon parts); older replays keep a u32 in the second
        return {"t": "sonar", "id": b.i32(0), "dir": DIRS[b.u16(4)],
                "value": b.u64(16) if b.dw >= 3 else b.u32(8),
                "stop": b.u64(24) if b.dw >= 4 else None,
                "origin": _pt(b.ptr_struct(0)), "end": _pt(b.ptr_struct(1)),
                "hit": b.i32(12) if b.u16(6) == 1 else None}
    return {"t": "unknown", "which": w}


class Replay:
    pass


def load(path) -> Replay:
    raw = open(path, "rb").read()
    if raw[:2] == b"\x1f\x8b":            # the ladder's storage serves replays gzip-compressed
        import gzip
        raw = gzip.decompress(raw)
    msg = Msg(unpack(raw))
    root = msg.root()
    rp = Replay()
    rp.map_text = root.text(0)
    rp.bot_a = root.text(1)
    rp.bot_b = root.text(2)
    rp.events = [_event(e) for e in root.struct_list(3)]
    res = root.ptr_struct(4)
    ta, tb = res.ptr_struct(0), res.ptr_struct(1)

    def stand(s):
        return {"dragons": s.i32(0), "longest": s.i32(4), "total": s.i32(8)} if s is not None else None

    rp.result = {"terminated": bool(res.bit(0)), "end": END[res.u16(2)] if res.u16(2) < 2 else res.u16(2),
                 "winner": TEAMS[res.u16(6)] if res.u16(4) == 1 else None, "A": stand(ta), "B": stand(tb)}
    return rp


if __name__ == "__main__":
    from collections import Counter

    rp = load(sys.argv[1])
    print("botA:", rp.bot_a, "| botB:", rp.bot_b)
    print("result:", rp.result)
    print("events:", len(rp.events), Counter(e["t"] for e in rp.events).most_common())
    print("map header:", rp.map_text[:120].replace("\n", " | "))
    for e in rp.events[:40]:
        print(e)
