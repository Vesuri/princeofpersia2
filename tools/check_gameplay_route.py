"""Require the original-paired native gameplay route and restored hardware."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from check_core_result import restored
from check_gameplay_restart import paired_frame
from check_display_scenes import decode_copper
from check_integration_result import require
from gameplay_route_inputs import metadata


def check(run, reference, setup, level=1, manifest=None, hall=False, deferred_rng=False):
    if level == 0:
        from gameplay_assisted_inputs import metadata as assisted_metadata
        keys, checkpoints, _, labels = assisted_metadata(reference, **({'manifest':manifest} if manifest else {}))
    else:
        keys, checkpoints = metadata(reference, level)
        labels = [f'route-{i}' for i in range(len(checkpoints))]
    final = checkpoints[-1]
    require(setup == 'a1200-030', 'ROUTE CURRENT TARGET')
    receipt = json.loads((run / 'build-receipt.json').read_text())
    require({'-DPOP2_VERIFY', '-DPOP2_INTEGRATION_PROBE',
             '-DPOP2_GAMEPLAY_ROUTE_PROBE'}.issubset(receipt['flags'].split()), 'ROUTE BUILD FLAGS')
    for key, name in (('exe', 'dh1/PrinceOfPersia2'), ('elf', 'PrinceOfPersia2.elf')):
        require(hashlib.sha256((run / name).read_bytes()).hexdigest() == receipt[key], 'ROUTE BUILD IDENTITY')
    if level >= 2:
        require(f'-DPOP2_GAMEPLAY_ROUTE_LEVEL={level}' in receipt['flags'].split(), 'ROUTE LEVEL BUILD FLAG')
    log = (run / 'gdb-out.log').read_text()
    restored(run, log, setup)
    completion = 'ASSISTED SURVEY COMPLETE' if level == 0 else f'LEVEL {level} COMPLETE'
    if level == 0 and 'GAMEPLAY_ENDING_COMPLETE\n' in (reference/'session.log').read_text():
        completion = 'ASSISTED ENDING COMPLETE'
    if hall:
        require(completion == 'ASSISTED ENDING COMPLETE' and '-DPOP2_GAMEPLAY_HALL_PROBE' in receipt['flags'].split(), 'HALL BUILD/ENDING')
        completion = 'HALL COMPLETE'
    require(log.count(f'CORE REASON GAMEPLAY ROUTE / {completion}\n') == 1, 'ROUTE COMPLETION')
    require(f'ROUTE RESULT complete=1 rooms={sum(p[4] for p in checkpoints)} frames={final[0]} keys={len(keys)} checkpoints={len(checkpoints)-1} last=fffe\n' in log, 'ROUTE COUNTS')
    require('INTEGRATION AUDIT code=1 input=1\n' in log, 'ROUTE RESTORATION AUDIT')
    records = re.findall(r'^ROUTE CHECKPOINT ordinal=(\d+) frames=(\d+) keys=(\d+) level=(\d+) room=(\d+) step=(\d+) planes=([0-9a-f]+) parity=(\d+)$', log, re.M)
    require(len(records) == len(checkpoints), 'ROUTE CHECKPOINT COUNT')
    failures = []; deferred = []
    allowed = json.loads((Path(__file__).resolve().parents[1]/'manifest/gameplay-deferred-rng.json').read_text()) if deferred_rng else []
    if deferred_rng: require(level in (1,2), 'DEFERRED RNG ROUTE SCOPE')
    def observed(action):
        try:
            action()
        except RuntimeError as error:
            text = str(error)
            match = re.fullmatch(r'INTEGRATION / route-(\d+) RNG', text)
            accepted = False
            if match and deferred_rng:
                ordinal = int(match[1]); label = f'route-{ordinal}'
                actual = dict(level=level, ordinal=ordinal,
                    native=hashlib.sha256((run/(label+'-a5.bin')).read_bytes()[-0x18fc:-0x18f8]).hexdigest(),
                    original=hashlib.sha256((reference/(labels[ordinal]+'-a5.bin')).read_bytes()[-0x18fc:-0x18f8]).hexdigest())
                accepted = actual in allowed
            if accepted: deferred.append(text)
            else: failures.append(text)
    for sequence, row in enumerate(records):
        require(int(row[0]) == sequence, 'ROUTE CHECKPOINT ORDER')
        expected = checkpoints[sequence]
        require(tuple(map(int, row[1:6])) == (expected[0], expected[3], expected[1], expected[2], expected[0]), 'ROUTE CHECKPOINT COUNTERS')
        label = f'route-{sequence}'
        observed(lambda: paired_frame(run, reference, label, labels[sequence]))
        observed(lambda: require((run / (label + "-context.bin")).read_bytes() ==
                (reference / (labels[sequence] + "-context.bin")).read_bytes(), label + " FULL CONTEXT"))
        native = (run / (label + '-a5.bin')).read_bytes()
        mac = (reference / (labels[sequence] + '-a5.bin')).read_bytes()
        for start, size in ((-0x5208, 6 * 62), (-0x4fd2, 2), (-0x4fc2, 2), (-0x349a, 2)):
            observed(lambda: require(native[start:start + size] == mac[start:start + size], label + ' ACTORS/COUNTERS'))
        observed(lambda: require(decode_copper((run / (label + '-copper.bin')).read_bytes(), int(row[6], 16), int(row[7])) ==
                (reference / (labels[sequence] + '.rgb')).read_bytes(), label + ' COPPER'))
    require(not failures, 'ROUTE PAIR FAILURES: ' + '; '.join(failures))
    (run/'route-comparison.json').write_text(json.dumps(dict(level=level,checkpoints=len(checkpoints),deferred_item_16=deferred),indent=2)+'\n')
    print(f'GAMEPLAY THROUGH LEVEL {level} NATIVE PAIR PASS: {len(checkpoints)} completed checkpoints; {len(deferred)} explicitly deferred RNG mismatches')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('run', type=Path); p.add_argument('reference', type=Path)
    p.add_argument('--setup', required=True)
    p.add_argument('--level', type=int, choices=(0, 1, 2, 3), default=1)
    p.add_argument('--manifest', type=Path)
    p.add_argument('--hall', action='store_true')
    p.add_argument('--deferred-rng', action='store_true', help='D11: only the pinned prior level-one RNG differences')
    a = p.parse_args(); check(a.run, a.reference, a.setup, a.level, a.manifest, a.hall, a.deferred_rng)
