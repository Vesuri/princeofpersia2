"""Require production fast-entry guards and positive integration completion."""
import argparse
import json
from pathlib import Path
import re
from check_core_result import restored


def evidence(log):
    assert log.count('QUICK CONTROLS calls=78 rejected=72 failures=0\n') == 1, 'QUICK / FIXTURE'
    assert log.count('QUICK DEADLINES rejected=6\n') == 1, 'QUICK / DEADLINE'
    guards = re.findall(r'^QUICK GUARD checks=(\d+) faults=(\d+)$', log, re.M)
    assert len(guards) == 1 and int(guards[0][0]) > 0 and guards[0][1] == '0', 'QUICK / NATIVE GUARD'
    assert log.count('CLOCK FAULT 0\n') == 1, 'QUICK / CLOCK'
    assert log.count('CORE REASON INTEGRATION / ROUTE COMPLETE\n') == 1, 'QUICK / ROUTE'
    assert log.count('INTEGRATION RESTORED\n') == 1, 'QUICK / RESTORE'


def check(run):
    flags = json.loads((run/'build-receipt.json').read_text())['flags'].split()
    assert {'-DPOP2_VERIFY', '-DPOP2_QUICK_TRAP_PROBE', '-DPOP2_INTEGRATION_PROBE'} <= set(flags), 'QUICK / BUILD'
    log = (run/'gdb-out.log').read_text()
    restored(run, log, 'a1200-030')
    evidence(log)
    changes = [
        ('calls=78', 'calls=77'), ('rejected=72', 'rejected=71'),
        ('QUICK DEADLINES rejected=6', 'QUICK DEADLINES rejected=5'),
        ('failures=0', 'failures=1'), ('faults=0', 'faults=1'),
        ('CLOCK FAULT 0', 'CLOCK FAULT 1'),
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
    print('QUICK TRAPS PASS: six positive bodies, 72 rejected guard cases, six due-deadline rejections, live native guard audit, restored route; nine alterations rejected')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    check(parser.parse_args().run)
