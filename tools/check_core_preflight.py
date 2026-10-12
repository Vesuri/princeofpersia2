#!/usr/bin/env python3
"""Loader input and reference identity checks; not the native execution gate."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import struct
import subprocess
from extract_original_data import ROOT, DESTINATION, pins, verify_output
from report_reference import analyze
from amigaxdev.mac.resource_fork import read_resource_fork


def require(ok, why):
    if not ok:
        raise ValueError('CORE PREFLIGHT / '+why)


def check(reference):
    verify_output(DESTINATION)
    expected = pins(ROOT/'manifest/reference-results.sha256')
    trace, facts = analyze((reference/'session.log').read_text())
    require(hashlib.sha256(trace).hexdigest() == expected['traps.txt'], 'ORIGINAL TRAP TRACE')
    require(hashlib.sha256((reference/'opening.ppm').read_bytes()).hexdigest() == expected['opening.ppm'], 'ORIGINAL COMPLETION CAPTURE')
    output = ROOT/'build/host'
    output.mkdir(parents=True, exist_ok=True)
    compiler = os.environ.get('CXX', 'c++')
    common = [compiler, '-std=c++17', '-O2', '-include', 'cstdint']
    for name, sources in (
        ('core_application_check', ['tools/core_application_check.cpp', 'src/mac/OriginalApplication.cpp', 'src/mac/ResourceMap.cpp', 'src/mac/SHA256.cpp']),
        ('test_core_identity', ['tools/test_core_identity.cpp', 'src/mac/SHA256.cpp']),
    ):
        subprocess.run([*common, *(str(ROOT/p) for p in sources), '-o', str(output/name)], check=True)
    app = DESTINATION/'Prince of Persia 2.rsrc'
    digest = pins(ROOT/'manifest/runtime-data.sha256')[app.name]
    result = subprocess.check_output([str(output/'core_application_check'), str(app), digest], text=True)
    require('APPLICATION PREFLIGHT PASS;' in result, 'INPUT COMPLETION')
    code0 = next(r.body for r in read_resource_fork(app) if r.kind == b'CODE' and r.rid == 0)
    layout = struct.unpack_from('>IIII', code0)
    observed = re.search(r'A5 above=(\d+) below=(\d+) jumpBytes=(\d+) jumpOffset=(\d+)', result)
    require(observed is not None and tuple(map(int, observed.groups())) == layout, 'A5 LAYOUT ORACLE')
    print(result, end='')
    result = subprocess.check_output([str(output/'test_core_identity')], text=True)
    observed = {bytes.fromhex(s).decode('mac_roman'): {'error': int(e, 16), 'value': int(v, 16)}
                for s, v, e in re.findall(r'^IDENTITY ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+)$', result, re.M)}
    require(observed == facts['gestalt'], 'GESTALT ORIGINAL RETURNS')
    env = re.search(r'^SYSENV ([0-9a-f]+)$', result, re.M)
    require(env is not None and env[1].upper() == facts['sysenvirons'].replace('/', ''), 'SYSENV ORIGINAL RECORD')
    require('PASS SHA256 standard vector;' in result, 'IDENTITY COMPLETION')
    print('CORE PREFLIGHT PASS: pinned application, corruption controls, A5 layout, original identity returns')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', type=Path, help='completed original reference run directory')
    check(parser.parse_args().reference)
