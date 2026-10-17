"""Extract the pinned SaveRect inner loop and author boundary/overlap cases."""
import hashlib
import json
from pathlib import Path
import struct
from extract_original_data import ROOT, pins
import _amigaxdev
from amigaxdev.mac.resource_fork import read_resource_fork

LOOP_SHA = 'ea8d8469e6402d5b7f8f1030d85c8db2bc265660ff675a39a5323c28243a49fa'
OUT = ROOT/'tmp/save-rect-fixture'


def build():
    path=ROOT/'tmp/runtime-data/Prince of Persia 2.rsrc'
    assert hashlib.sha256(path.read_bytes()).hexdigest()==pins(ROOT/'manifest/runtime-data.sha256')[path.name]
    code=next(r.body for r in read_resource_fork(path) if r.kind==b'CODE' and r.rid==17)
    body=code[0x53a4:0x53c6]
    assert hashlib.sha256(body).hexdigest()==LOOP_SHA, 'SAVE RECT / ORIGINAL LOOP'
    # Only the original backward branch remains in this slice; RTS is authored.
    image=body+struct.pack('>H',0x4e75)
    cases=[]
    for width in (4,8,12,16,60,256,512,32764):
        for alignment in range(4):
            for delta in (-7,-4,-3,-1,0,1,3,4,7,width+4):
                for cc in (0,31):
                    src=16+alignment;dst=src+delta
                    cases.append([width,src,dst,cc])
    # Exercise all incoming condition codes, not just the two extremes.
    cases.extend([4,17,22,cc] for cc in range(32))
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'original.bin').write_bytes(image)
    (OUT/'cases.json').write_text(json.dumps(cases)+'\n')
    (OUT/'cases.lua').write_text('return {'+','.join('{'+','.join(map(str,c))+'}' for c in cases)+'}\n')
    (OUT/'fixture.h').write_text('static const unsigned char original[] = {'+','.join(map(str,image))+'};\n'+
        'static const unsigned long cases[][4] = {'+','.join('{'+','.join(map(str,c))+'}' for c in cases)+'};\n'+
        '#define CASE_COUNT '+str(len(cases))+'\n')
    print('SAVE RECT FIXTURE:',len(cases),'cases; original loop SHA256',LOOP_SHA)


if __name__=='__main__':build()
