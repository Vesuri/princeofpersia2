"""Generate private native replay inputs from a hash-pinned original route."""
import argparse
from pathlib import Path
import re
from replay_gameplay_route import ROOT, verify_reference

OUTPUT = ROOT / 'build/gameplay/RouteInputs.h'
REFERENCES = {1: ROOT / 'ref/mame/runs/gameplay-level1-complete-1',
              2: ROOT / 'ref/mame/runs/gameplay-level2-fresh-1',
              3: ROOT / 'ref/mame/runs/gameplay-level3-fresh-1'}


def metadata(reference, level=1):
    assert level in REFERENCES, 'ROUTE INPUT / LEVEL'
    verify_reference(reference, level)
    text = (reference / 'session.log').read_text()
    rows = re.findall(r'^GAMEPLAY_ROUTE_KEYS frame=(\d+) step=(\d+) map=([0-9A-F]{32})$', text, re.M)
    steps = list(range(1002)) + (list(range(1001, 1361)) if level >= 2 else [])
    if level == 3:
        steps += list(range(1360, 2768))
    assert [(int(a), int(b)) for a, b, _ in rows] == [(v, v) for v in steps], 'ROUTE INPUT / STEP ORDER'
    checkpoints = []
    consumed = 0
    pattern = re.compile(r'^GAMEPLAY_ROUTE_CHECKPOINT sequence=(\d+) frames=(\d+) step=(\d+) level=(\d+) room=(\d+).* reason=([\w-]+) prefix=route-(\d+)$')
    for line in text.splitlines():
        if line.startswith('GAMEPLAY_ROUTE_KEYS '):
            consumed += 1
        match = pattern.match(line)
        if match and match[6] != 'discovery-finished':
            ordinal, frames, step, current, room = map(int, match.group(1, 2, 3, 4, 5))
            assert ordinal == len(checkpoints) == int(match[7]) and frames == step, 'ROUTE INPUT / CHECKPOINT ORDER'
            assert match[6] in ('first-frame', 'next-level', 'completed-frames'), 'ROUTE INPUT / CHECKPOINT REASON'
            checkpoints.append((step, current, room, consumed, match[6] != 'completed-frames'))
    assert len(checkpoints) == {1: 96, 2: 115, 3: 181}[level], 'ROUTE INPUT / CHECKPOINT COUNT'
    assert checkpoints[0] == (0, 1, 4, 0, True), 'ROUTE INPUT / FIRST FRAME'
    assert checkpoints[-1][:4] == {1: (1001, 2, 2, 1002), 2: (1360, 3, 1, 1362), 3: (2767, 4, 1, 2770)}[level], 'ROUTE INPUT / LAST FRAME'
    return rows, checkpoints


def key_rows(rows):
    virtual = (0x7b, 0x7c, 0x7e, 0x7d, 0x38, 0x3b, 0x31)
    result = []
    for _, step, raw in rows:
        keys = bytes.fromhex(raw)
        mask = sum(1 << i for i, key in enumerate(virtual) if keys[key // 8] & (1 << (key % 8)))
        known = bytearray(16)
        for i, key in enumerate(virtual):
            if mask & (1 << i): known[key // 8] |= 1 << (key % 8)
        assert bytes(known) == keys, 'ROUTE INPUT / UNMAPPED KEY'
        result.append('{%s,%d,{%s}},' % (step, mask, ','.join(str(v) for v in keys)))
    return result


def generate(reference, level=1):
    rows, checkpoints = metadata(reference, level)
    out = ['// PRIVATE: original consumed input; generated only in ignored build/.',
           '#ifndef POP2_GAMEPLAY_ROUTE_LEVEL', '#define POP2_GAMEPLAY_ROUTE_LEVEL 1', '#endif',
           f'#if POP2_GAMEPLAY_ROUTE_LEVEL != {level}', '#error Route input level differs from build', '#endif',
           'struct RouteKey { uint16_t step; uint8_t mask; uint8_t keys[16]; };',
           'static const RouteKey routeKeys[] = {']
    out += key_rows(rows)
    out += ['};', 'struct RouteCheckpoint { uint16_t step,level,room,keys; bool first,automatic; };',
            'static const RouteCheckpoint routeCheckpoints[] = {']
    out += ['{%d,%d,%d,%d,%s,false},' % (*row[:4], 'true' if row[4] else 'false') for row in checkpoints]
    out += ['};', f'static const char routeCompleteReason[] = "GAMEPLAY ROUTE / LEVEL {level} COMPLETE";']
    out += ['static const int routeEndingOrdinal = -1;']
    return '\n'.join(out) + '\n'


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--reference', type=Path)
    p.add_argument('--level', type=int, choices=(0, 1, 2, 3), default=1)
    p.add_argument('--manifest', type=Path)
    p.add_argument('--verify', action='store_true')
    a = p.parse_args()
    if a.level == 0:
        from gameplay_assisted_inputs import generate as assisted_generate, REFERENCE
        content = assisted_generate(a.reference or REFERENCE, **({'manifest':a.manifest} if a.manifest else {}))
    else:
        content = generate(a.reference or REFERENCES[a.level], a.level)
    if a.verify:
        assert OUTPUT.read_text() == content, 'ROUTE INPUT / GENERATED MISMATCH'
    else:
        OUTPUT.parent.mkdir(parents=True, exist_ok=True); OUTPUT.write_text(content)
    print(f'ROUTE INPUTS PASS: through level {a.level}; pinned key reads and checkpoints')
