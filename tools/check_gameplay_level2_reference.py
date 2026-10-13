"""Require two fresh original routes through level two, with matching game evidence."""
import argparse
from pathlib import Path
import re
from replay_gameplay_route import ROOT, compare, inputs, verify_reference


def check(first, second):
    canonical = ROOT / 'ref/mame/runs/gameplay-level1-complete-1'
    verify_reference(canonical)
    schedule = inputs(ROOT / 'tools/gameplay_routes/level1.tsv') + inputs(ROOT / 'tools/gameplay_routes/level2.tsv')
    expected_actions = [(str(i), str(count), keys) for i, (count, keys) in enumerate(schedule, 1)] + [('115', 'finish')]
    keymaps = []
    for run in (first, second):
        log = (run / 'session.log').read_text()
        assert log.count('GAMEPLAY_DISCOVERY_COMPLETE\n') == 1 and 'FAIL REFERENCE' not in log, 'LEVEL TWO / ORIGINAL COMPLETION'
        assert 'GAMEPLAY_STATE_SAVE' not in log and 'GAMEPLAY_STATE_LOAD' not in log, 'LEVEL TWO / FRESH RUN REQUIRED'
        actions = [tuple(line.split()) for line in (run / 'route-actions.txt').read_text().splitlines()]
        assert actions == expected_actions, 'LEVEL TWO / AUTHORED INPUTS'
        checkpoints = re.findall(r'^GAMEPLAY_ROUTE_CHECKPOINT sequence=(\d+) frames=(\d+) step=(\d+) level=(\d+) room=(\d+).*? reason=([\w-]+) prefix=route-(\d+)$', log, re.M)
        assert len(checkpoints) == 116, 'LEVEL TWO / CHECKPOINT COUNT'
        assert all(int(row[0]) == int(row[6]) == i for i, row in enumerate(checkpoints)), 'LEVEL TWO / CHECKPOINT ORDER'
        assert checkpoints[95][1:6] == ('1001', '1001', '2', '2', 'next-level'), 'LEVEL TWO / ENTRY'
        assert checkpoints[114][1:6] == ('1360', '1360', '3', '1', 'next-level'), 'LEVEL TWO / NATURAL EXIT'
        assert checkpoints[0][1:6] == ('0', '0', '1', '4', 'first-frame'), 'LEVEL TWO / INITIAL FRAME'
        assert checkpoints[115][1:6] == ('1360', '1360', '3', '1', 'discovery-finished'), 'LEVEL TWO / FINISH'
        for i, row in enumerate(checkpoints[:115]):
            frames, step, level, room = map(int, row[1:5])
            assert frames == step, 'LEVEL TWO / FRAME STEP'
            assert level == (1 if i < 95 else 2 if i < 114 else 3), 'LEVEL TWO / LEVEL ORDER'
            if i not in (0, 95, 114):
                assert row[5] == 'completed-frames' and frames - int(checkpoints[i - 1][1]) == schedule[i - 1][0], 'LEVEL TWO / COMPLETED INTERVAL'
            state = (run / f'route-{i}-a5.bin').read_bytes()
            assert tuple(int.from_bytes(state[offset:offset + 2], 'big') for offset in (-0x4fd2, -0x4fc2, -0x349a)) == (level, room, step), 'LEVEL TWO / CAPTURE COUNTERS'
        for i in range(96):
            compare(run, canonical, i)
        keys = re.findall(r'^GAMEPLAY_ROUTE_KEYS frame=(\d+) step=(\d+) map=([0-9A-F]{32})$', log, re.M)
        expected_steps = list(range(1002)) + list(range(1001, 1361))
        assert len(keys) == 1362 and [(int(a), int(b)) for a, b, _ in keys] == [(v, v) for v in expected_steps], 'LEVEL TWO / CONSUMED KEY ORDER'
        keymaps.append(keys)
    assert first.resolve() != second.resolve(), 'LEVEL TWO / INDEPENDENT RUNS REQUIRED'
    assert keymaps[0] == keymaps[1], 'LEVEL TWO / CONSUMED KEY REPEAT'
    for i in range(95, 115):
        compare(first, second, i)
    print('LEVEL TWO ORIGINAL REPEAT PASS: two fresh runs, 20 state/pixel checkpoints, 1362 matching key reads, natural level-three entry')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('first', type=Path); parser.add_argument('second', type=Path)
    args = parser.parse_args(); check(args.first, args.second)
