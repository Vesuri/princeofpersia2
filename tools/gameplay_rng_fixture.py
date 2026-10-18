"""Prepare and verify an opt-in original RNG-stream causality fixture."""
import argparse
import hashlib
import json
from pathlib import Path

from check_gameplay_random_window import compare, rows
from extract_original_data import ROOT, pins

MANIFEST = ROOT/'manifest/gameplay-rng-fixture.sha256'


def actor_sources(mode):
    if mode == 2:
        return (ROOT/'ref/mame/runs/gameplay-level3-fresh-1', ROOT/'manifest/gameplay-level3-reference.sha256',
                ROOT/'amiga/.run-Vz4heJ', ROOT/'manifest/gameplay-level3-rejected-entry.sha256', 133, 134, 133, 134)
    assert mode in (3, 4, 5), 'RNG FIXTURE / MODE'
    return (ROOT/'ref/mame/runs/gameplay-assisted-level4-1', ROOT/'manifest/gameplay-assisted-level4.sha256',
            ROOT/'amiga/.run-0c0hOQ', ROOT/'manifest/gameplay-level4-native.sha256', 83, 84, 86, 87)


def actor_inputs(mode):
    reference, original_pins, native, native_pins, before, after, native_before, native_after = actor_sources(mode)
    for run, manifest in ((reference, original_pins), (native, native_pins)):
        for name, digest in pins(manifest).items():
            assert hashlib.sha256((run/name).read_bytes()).hexdigest() == digest, 'RNG FIXTURE / ACTOR INPUT PIN '+name
    original = (reference/f'route-{before}-a5.bin').read_bytes()
    replacement = (native/f'route-{native_before}-a5.bin').read_bytes()
    original_end = (reference/f'route-{after}-a5.bin').read_bytes()
    native_end = (native/f'route-{native_after}-a5.bin').read_bytes()
    actors = slice(-0x5208, -0x5208+372)
    assert original[actors] == replacement[actors], 'RNG FIXTURE / ACTORS ALREADY DIFFER'
    assert original_end[actors] != native_end[actors], 'RNG FIXTURE / ACTOR CONTROL MATCHES'
    seed = lambda a: int.from_bytes(a[-0x18fc:-0x18f8], 'big')
    step = lambda a: int.from_bytes(a[-0x349a:-0x3498], 'big')
    assert seed(original) != seed(replacement), 'RNG FIXTURE / SEEDS ALREADY MATCH'
    assert step(original) == step(replacement) and step(original_end) == step(native_end), 'RNG FIXTURE / FRAME PAIR'
    result = dict(mode=mode, inject_checkpoint=before, checkpoint=after,
                native_before=native_before, native_after=native_after,
                step=step(original), final_step=step(original_end),
                before=seed(original), after=seed(replacement))
    if mode in (4, 5):
        original_context = (reference/f'route-{before}-context.bin').read_bytes()
        native_context = (native/f'route-{native_before}-context.bin').read_bytes()
        assert len(original_context) == len(native_context) == 0x2200, 'RNG FIXTURE / CONTEXT EXTENT'
        differences = [(i, a, b) for i, (a, b) in enumerate(zip(original_context, native_context)) if a != b]
        assert len(differences) == 5, 'RNG FIXTURE / CONTEXT DIFFERENCES'
        result['context_count'] = len(differences)
        for i, (offset, old, new) in enumerate(differences):
            result.update({f'context_offset_{i}': offset, f'context_before_{i}': old, f'context_after_{i}': new})
    if mode == 5:
        runs = (ROOT/'ref/mame/runs/item16-level4-opponents-1', ROOT/'amiga/.run-KkYYZ2')
        for run, side in zip(runs, ('original', 'native')):
            for name, digest in pins(ROOT/f'manifest/gameplay-item16-opponents-{side}.sha256').items():
                assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'RNG FIXTURE / OPPONENT PIN '+name
        for checkpoint, ordinal in ((before, native_before), (after, native_after)):
            for suffix in ('-context.bin', '-a5.bin'):
                old = (native/f'route-{ordinal}{suffix}').read_bytes()
                new = (runs[1]/f'route-{ordinal}{suffix}').read_bytes()
                if suffix == '-context.bin':
                    assert old == new, 'RNG FIXTURE / REPEATED CONTEXT'
                else:
                    for distance, size in ((0x5208, 372), (0x18fc, 4), (0x349a, 2)):
                        assert old[-distance:-distance+size] == new[-distance:-distance+size], 'RNG FIXTURE / REPEATED ACTORS SEED STEP'
            assert (runs[0]/f'route-{checkpoint}-context.bin').read_bytes() == (reference/f'route-{checkpoint}-context.bin').read_bytes(), 'RNG FIXTURE / ORIGINAL CONTEXT REPEAT'
        a = (runs[0]/f'route-{before}-opponents.bin').read_bytes()
        b = (runs[1]/f'route-{native_before}-opponents.bin').read_bytes()
        assert len(a) == len(b) == 0x1800, 'RNG FIXTURE / OPPONENT EXTENT'
        differences = [(i, int.from_bytes(a[i:i+2], 'big'), int.from_bytes(b[i:i+2], 'big'))
                       for i in range(0, len(a), 2) if a[i:i+2] != b[i:i+2]]
        assert len(differences) == 5 and all((i % 192-2) % 38 == 30 for i, _, _ in differences), 'RNG FIXTURE / ONLY FIVE TIMERS'
        result['opponent_count'] = len(differences)
        for i, (offset, old, new) in enumerate(differences):
            result.update({f'opponent_offset_{i}': offset, f'opponent_before_{i}': old, f'opponent_after_{i}': new})
    return result


def inputs(mode=1):
    if mode != 1:
        return actor_inputs(mode)
    for name, digest in pins(ROOT/'manifest/gameplay-item16-random-window.sha256').items():
        assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'RNG FIXTURE / INPUT PIN '+name
    original = (ROOT/'ref/mame/runs/gameplay-level3-random-1/session.log').read_text()
    native = (ROOT/'amiga/.run-845iXe/gdb-out.log').read_text()
    index = compare(original, native)
    calls = rows(original)
    step, after, result, site, _ = calls[index]
    return dict(step=step, before=calls[index-1][1], after=after, result=result,
                site=site, caller_segment=5, caller_offset=0x50ba, checkpoint=121)


def lua(data):
    return 'return {'+', '.join(f'{key}=0x{value:x}' for key, value in sorted(data.items()))+'}\n'


def build(directory, mode=1):
    directory = directory.resolve()
    assert directory.is_relative_to(ROOT/'tmp'), 'RNG FIXTURE / IGNORED OUTPUT REQUIRED'
    directory.mkdir(parents=True, exist_ok=True)
    data = inputs(mode)
    paths = [directory/'metadata.json', directory/'metadata.lua']
    paths[0].write_text(json.dumps(data, indent=2, sort_keys=True)+'\n')
    paths[1].write_text(lua(data))
    recorded = pins(MANIFEST) if MANIFEST.exists() else {}
    for p in paths:
        recorded[str(p.relative_to(ROOT))] = hashlib.sha256(p.read_bytes()).hexdigest()
    MANIFEST.write_text(''.join(f'{digest}  {name}\n' for name, digest in sorted(recorded.items())))
    verify(directory)


def verify(directory):
    directory = directory.resolve()
    recorded = pins(MANIFEST)
    for filename in ('metadata.json', 'metadata.lua'):
        path = directory/filename
        name = str(path.relative_to(ROOT))
        assert name in recorded and hashlib.sha256(path.read_bytes()).hexdigest() == recorded[name], 'RNG FIXTURE / METADATA PIN'
    data = inputs(json.loads((directory/'metadata.json').read_text()).get('mode', 1))
    assert json.loads((directory/'metadata.json').read_text()) == data, 'RNG FIXTURE / DERIVATION'
    assert (directory/'metadata.lua').read_text() == lua(data), 'RNG FIXTURE / LUA DERIVATION'
    return data


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--mode', type=int, choices=(1, 2, 3, 4, 5), default=1,
                        help='1: ambient draw; 2: level-three actor; 3: level-four seed only; 4: level-four seed and preceding context differences; 5: also align five persistent skeleton timers')
    args = parser.parse_args()
    if args.verify:
        verify(args.directory)
    else:
        build(args.directory, args.mode)
