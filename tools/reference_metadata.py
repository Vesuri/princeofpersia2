#!/usr/bin/env python3
"""Generate local original-byte guards for the reference probes, never tracked."""
import hashlib
import json
from pathlib import Path
from extract_original_data import DESTINATION, ROOT, pins, verify_output
from amigaxdev.mac.resource_fork import read_resource_fork
from amigaxdev.mac.m68k_sweep import jump_table


def generate(path):
    verify_output(DESTINATION)
    code={r.rid:r.body for r in read_resource_fork(DESTINATION/'Prince of Persia 2.rsrc') if r.kind==b'CODE'}
    entries, _, clean=jump_table(code[0]);assert clean
    first={}
    for i,e in enumerate(entries):
        first.setdefault(e[0],(32+8*i,e[1]+4))
    # Assemble the pinned interleaved IIci ROM only in local generated metadata.
    manifest=pins(ROOT/'manifest/reference-inputs.sha256')
    rom=bytearray(512*1024)
    names=('341-0736.um12','341-0735.um11','342-0734.um10','342-0733.um9')
    for lane,name in enumerate(names):
        relative='ref/mame/roms/maciici/'+name
        data=(ROOT/relative).read_bytes()
        if len(data)!=128*1024 or hashlib.sha256(data).hexdigest()!=manifest[relative]:
            raise ValueError('REFERENCE / ROM HASH MISMATCH')
        rom[lane::4]=data
    lines=['return {dispatcher={address=0x408099b0,hex='+json.dumps(rom[0x99b0:0x99b8].hex())+'},segments={']
    for seg,(jt,offset) in sorted(first.items()):
        data=code[seg]
        lines.append('[%d]={jt=%d,offset=%d,hex=%s},'%(seg,jt,offset,json.dumps(data.hex())))
    lines.append('},resources={')
    for fork in sorted(DESTINATION.rglob('*.rsrc')):
        if not fork.stat().st_size:continue
        name=str(fork.relative_to(DESTINATION))[:-5]
        resources=read_resource_fork(fork)
        entries=','.join('[%s]=%d'%(json.dumps(r.kind.hex()+':'+str(r.rid)),len(r.body)) for r in resources)
        lines.append('[%s]={%s},'%(json.dumps(name),entries))
    lines.append('}}\n')
    path.write_text('\n'.join(lines))

if __name__=='__main__':
    import sys
    generate(Path(sys.argv[1]))
