"""Compare shared NFNT metrics with captured original Macintosh calls."""
import argparse
import hashlib
import os
import re
from pathlib import Path
import struct
import subprocess
import tempfile
from extract_original_data import ROOT, DESTINATION, pins, verify_output
from amigaxdev.mac.resource_fork import read_resource_fork


def check(reference):
    log = (reference/'session.log').read_text()
    assert 'FAIL REFERENCE' not in log and '[LUA ERROR]' not in log, 'FONT / REFERENCE FAILURE'
    completed = re.findall(r'^INTEGRATION_COMPLETE calls=(\d+)$', log, re.M)
    assert len(completed) == 1 and int(completed[0]) >= 331, 'FONT / REFERENCE COMPLETION'
    assert log.count('REFERENCE_CAPTURED opening-text-1\n') == 1, 'FONT / OPENING COMPLETION'
    verify_output(DESTINATION)
    for name, expected in pins(ROOT/'manifest/integration-font-reference.sha256').items():
        assert hashlib.sha256((reference/name).read_bytes()).hexdigest() == expected, 'FONT / REFERENCE PIN '+name
    resources={(r.kind,r.rid):r.body for r in read_resource_fork(DESTINATION/'Data/Prince.rsrc.rsrc')}
    work=Path(tempfile.mkdtemp(prefix='font-contract-',dir=ROOT/'tmp'))
    executable=work/'probe'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-fsanitize=address,undefined','-Isrc','tools/probe_bitmap_font.cpp','-o',str(executable)],cwd=ROOT,check=True)
    results={}
    for family,bitmap,size in ((213,24878,20),(212,23331,14)):
        fond=work/f'FOND-{family}';nfnt=work/f'NFNT-{bitmap}'
        fond.write_bytes(resources[b'FOND',family]);nfnt.write_bytes(resources[b'NFNT',bitmap])
        lines=subprocess.check_output([str(executable),str(fond),str(nfnt),str(family),str(size)],text=True).splitlines()
        results[family]=([int(n) for n in lines[0].split()[1:]],{int(a):int(b) for _,a,b in (line.split() for line in lines[1:])})
    port=(reference/'integration-325-before-port.bin').read_bytes()
    family=struct.unpack_from('>H',port,68)[0]
    assert family==213 and port[70]==0 and struct.unpack_from('>H',port,74)[0]==20, 'FONT / ORIGINAL SELECTION'
    metrics,widths=results[family]
    for serial in (325,326):
        assert tuple(metrics)==struct.unpack_from('>4H',(reference/f'integration-{serial}-after-pointer-3.bin').read_bytes()), 'FONT / ORIGINAL GETFONTINFO'
    for serial in (327,330):
        character=struct.unpack_from('>H',(reference/f'integration-{serial}-before-stack.bin').read_bytes())[0]
        original=struct.unpack_from('>H',(reference/f'integration-{serial}-after-stack.bin').read_bytes())[0]
        assert widths[character]==original, 'FONT / ORIGINAL CHARWIDTH'
    text=(reference/'integration-331-before-pointer-3.bin').read_bytes()
    assert text[0]<len(text), 'FONT / COMPLETE STRING'
    original=struct.unpack_from('>H',(reference/'integration-331-after-stack.bin').read_bytes())[0]
    assert sum(widths[c] for c in text[1:text[0]+1])==original, 'FONT / ORIGINAL STRINGWIDTH'
    print('FONT CONTRACT PASS: two original metrics calls, two character widths and one string width; both NFNTs reject every truncation')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference',type=Path)
    check(parser.parse_args().reference)
