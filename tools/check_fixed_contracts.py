"""Compare fixed-point services with pinned original Macintosh results."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import subprocess
import tempfile
from extract_original_data import ROOT, pins


def check(reference):
    log = (reference/'session.log').read_text()
    assert 'FAIL REFERENCE' not in log and '[LUA ERROR]' not in log, 'FIXED / REFERENCE FAILURE'
    assert log.count('FIXED_FIXTURE_COMPLETE calls=80\n') == 1, 'FIXED / COMPLETION'
    rows = re.findall(r'^FIXED_RESULT trap=(\w+) a=(\w+) b=(\w+) result=(\w+)$', log, re.M)
    assert len(rows) == 80, 'FIXED / CASE COUNT'
    canonical = ''.join(' '.join(row).lower()+'\n' for row in rows).encode()
    expected = pins(ROOT/'manifest/integration-fixed-reference.sha256')['fixed-results.txt']
    assert hashlib.sha256(canonical).hexdigest() == expected, 'FIXED / ORIGINAL RESULT PIN'
    scratch = Path(tempfile.mkdtemp(prefix='fixed-contract-', dir=ROOT/'tmp'))
    probe = scratch/'probe'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-fsanitize=address,undefined',
                    '-Isrc', 'tools/probe_fixed_math.cpp', '-o', str(probe)], cwd=ROOT, check=True)
    output = subprocess.check_output([str(probe)], text=True,
                                     input=''.join(' '.join(row[:3])+'\n' for row in rows)).splitlines()
    assert len(output) == len(rows), 'FIXED / NATIVE RESULT COUNT'
    for row, actual in zip(rows, output):
        assert actual == row[3].lower(), ('FIXED / ORIGINAL MISMATCH', row, actual)
    print('FIXED CONTRACT PASS: 80 original signed, fractional, saturation and zero-divisor results')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', type=Path)
    check(parser.parse_args().reference)
