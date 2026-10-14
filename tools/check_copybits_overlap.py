"""Check pinned original cutscene self-copies against the shared byte blitter."""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
from extract_original_data import ROOT, pins


def check(run):
    for name, digest in pins(ROOT/'manifest/gameplay-self-copy.sha256').items():
        assert hashlib.sha256((run/name).read_bytes()).hexdigest()==digest, 'SELF COPY / HASH '+name
    log=(run/'session.log').read_text()
    assert log.count('GAMEPLAY_DISCOVERY_COMPLETE\n')==1 and 'FAIL REFERENCE' not in log, 'SELF COPY / COMPLETION'
    calls=re.findall(r'^GAMEPLAY_COPY_RETURN prefix=(copy-\d+)$',log,re.M)
    assert calls==[f'copy-{n}' for n in range(1,9)], 'SELF COPY / CALLS'
    executable=ROOT/'tmp/check-copybits-overlap'
    subprocess.run(['c++','-std=c++17','-O2','-I'+str(ROOT/'src'),str(ROOT/'tools/check_copybits_overlap.cpp'),'-o',str(executable)],check=True)
    for prefix in calls:
        subprocess.run([str(executable),str(run/prefix)],check=True)
    print('ORIGINAL SELF COPY PASS: eight complete pixel buffers, including untouched bytes')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path)
    check(p.parse_args().run)
