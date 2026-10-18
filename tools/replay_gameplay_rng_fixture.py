"""Replay through the first level-three RNG divergence and compare its causal control."""
import argparse
import hashlib
import json
from pathlib import Path

from extract_original_data import ROOT, pins
from gameplay_rng_fixture import verify, actor_sources
from replay_gameplay_route import compare, verify_reference, wait
from check_gameplay_restart import paired_frame
from gameplay_assist import assist


def replay(run, fixture):
    data = verify(fixture)
    mode = data.get('mode', 1)
    if mode == 1:
        reference = ROOT/'ref/mame/runs/gameplay-level3-fresh-1'
        native = ROOT/'amiga/.run-845iXe'
        verify_reference(reference, 3)
    else:
        reference, _, native, _, _, _, _, _ = actor_sources(mode)
    target = data['checkpoint']
    assert wait(run, 0, 900)['reason'] == 'first-frame', 'RNG FIXTURE / FIRST ROOM'
    compare(run, reference, 0)
    commands = (reference/'route-actions.txt').read_text().splitlines()[:target]
    for sequence, line in enumerate(commands, 1):
        assert int(line.split()[0]) == sequence, 'RNG FIXTURE / INPUT SEQUENCE'
        fields = line.split()
        if fields[1] == 'assist':
            current = assist(run, fields[2], int(fields[3]))
        else:
            temporary = run/'route-command.tmp'
            temporary.write_text(line+'\n'); temporary.replace(run/'route-command.txt')
            current = wait(run, sequence, 180)
        if sequence < target:
            compare(run, reference, sequence)
        print(f'RNG FIXTURE checkpoint={sequence} step={current["step"]}', flush=True)
    # Complete the original diagnostic before assessing the changed capture;
    # a comparison failure must not leave it parked waiting for more inputs.
    temporary = run/'route-command.tmp'
    temporary.write_text(f'{target+1} finish\n'); temporary.replace(run/'route-command.txt')
    assert wait(run, target+1, 60)['reason'] == 'discovery-finished', 'RNG FIXTURE / FINISH'
    check(run, fixture)


def check(run, fixture):
    data = verify(fixture)
    mode = data.get('mode', 1)
    target = data['checkpoint']
    if mode == 1:
        reference = ROOT/'ref/mame/runs/gameplay-level3-fresh-1'
        native = ROOT/'amiga/.run-845iXe'
        verify_reference(reference, 3)
    else:
        reference, _, native, _, _, _, _, _ = actor_sources(mode)
    for sequence in range(target):
        compare(run, reference, sequence)
        if mode == 5:
            source = ROOT/'ref/mame/runs/item16-level4-opponents-1'
            assert (run/f'route-{sequence}-opponents.bin').read_bytes() == (source/f'route-{sequence}-opponents.bin').read_bytes(), 'RNG FIXTURE / OPPONENT PREFIX'
    current = dict(field.split('=') for field in (run/'route-status.txt').read_text().split())
    assert int(current['sequence']) == target+1 and current['reason'] == 'discovery-finished', 'RNG FIXTURE / FINISH'
    log = (run/'session.log').read_text()
    assert log.count('RNG_FIXTURE_ROLLBACK ' if mode == 1 else 'RNG_FIXTURE_SEED ') == 1, 'RNG FIXTURE / WRITE COUNT'
    assert f'RNG_FIXTURE_COMPLETE checkpoint={target}\n' in log, 'RNG FIXTURE / POSITIVE ENDPOINT'
    label = f'route-{target}'
    native_label = f'route-{data.get("native_after", target)}'
    context = (run/(label+'-context.bin')).read_bytes()
    assert len(context) == 0x2200, 'RNG FIXTURE / CONTEXT EXTENT'
    context_match = context == (native/(native_label+'-context.bin')).read_bytes()
    actual = (run/(label+'-a5.bin')).read_bytes()
    expected = (native/(native_label+'-a5.bin')).read_bytes()
    baseline = (reference/(label+'-a5.bin')).read_bytes()
    assert len(actual) == len(expected) == len(baseline) == 22266, 'RNG FIXTURE / A5 EXTENT'
    if mode != 1:
        before = (run/f'route-{data["inject_checkpoint"]}-a5.bin').read_bytes()
        changed = bytearray(before); changed[-0x18fc:-0x18f8] = data['after'].to_bytes(4, 'big')
        assert (run/'rng-checkpoint-after-a5.bin').read_bytes() == changed, 'RNG FIXTURE / ONLY SEED CHANGED'
        assert baseline[-0x5208:-0x5208+372] != expected[-0x5208:-0x5208+372], 'RNG FIXTURE / UNMODIFIED ACTORS CONTROL'
        assert int(current['step']) == data['final_step'], 'RNG FIXTURE / FINAL STEP'
        if mode in (4, 5):
            before_context = (run/f'route-{data["inject_checkpoint"]}-context.bin').read_bytes()
            changed_context = bytearray(before_context)
            for i in range(data['context_count']):
                offset = data[f'context_offset_{i}']
                assert changed_context[offset] == data[f'context_before_{i}'], 'RNG FIXTURE / CONTEXT BEFORE'
                changed_context[offset] = data[f'context_after_{i}']
            assert (run/'rng-checkpoint-after-context.bin').read_bytes() == changed_context, 'RNG FIXTURE / CONTEXT WRITE EXTENT'
            assert log.count('RNG_FIXTURE_CONTEXT bytes=5\n') == 1, 'RNG FIXTURE / CONTEXT WRITE COUNT'
    if mode == 5:
        before_opponents = (run/f'route-{data["inject_checkpoint"]}-opponents.bin').read_bytes()
        changed_opponents = bytearray(before_opponents)
        for i in range(data['opponent_count']):
            offset = data[f'opponent_offset_{i}']
            assert int.from_bytes(changed_opponents[offset:offset+2], 'big') == data[f'opponent_before_{i}'], 'RNG FIXTURE / TIMER BEFORE'
            changed_opponents[offset:offset+2] = data[f'opponent_after_{i}'].to_bytes(2, 'big')
        assert (run/'rng-checkpoint-after-opponents.bin').read_bytes() == changed_opponents, 'RNG FIXTURE / TIMER WRITE EXTENT'
        assert log.count('RNG_FIXTURE_OPPONENTS words=5\n') == 1, 'RNG FIXTURE / TIMER WRITE COUNT'
    for distance, size in ((0x5208, 372), (0x18fc, 4), (0x4fd2, 2), (0x4fc2, 2), (0x349a, 2)):
        assert actual[-distance:-distance+size] == expected[-distance:-distance+size], 'RNG FIXTURE / GAME STATE'
    if mode == 1:
        paired_frame(native, run, native_label, label)
        assert context_match, 'RNG FIXTURE / FULL CONTEXT'
        assert baseline[-0x18fc:-0x18f8] != expected[-0x18fc:-0x18f8], 'RNG FIXTURE / UNMODIFIED RNG CONTROL'
        assert (reference/(label+'-context.bin')).read_bytes() != context, 'RNG FIXTURE / UNMODIFIED CONTEXT CONTROL'
        report = dict(scope='single ambient RNG draw, next recorded checkpoint only',
                      pixels_planes_palette_context_actors_rng_match=True)
    else:
        # Actor fixtures deliberately leave earlier torch state intact. Report
        # the full frame check separately; do not turn it into route acceptance.
        try:
            paired_frame(native, run, native_label, label)
            frame_match = True
        except RuntimeError as error:
            if not any(str(error).endswith(native_label+' '+reason)
                       for reason in ('PIXELS', 'AGA PLANES', 'RGB PALETTE', 'CONTEXT WORDS')):
                raise
            frame_match = False
        report = dict(scope='checkpoint seed replacement, next actor checkpoint only',
                      actors_rng_counters_match=True, changes_only_rng_seed=True,
                      frame_gate_matches=frame_match, full_context_matches=context_match)
        if mode == 4:
            assert frame_match and context_match, 'RNG FIXTURE / ALIGNED FULL FRAME'
            report.update(scope='preceding seed and five context-byte replacements, next actor checkpoint only',
                          changes_only_rng_seed=False, changed_context_bytes=data['context_count'])
    if mode == 5:
        assert (run/(label+'-opponents.bin')).read_bytes() == (ROOT/'amiga/.run-KkYYZ2'/f'route-{data["native_after"]}-opponents.bin').read_bytes(), 'RNG FIXTURE / FINAL OPPONENTS'
        report.update(scope='seed, context and five persistent skeleton timer replacements', changes_only_rng_seed=False, changed_context_bytes=5, changed_timer_words=5, persistent_opponents_match=True)
    report.update(checkpoint=target, native_checkpoint=data.get('native_after', target),
                  frame=int(current['step']), original_prefix_checkpoints=target,
                  unmodified_reference_matches=False, whole_route_accepted=False)
    (run/'rng-fixture-comparison.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('fixture', type=Path)
    parser.add_argument('--check-only', action='store_true')
    args = parser.parse_args()
    if args.check_only:
        mode = verify(args.fixture).get('mode', 1)
        label = {1: 'rng-rollback', 2: 'actor3-seed', 3: 'actor4-seed', 4: 'actor4-context', 5: 'actor4-timers'}[mode]
        recorded = pins(ROOT/f'manifest/gameplay-item16-{label}.sha256')
        assert str((args.run/'session.log').resolve().relative_to(ROOT)) in recorded, 'RNG FIXTURE / UNPINNED RUN'
        for name, digest in recorded.items():
            assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'RNG FIXTURE / CAPTURE PIN '+name
    (check if args.check_only else replay)(args.run, args.fixture)
