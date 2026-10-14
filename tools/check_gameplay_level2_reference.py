"""Require two fresh original routes through the selected level, with matching game evidence."""
import argparse
from pathlib import Path
import re
from replay_gameplay_route import ROOT, compare, inputs, verify_reference


def check(first, second, level=2):
    assert level in (2, 3, 4), "ROUTE / UNSUPPORTED LEVEL"
    boundaries = {95: (1001, 2, 2), 114: (1360, 3, 1)}
    if level >= 3:
        boundaries[180] = (2767, 4, 1)
    if level == 4:
        boundaries[265] = (4656, 5, 1)
    last = max(boundaries)
    final_step, final_level, final_room = boundaries[last]
    canonical = ROOT / 'ref/mame/runs/gameplay-level1-complete-1'
    verify_reference(canonical)
    schedule = sum((inputs(ROOT / f'tools/gameplay_routes/level{n}.tsv') for n in range(1, level + 1)), [])
    assert len(schedule) == last, 'ROUTE / SCHEDULE LENGTH'
    expected_actions = [(str(i), str(count), keys) for i, (count, keys) in enumerate(schedule, 1)] + [(str(last + 1), 'finish')]
    keymaps = []
    for run in (first, second):
        log = (run / 'session.log').read_text()
        assert log.count('GAMEPLAY_DISCOVERY_COMPLETE\n') == 1 and 'FAIL REFERENCE' not in log, 'ROUTE / ORIGINAL COMPLETION'
        assert 'GAMEPLAY_STATE_SAVE' not in log and 'GAMEPLAY_STATE_LOAD' not in log, 'ROUTE / FRESH RUN REQUIRED'
        actions = [tuple(line.split()) for line in (run / 'route-actions.txt').read_text().splitlines()]
        assert actions == expected_actions, 'ROUTE / AUTHORED INPUTS'
        checkpoints = re.findall(r'^GAMEPLAY_ROUTE_CHECKPOINT sequence=(\d+) frames=(\d+) step=(\d+) level=(\d+) room=(\d+).*? reason=([\w-]+) prefix=route-(\d+)$', log, re.M)
        assert len(checkpoints) == last + 2, 'ROUTE / CHECKPOINT COUNT'
        assert all(int(row[0]) == int(row[6]) == i for i, row in enumerate(checkpoints)), 'ROUTE / CHECKPOINT ORDER'
        assert checkpoints[95][1:6] == ('1001', '1001', '2', '2', 'next-level'), 'ROUTE / ENTRY'
        for ordinal, (step, target, room) in boundaries.items():
            assert checkpoints[ordinal][1:6] == (str(step), str(step), str(target), str(room), 'next-level'), 'ROUTE / NATURAL EXIT'
        assert checkpoints[0][1:6] == ('0', '0', '1', '4', 'first-frame'), 'ROUTE / INITIAL FRAME'
        assert checkpoints[last + 1][1:6] == (str(final_step), str(final_step), str(final_level), str(final_room), 'discovery-finished'), 'ROUTE / FINISH'
        for i, row in enumerate(checkpoints[:last + 1]):
            frames, step, current_level, room = map(int, row[1:5])
            assert frames == step, 'ROUTE / FRAME STEP'
            assert current_level == 1 + sum(i >= boundary for boundary in boundaries), 'ROUTE / LEVEL ORDER'
            if i != 0 and i not in boundaries:
                assert row[5] == 'completed-frames' and frames - int(checkpoints[i - 1][1]) == schedule[i - 1][0], 'ROUTE / COMPLETED INTERVAL'
            state = (run / f'route-{i}-a5.bin').read_bytes()
            assert tuple(int.from_bytes(state[offset:offset + 2], 'big') for offset in (-0x4fd2, -0x4fc2, -0x349a)) == (current_level, room, step), 'ROUTE / CAPTURE COUNTERS'
        for i in range(96):
            compare(run, canonical, i)
        keys = re.findall(r'^GAMEPLAY_ROUTE_KEYS frame=(\d+) step=(\d+) map=([0-9A-F]{32})$', log, re.M)
        expected_steps = list(range(1002)) + list(range(1001, 1361))
        if level >= 3:
            expected_steps += list(range(1360, 2768))
            scenes = re.findall(r"^GAMEPLAY_ROUTE_NIS_(ENTER|RETURN) selector=(\d+)$", log, re.M)
            assert scenes == [(event, str(selector)) for selector in ((7, 4, 9, 100, 10) if level == 3 else (7, 4, 9, 100, 10, 20)) for event in ("ENTER", "RETURN")], "ROUTE / CUTSCENE COMPLETION"
        if level == 4:
            expected_steps += list(range(2767, 4657))
        assert len(keys) == len(expected_steps) and [(int(a), int(b)) for a, b, _ in keys] == [(v, v) for v in expected_steps], 'ROUTE / CONSUMED KEY ORDER'
        keymaps.append(keys)
    assert first.resolve() != second.resolve(), 'ROUTE / INDEPENDENT RUNS REQUIRED'
    assert keymaps[0] == keymaps[1], 'ROUTE / CONSUMED KEY REPEAT'
    for i in range(95, last + 2):
        compare(first, second, i)
    print(f'LEVEL {level} ORIGINAL REPEAT PASS: two fresh runs, {last + 1} state/pixel checkpoints, {len(expected_steps)} matching key reads, natural level-{final_level} entry')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('first', type=Path); parser.add_argument('second', type=Path)
    parser.add_argument('--level', type=int, choices=(2, 3, 4), default=2)
    args = parser.parse_args(); check(args.first, args.second, args.level)
