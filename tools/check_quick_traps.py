"""Require production fast-entry guards and positive integration completion."""
import argparse
import json
from pathlib import Path
import re
from check_core_result import restored


def evidence(log):
    assert log.count('BLOCK MOVE checks=28388\n') == 1, 'QUICK / BLOCK MOVE'
    assert log.count('QUICK CONTROLS calls=84 rejected=78 failures=0\n') == 1, 'QUICK / FIXTURE'
    assert log.count('QUICK DEADLINES rejected=6\n') == 1, 'QUICK / DEADLINE'
    assert log.count('QUICK ZONE rejected=1\n') == 1, 'QUICK / ZONE'
    assert log.count('QUICK CALLBACK checks=64 rejected=128\n') == 1, 'QUICK / CALLBACK'
    guards = re.findall(r'^QUICK GUARD checks=(\d+) faults=(\d+)$', log, re.M)
    assert len(guards) == 1 and int(guards[0][0]) > 0 and guards[0][1] == '0', 'QUICK / NATIVE GUARD'
    assert log.count('QUICK AUDIO MODE -1\n') == 1, 'QUICK / LIVE AUDIO'
    assert log.count('CLOCK FAULT 0\n') == 1, 'QUICK / CLOCK'
    assert log.count('CORE REASON INTEGRATION / ROUTE COMPLETE\n') == 1, 'QUICK / ROUTE'
    assert log.count('INTEGRATION RESTORED\n') == 1, 'QUICK / RESTORE'


def check(run, setup):
    flags = json.loads((run/'build-receipt.json').read_text())['flags'].split()
    assert {'-DPOP2_VERIFY', '-DPOP2_QUICK_TRAP_PROBE', '-DPOP2_INTEGRATION_PROBE'} <= set(flags), 'QUICK / BUILD'
    log = (run/'gdb-out.log').read_text()
    restored(run, log, setup)
    evidence(log)
    changes = [
        ('BLOCK MOVE checks=28388', 'BLOCK MOVE checks=28387'),
        ('calls=84', 'calls=83'), ('rejected=78', 'rejected=77'),
        ('QUICK DEADLINES rejected=6', 'QUICK DEADLINES rejected=5'),
        ('QUICK ZONE rejected=1', 'QUICK ZONE rejected=0'),
        ('QUICK CALLBACK checks=64', 'QUICK CALLBACK checks=63'),
        ('checks=64 rejected=128', 'checks=64 rejected=127'),
        ('failures=0', 'failures=1'), ('faults=0', 'faults=1'),
        ('CLOCK FAULT 0', 'CLOCK FAULT 1'),
        ('QUICK AUDIO MODE -1', 'QUICK AUDIO MODE 0'),
        ('INTEGRATION / ROUTE COMPLETE', 'INTEGRATION / WRONG STOP'),
        ('INTEGRATION RESTORED', 'INTEGRATION UNRESTORED'),
    ]
    altered = [log.replace(a, b) for a, b in changes]
    altered.append(re.sub(r'QUICK GUARD checks=\d+', 'QUICK GUARD checks=0', log))
    for text in altered:
        assert text != log, 'QUICK / INACTIVE CONTROL'
        try:
            evidence(text)
        except AssertionError:
            continue
        raise AssertionError('QUICK / ACCEPTED ALTERATION')
    evidence(log)
    print('QUICK TRAPS PASS: 28,388 native copy cases, six positive bodies, 78 rejected guard cases, six due-deadline rejections, invalid zone rejection, 64 callback CCR cases and 128 inactive/pending-audio callback rejections, live native guard audit, restored route; fourteen alterations rejected')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--setup', choices=('a1200-030','a1200-fast'), default='a1200-030')
    args = parser.parse_args()
    check(args.run, args.setup)
