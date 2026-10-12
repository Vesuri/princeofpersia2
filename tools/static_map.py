#!/usr/bin/env python3
"""Reproducible flow map. Original bytes and listings stay under ignored tmp/."""
import argparse
import collections
import csv
import hashlib
import json
import re
from pathlib import Path
import struct
import trap_census as census
from extract_original_data import ROOT, DESTINATION, verify_output
from inspect_original import procedure_names
from amigaxdev.mac.resource_fork import read_resource_fork

OUT = ROOT/'tmp/static-map'

def a5world(resources, a5=0, strs=0):
    r = {(r.kind,r.rid):r.body for r in resources}
    below = struct.unpack_from('>I',r[b'CODE',0],4)[0]
    data, zero, drel = (r[k,0] for k in (b'DATA',b'ZERO',b'DREL'))
    result=bytearray(below); p=d=z=0
    while p < below:
        if d+2>len(data) or p+2>below: raise ValueError('DATA bounds')
        word=data[d:d+2]; result[p:p+2]=word; p+=2; d+=2
        if word==b'\0\0':
            if z+2>len(zero): raise ValueError('ZERO bounds')
            p+=struct.unpack_from('>H',zero,z)[0];z+=2
            if p>below: raise ValueError('ZERO extent')
    if d!=len(data) or z!=len(zero) or len(drel)%2: raise ValueError('A5 stream extent')
    relocations=list(struct.unpack('>'+str(len(drel)//2)+'h',drel))
    for off in relocations:
        pos=below+(off&~1)
        if not 0<=pos<=below-4: raise ValueError('DREL bounds')
        value=struct.unpack_from('>I',result,pos)[0]
        struct.pack_into('>I',result,pos,(value+(strs if off&1 else a5))&0xffffffff)
    return result,relocations,z//2

def build():
    verify_output(DESTINATION)
    resources=read_resource_fork(DESTINATION/'Prince of Persia 2.rsrc')
    from extract_original_data import pins
    for name,digest in pins(ROOT/'manifest/analysis-inputs.sha256').items():
        if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=digest:raise ValueError('ANALYSIS INPUT HASH / '+name)
    census.NAMES={int(a,16):b for a,b in re.findall(r'\[0x([0-9A-F]+)\] = "(\w+)"',(ROOT/'tmp/trap_names.lua').read_text())}
    census.load(DESTINATION/'Prince of Persia 2.rsrc')
    w=census.run(extra=True)
    # Runtime-installed LoadSeg hook and initialization/termination entries.
    w.walk(1,[0x10,0x64,0x404])
    names=procedure_names(resources)
    # Seed code following a verified name record only when it begins with LINK.
    # This adds local procedures absent from the exported jump table.
    roots=set(census.JT)|{(1,0x10),(1,0x64),(1,0x404)}
    for seg,off,name in names:
        raw=census.CODE[seg]; n=raw[off+1] if raw[off]==128 else raw[off]-128
        end=off+(2 if raw[off]==128 else 1)+n
        end=(end+1)&~1
        for target in (end,end+2):
            if raw[target:target+2]==bytes.fromhex('4e56'):
                roots.add((seg,target));w.walk(seg,[target]);break
    # Reach a fixed point of explicit PC-relative callback addresses.
    while True:
        extra={(s,t) for s in census.CODE for t in w.codeptrs[s]
               if 4<=t<len(census.CODE[s])-1 and t not in w.seen[s]
               and census.CODE[s][t:t+2] in (bytes.fromhex('4e56'),bytes.fromhex('48e7'))}
        if not extra:break
        roots|=extra
        for s,t in sorted(extra):w.walk(s,[t])
    roots |= w.indirect_roots
    OUT.mkdir(parents=True,exist_ok=True)
    for r in resources:
        if r.kind in (b'CODE',b'DATA',b'ZERO',b'DREL',b'STRS',b'MDRV'):
            (OUT/(r.kind.decode()+'_'+str(r.rid)+'.bin')).write_bytes(r.body)
    with (OUT/'entrypoints.csv').open('w') as f:
        f.write('segment,addr,name,note\n')
        for s,o in sorted(roots):f.write(f'{s},0x{o:04X},entry_{s}_{o:04X},DERIVED flow root\n')
    # Names refer to their proven records; no guess that a record is executable.
    existing=list(csv.DictReader((ROOT/'disasm/symbols.csv').open()))
    known={(int(x['segment']),int(x['offset'],0)) for x in existing}
    seeded=existing+[dict(segment=s,offset=f'0x{o:04X}',name=n+'_MacsBug',evidence='MEASURED',notes='Original RTS-adjacent procedure name record; not an entry point') for s,o,n in names if (s,o) not in known]
    with (OUT/'symbols.csv').open('w') as f:
        writer=csv.DictWriter(f,fieldnames=['segment','offset','name','evidence','notes']);writer.writeheader();writer.writerows(seeded)
    with (OUT/'ghidra-symbols.csv').open('w') as f:
        f.write('space,segment,offset,name,kind,evidence,notes\n')
        for row in seeded:f.write('code,{segment},{offset},{name},label,{evidence},{notes}\n'.format(**row))
    coverage={}; runs=[]
    for s,raw in sorted(census.CODE.items()):
        labels=['unresolved']*len(raw)
        for i in range(4):labels[i]='header'
        for i in w.seen[s]:labels[i]='switch-data'
        for off,insn in w.insn_at[s].items():
            for i in range(off,off+insn.size):labels[i]='code'
        for seg,off,_,_ in w.traps:
            if seg==s:labels[off:off+2]=['code','code']
        if s==19:
            for i in range(0x478,0x47c):labels[i]='runtime-data'
        if s==1:
            for i in range(4,16):labels[i]='runtime-data'
        for ns,off,n in names:
            if ns!=s:continue
            size=raw[off+1] if raw[off]==128 else raw[off]-128
            end=off+(2 if raw[off]==128 else 1)+size
            for i in range(off,min((end+1)&~1,len(raw))):
                if labels[i]=='code':raise ValueError(f'name/code overlap {s}:{i:x}')
                labels[i]='name-data'
        coverage[s]=dict(collections.Counter(labels))
        start=0
        for i in range(1,len(labels)+1):
            if i==len(labels) or labels[i]!=labels[start]:
                runs.append((s,start,i,labels[start]));start=i
    globals_,rel,runs_zero=a5world(resources)
    cached=[]
    slots={0x1f58:0xa8aa,0x1f5c:0xa8ab,0x1f60:0xa8ae,0x1f64:0xa87b,0x1f68:0xa976}
    for seg,insns in w.insn_at.items():
        for pc,insn in insns.items():
            match=re.fullmatch(r'-\$([0-9a-f]+)\(a5\), a0',insn.op_str)
            nxt=insns.get(pc+insn.size)
            if match and insn.mnemonic.startswith('movea') and nxt and nxt.mnemonic=='jsr' and nxt.op_str=='(a0)' and int(match[1],16) in slots:
                cached.append((seg,nxt.address,slots[int(match[1],16)]))
    resolved={(s,o) for s,o,_ in cached}
    result={'coverage':coverage,'ranges':runs,'traps':sorted((s,o,t) for s,o,t,_ in w.traps),
            'lowmem':sorted(set(x for x in w.lowmem if not x[3].startswith(('pea','lea')))),'absolute_other':sorted(set(w.absother)),
            'cached_services':sorted(cached),'unresolved_transfers':sorted(set(x for x in w.blind if x[:2] not in resolved)), 'roots':sorted(roots),
            'a5':{'below':len(globals_),'zero_runs':runs_zero,'relocations':len(rel),'strs_relocations':sum(x&1 for x in rel),'unbased_sha256':hashlib.sha256(globals_).hexdigest()}}
    for name,header,rows in (
            ('traps.csv',['segment','offset','word','name','context'],[(s,hex(o),hex(t),census.trapname(t),' | '.join(ctx)) for s,o,t,ctx in sorted(w.traps)]),
            ('byte-ranges.csv',['segment','start','end','classification'],runs),
            ('lowmem.csv',['segment','offset','address','instruction'],result['lowmem']),
            ('unresolved.csv',['segment','offset','transfer'],result['unresolved_transfers'])):
        with (OUT/name).open('w') as f:
            writer=csv.writer(f);writer.writerow(header);writer.writerows(rows)
    (OUT/'map.json').write_text(json.dumps(result,indent=2)+'\n')
    (OUT/'instructions.txt').write_text('\n'.join(f'{s}:{o:04x} {i.mnemonic} {i.op_str}' for s in sorted(w.insn_at) for o,i in sorted(w.insn_at[s].items()))+'\n')
    print(json.dumps({k:v for k,v in result.items() if k in ('coverage','a5')},indent=2))
    print('STATIC MAP GENERATED',len(result['traps']),'trap sites;',len(result['unresolved_transfers']),'unresolved transfers')
    return result

if __name__=='__main__':build()
