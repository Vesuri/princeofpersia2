#!/usr/bin/env python3
"""Fail-closed static/live comparison and byte-write classification."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct
from static_map import build, a5world, ROOT, DESTINATION, OUT
import trap_census as c
from driver_image import decrypt, expand
from amigaxdev.mac.resource_fork import read_resource_fork
from report_reference import report

def require(ok,why):
    if not ok:raise ValueError('STATIC/LIVE MAP / '+why)

def driver_map(raw):
    saved=(c.CODE,c.RELOC,c.SEGNAME,c.JT)
    c.CODE={-11:raw};c.RELOC={-11:{}};c.SEGNAME={-11:'MDRV 11'};c.JT=[]
    try:
        w=c.Walker()
        # Dispatcher bounds selector at 38, indexes the adjacent BRA.W vector.
        require(raw[0x2a:0x2e]==bytes.fromhex('0c400026'),'MDRV selector bound')
        require(all(raw[i:i+2]==bytes.fromhex('6000') for i in range(0x3c,0xd4,4)),'MDRV branch vector')
        w.walk(-11,[0]+list(range(0x3c,0xd4,4)))
        while True:
            roots=[t for t in w.codeptrs[-11] if 0<=t<0x4600 and t not in w.seen[-11] and raw[t:t+2] in (bytes.fromhex('48e7'),bytes.fromhex('4e56'))]
            if not roots:break
            w.walk(-11,roots)
        require(not any(i>=0x4600 for i in w.seen[-11]),'MDRV executable/data overlap')
        return w
    finally:c.CODE,c.RELOC,c.SEGNAME,c.JT=saved

def check(directory,mapping):
    directory=directory.resolve()
    report(directory)
    log=(directory/'session.log').read_text()
    for marker in ('LIVE_MAP_COMPLETE','STUBS_COMPLETE','DECRYPT_COMPLETE','MDRV_EXPANDED_COMPLETE','MDRV_INSTALLED base=','WRITE_CONTROL'):
        require(sum(line.startswith(marker) for line in log.splitlines())==1,'completion '+marker)
    require('FAIL' not in log and 'Error in' not in log,'probe failure')
    rs=read_resource_fork(DESTINATION/'Prince of Persia 2.rsrc')
    resources={(r.kind,r.rid):r.body for r in rs}
    a5,strs=[int(x,16) for x in re.search(r'A5_INITIAL a5=(\w+) strs=(\w+)',log).groups()]
    expected,rel,runs=a5world(rs,a5,strs)
    require(expected==(directory/'a5-initial.bin').read_bytes(),'A5 full image')
    encrypted=(directory/'MDRV-encrypted.bin').read_bytes()
    require(encrypted==resources[b'MDRV',11],'encrypted resource identity')
    decrypted=decrypt(encrypted);driver=expand(decrypted)
    require(decrypted==(directory/'MDRV-decrypted.bin').read_bytes(),'decryption model')
    for name in ('MDRV-expanded.bin','MDRV-installed.bin'):
        require(driver==(directory/name).read_bytes(),'expansion model '+name)
    final_driver=(directory/'MDRV-final.bin').read_bytes()
    require(len(final_driver)==len(driver) and driver[:0x4600]==final_driver[:0x4600],'MDRV final executable image')
    code1base=int(re.search(r'LIVE_MAP seg=1 base=(\w+)',log)[1],16)
    mapped_segments={int(s) for s in re.findall(r'^LIVE_MAP seg=(\d+)',log,re.M)}
    for seg in mapped_segments:
        file=directory/f'map-CODE_{seg}-after.bin'
        require(file.is_file(),'missing CODE capture '+str(seg))
        expected=bytearray(resources[b'CODE',seg])
        if seg==1:struct.pack_into('>I',expected,4,strs)
        if seg==19:struct.pack_into('>I',expected,0x478,a5)
        require(file.read_bytes()==expected,'final CODE '+str(seg))
    stubs=(directory/'runtime-stubs.bin').read_bytes()
    require(len(stubs)==24,'stub extent')
    for offset,target in ((0,code1base+0x64),(12,code1base+0x4c)):
        require(stubs[offset:offset+2]==bytes.fromhex('4eb9') and stubs[offset+6:offset+8]==bytes.fromhex('4ef9'),'stub opcodes')
        require(int.from_bytes(stubs[offset+2:offset+6],'big')==target,'stub original handler')
    static={tuple(x) for x in mapping['traps']}
    live={tuple(int(x,b) for x,b in zip(t,(10,16,16))) for t in re.findall(r'CODE (\d+)\+([0-9A-F]+) trap=([0-9A-F]+)',(directory/'traps.txt').read_text())}
    require(live<=static,'unmapped CODE trap '+str(sorted(live-static)))
    installed=re.findall(r'^GETKEYS_INSTALLED target=(\w+)$',log,re.M)
    calls=re.findall(r'^CACHED_GETKEYS pc=(\w+) target=(\w+) slot=(\w+)$',log,re.M)
    require(len(installed)==1 and calls,'cached GetKeys completion')
    known_cached={tuple(x) for x in mapping['cached_services']}
    live_maps=[(int(s),int(b,16),int(n)) for s,b,n in re.findall(r'^MAP seg=(\d+) base=(\w+) size=(\d+)',log,re.M)]
    for pc,target,slot in calls:
        require(target==slot==installed[0],'GetKeys cached address')
        pc=int(pc,16)&0xffffff
        locations=[(s,pc-b,0xa976) for s,b,n in live_maps if b<=pc<b+n]
        require(len(locations)==1 and locations[0] in known_cached,'GetKeys static seam')
    delta=int(re.search(r'WRITE_CONTROL .* delta=(\w+)',log)[1],16)
    scopes={};counts=Counter();seen_decrypt=set();seen_expand=set();seen_stub=set();driverbase=None
    loaded={}; stable=set()
    for line in log.splitlines():
        if line.startswith('WATCH '):
            label,base,size=re.match(r'WATCH (\S+) base=(\w+) size=(\d+)',line).groups()
            scopes[label]=(int(base,16)+delta,int(size))
        elif line.startswith('MAP seg='):
            seg,base,size=re.match(r'MAP seg=(\d+) base=(\w+) size=(\d+)',line).groups()
            stable.add(int(seg))
        elif line.startswith('LIVE_MAP seg='):
            seg,base,size=re.match(r'LIVE_MAP seg=(\d+) base=(\w+) size=(\d+)',line).groups()
            loaded[int(seg)]=(int(base,16),int(size))
        elif line.startswith('MDRV_INSTALLED base='):
            driverbase=int(re.search(r'base=(\w+)',line)[1],16)
        elif line.startswith('CODEWRITE '):
            label,pc,addr,size,data=re.match(r'CODEWRITE (\S+) pc=(\w+) addr=(\w+) size=(\w+) data=(\w+)',line).groups()
            pc,addr,bits,data=(int(x,16) for x in (pc,addr,size,data));base,length=scopes[label];off=addr-base
            require(bits in (8,16,32) and 0<=off and off+bits//8<=length,'write extent')
            loc=next(((s,(pc&0xffffff)-b) for s,(b,n) in loaded.items() if b<=pc&0xffffff<b+n),None)
            if label=='STUB':
                require(loc and loc[0]==1 and 0x3d2<=loc[1]<=0x402,'stub writer')
                require(data==int.from_bytes(stubs[off:off+bits//8],'big'),'stub write value')
                seen_stub.update(range(off,off+bits//8))
                counts['runtime-stub construction']+=1
            elif label=='MDRV':
                require(loc==(11,0x21ee) and bits==8 and 7<=off<len(encrypted),'decrypt writer')
                require(off not in seen_decrypt and data==decrypted[off],'decrypt byte');seen_decrypt.add(off)
                counts['MDRV decryption']+=1
            elif label=='MDRV_EXPANDED':
                require(loc in ((11,0x3886),(11,0x38b2)) and bits==8,'expansion writer')
                require(off not in seen_expand and data==driver[off],'expansion byte');seen_expand.add(off)
                counts['MDRV LZSS expansion']+=1
            elif label=='MDRV_INSTALLED':
                require(0x4600<=off<length,'MDRV write to executable prefix')
                counts['MDRV mutable data']+=1
            elif label.startswith('CODE_'):
                seg=int(label[5:])
                if loc==(1,0x88) and off==0 and bits==8:
                    require(data==resources[b'CODE',seg][0],'near header unchanged')
                    counts['near-header BCLR (unchanged)']+=1
                elif seg==19 and off==0x478 and bits==32 and loc==(19,0x486) and data==a5:
                    counts['CODE 19 saved A5 data']+=1
                else:
                    require(seg==15 and seg not in stable and loc is None,'unclassified CODE write '+line)
                    counts['OS heap reuse before relocated CODE 15 publication']+=1
            else:raise ValueError('unknown watch scope')
    require(seen_stub==set(range(24)),'stub write coverage')
    require({'CODE_'+str(s) for s in mapped_segments}<=set(scopes),'missing CODE watch scope')
    require(len(seen_decrypt)==len(encrypted)-7,'decrypt write coverage')
    require(len(seen_expand)==len(driver),'expansion write coverage')
    require(counts['runtime-stub construction']>=8 and counts['CODE 19 saved A5 data']>0 and counts['MDRV mutable data']>0,'write positive controls')
    dw=driver_map(driver);drivertraps={(o,t) for _,o,t,_ in dw.traps}
    livemdrv={(int(pc,16)&0xffffff)-driverbase:int(t,16) for pc,t in re.findall(r'^TRAP pc=(\w+) trap=(\w+)',log,re.M) if driverbase<=int(pc,16)&0xffffff<driverbase+len(driver)}
    require(set(livemdrv.items())<=drivertraps,'unmapped MDRV trap '+str(sorted(set(livemdrv.items())-drivertraps)))
    result={'live_code_sites':len(live),'live_mdrv_sites':len(livemdrv),'writes':dict(counts),
            'driver_code_bytes':len(dw.seen[-11]),'driver_size':len(driver),'driver_unresolved_transfers':len(dw.blind),
            'image_sha256':hashlib.sha256(driver).hexdigest(),'a5_bytes':22266}
    (directory/'map-report.json').write_text(json.dumps(result,indent=2)+'\n')
    (OUT/'driver-map.json').write_text(json.dumps({'traps':sorted(drivertraps),'unresolved':dw.blind},indent=2)+'\n')
    print('STATIC/LIVE MAP PASS',json.dumps(result))
    return result

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('run',type=Path);args=parser.parse_args()
    check(args.run,build())
