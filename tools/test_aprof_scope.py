"""Reject incomplete scoped captures using one completed run as the control."""
import contextlib
import io
from pathlib import Path
import re
import sys
import tempfile
from aprof_report import ROOT, check_run, parse_profile


def rejects(action):
    try:
        action()
    except SystemExit:
        return
    raise AssertionError('APROF / ACCEPTED ALTERED SCOPE')


def check(source):
    with contextlib.redirect_stdout(io.StringIO()):
        check_run(source)
        with tempfile.TemporaryDirectory(prefix='aprof-scope-test-', dir=ROOT/'tmp') as tmp:
            run = Path(tmp)
            for name in ['opening-frames.bin', 'logs']:
                (run/name).symlink_to((source/name).resolve())
            profiles = list(source.glob('*.prof'))
            for p in profiles:
                (run/p.name).symlink_to(p.resolve())
            original = (source/'gdb-out.log').read_text()
            log = run/'gdb-out.log'
            changes = [
                re.sub(r'^ACTIVE SCOPE COMPLETE.*\n', '', original, flags=re.M),
                re.sub(r'^ACTIVE WAIT EXIT 1 .*\n', '', original, flags=re.M),
                original.replace('ACTIVE WAIT ENTER 1 ', 'ACTIVE WAIT ENTER 2 ', 1),
                re.sub(r'(ACTIVE SCOPE COMPLETE waits=)(\d+)', lambda m: m[1] + str(int(m[2]) + 1), original),
                original.replace('CORE REASON INTEGRATION / ROUTE COMPLETE', 'CORE REASON WRONG STOP'),
            ]
            for changed in changes:
                assert changed != original, 'APROF / INACTIVE CONTROL'
                log.write_text(changed)
                rejects(lambda: check_run(run))
            log.write_text(original)
            for pattern in ['opening-*.prof', 'wait-*.prof']:
                missing = next(run.glob(pattern))
                target = missing.resolve()
                missing.unlink()
                rejects(lambda: check_run(run))
                missing.symlink_to(target)
            check_run(run)
    text = next(source.glob('opening-*.prof')).read_text()
    header, _, _ = parse_profile(text)
    for altered in [text.replace(f"units {header['units']}\n", f"units {header['units']+1}\n", 1),
                    text.replace('overflow 0\n', 'overflow 1\n', 1)]:
        assert altered != text
        rejects(lambda: parse_profile(altered))
    print('APROF SCOPE PASS: positive run, seven incomplete/altered captures and two accounting faults rejected')


if __name__ == '__main__':
    check(Path(sys.argv[1]))
