"""Derive an opt-in original music-clock replay from a pinned native track."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from extract_original_data import ROOT, pins

MANIFEST = ROOT/'manifest/gameplay-clock-fixture.sha256'


def inputs(include_keys=True, original_start_step=109):
    for name, digest in pins(ROOT/'manifest/gameplay-item16-exit-audio.sha256').items():
        assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'CLOCK FIXTURE / SOURCE PIN '+name
    blocks, block, clock = [], None, None
    for line in (ROOT/'amiga/.run-eNDhoJ/gdb-out.log').read_text().splitlines():
        match = re.match(r'ROUTE_AUDIO step=(\d+) selector=(\d+) result=(\w+)', line)
        if not match:
            continue
        step, selector, value = int(match[1]), int(match[2]), int(match[3], 16)
        if selector == 6:
            if block:
                blocks.append(block)
            block = dict(step=step, previous=clock, queries=[], notes=0)
        elif selector == 15:
            clock = value
            if block:
                block['queries'].append(clock)
        elif selector == 9 and block:
            block['notes'] += 1
    selected = [block for block in blocks if block['notes'] == 194]
    assert len(selected) == 1, 'CLOCK FIXTURE / TRACK IDENTITY'
    block = selected[0]
    assert block['step'] == 109 and len(block['queries']) == 1115, 'CLOCK FIXTURE / TRACK EXTENT'
    deltas = [clock-block['previous'] for clock in block['queries']]
    assert deltas[0] == 1 and deltas[-1] == 1116 and all(b >= a for a, b in zip(deltas, deltas[1:])), 'CLOCK FIXTURE / QUERY ORDER'
    assert all(value == index+int(index % 6 in (3, 4) or index in (5, len(deltas))) for index, value in enumerate(deltas, 1)), 'CLOCK FIXTURE / COMPACT RESPONSE FORMULA'
    assert original_start_step in (108, 109), 'CLOCK FIXTURE / TRIGGER VERSION'
    result = dict(original_start_step=original_start_step, native_start_step=109, notes=194, deltas=deltas)
    if include_keys:
        from gameplay_assisted_inputs import metadata
        keys, _, _, _ = metadata(ROOT/'ref/mame/runs/gameplay-level6-door-1', ROOT/'manifest/gameplay-assisted-level6-door.sha256')
        result['keys'] = [dict(frame=int(frame), step=int(step), map=keymap) for frame, step, keymap in keys]
    return result


def lua(data):
    result = 'return {original_start_step='+str(data['original_start_step'])+',deltas={'+','.join(str(v) for v in data['deltas'])+'}'
    if 'keys' in data:
        result += ',keys={'+','.join('{frame=%d,step=%d,map="%s"}' % (row['frame'], row['step'], row['map']) for row in data['keys'])+'}'
    return result+'}\n'


def verify(directory):
    recorded = pins(MANIFEST)
    for filename in ('metadata.json', 'metadata.lua'):
        path = directory.resolve()/filename
        key = str(path.relative_to(ROOT))
        assert key in recorded and hashlib.sha256(path.read_bytes()).hexdigest() == recorded[key], 'CLOCK FIXTURE / METADATA PIN'
    saved = json.loads((directory/'metadata.json').read_text())
    data = inputs(include_keys='keys' in saved, original_start_step=saved['original_start_step'])
    assert saved == data, 'CLOCK FIXTURE / DERIVATION'
    assert (directory/'metadata.lua').read_text() == lua(data), 'CLOCK FIXTURE / LUA DERIVATION'
    return data


def build(directory):
    directory = directory.resolve()
    assert directory.is_relative_to(ROOT/'tmp'), 'CLOCK FIXTURE / IGNORED OUTPUT'
    directory.mkdir(parents=True, exist_ok=True)
    data = inputs()
    (directory/'metadata.json').write_text(json.dumps(data, indent=2)+'\n')
    (directory/'metadata.lua').write_text(lua(data))
    recorded = pins(MANIFEST) if MANIFEST.exists() else {}
    for filename in ('metadata.json', 'metadata.lua'):
        path = directory/filename
        recorded[str(path.relative_to(ROOT))] = hashlib.sha256(path.read_bytes()).hexdigest()
    MANIFEST.write_text(''.join(f'{digest}  {name}\n' for name, digest in sorted(recorded.items())))
    verify(directory)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    (verify if args.verify else build)(args.directory)
