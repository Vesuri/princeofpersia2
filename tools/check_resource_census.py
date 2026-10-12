#!/usr/bin/env python3
"""Compare all installed resource bodies and metadata through independent readers."""
import hashlib
import json
import os
import subprocess
import tempfile
from pathlib import Path
from extract_original_data import ROOT, DESTINATION, verify_output, pins
from amigaxdev.mac.resource_fork import read_resource_fork
from report_reference import require


def check():
    verify_output(DESTINATION)
    scratch=Path(tempfile.mkdtemp(prefix='resource-census-',dir=ROOT/'tmp'))
    executable=scratch/'reader'
    sources=['ResourceDirectory','ResourceForks','ResourceMap','ResourceWriter','SHA256']
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-include','cstdint','-Isrc',
                    'tools/resource_census_check.cpp',*[f'src/mac/{s}.cpp' for s in sources],
                    '-o',str(executable)],cwd=ROOT,check=True)
    counts={}
    for relative in sorted(pins(ROOT/'manifest/runtime-data.sha256')):
        path=DESTINATION/relative
        if not relative.endswith('.rsrc') or not path.stat().st_size:continue
        expected=[]
        for entry in read_resource_fork(path):
            name=entry.name.encode('mac_roman').hex() if entry.name else '-'
            expected.append(f'{entry.kind.hex()} {entry.rid} {entry.attrs} {len(entry.body)} {hashlib.sha256(entry.body).hexdigest()} {name}')
        actual=subprocess.check_output([str(executable),str(path)],text=True).splitlines()
        require(sorted(actual)==sorted(expected),'RESOURCE CENSUS '+relative)
        counts[relative]=len(expected)
    require(counts and sum(counts.values())>0,'RESOURCE CENSUS NONEMPTY')
    (scratch/'report.json').write_text(json.dumps({'forks':counts,'resources':sum(counts.values())},indent=2)+'\n')
    print(f'RESOURCE CENSUS PASS: {len(counts)} forks, {sum(counts.values())} complete resources; {scratch.relative_to(ROOT)}')


if __name__=='__main__':check()
