#!/usr/bin/env python3
"""Validate a completed original reference; no timeout or emulator exit is a pass."""
import hashlib
import json
from pathlib import Path
import re
import struct
import zlib
from extract_original_data import ROOT, pins


def require(condition, message):
    if not condition:
        raise ValueError('REFERENCE / '+message)


def analyze(log):
    require('FAIL REFERENCE' not in log and '[LUA ERROR]' not in log, 'SESSION ERROR')
    require(log.count('MILESTONE opening-text-1')==1, 'MILESTONE COUNT')
    require(log.count('REFERENCE_CAPTURED opening-text-1')==1, 'CAPTURE COMPLETION')
    require('DISPLAY_CONFIRMED 640x480x8 mdc48' in log, 'DISPLAY PROOF')
    require(log.count('SEED old=0000 fixed=1234')==1 and log.count('SEED_RESULT value=00001234')==1, 'SEED PROOF')
    require('PACK_CHECK flags=0000/0001' in log and 'PACKING_ENTER' not in log, 'PACKING BRANCH')
    require('COPY_ROUTE nis=0007' in log and 'COPY_ENTER' not in log, 'COPY PROTECTION BRANCH')
    maps=[(int(s),int(b,16),int(n)) for s,b,n in re.findall(r'^MAP seg=(\d+) base=([0-9A-F]+) size=(\d+)$',log,re.M)]
    require({1,2,11,15,17,20,21}.issubset({s for s,_,_ in maps}), 'SEGMENT MAP')
    def location(pc):
        pc &= 0xffffff
        found=[(s,pc-b) for s,b,n in maps if b<=pc<b+n]
        require(len(found)<=1, 'AMBIGUOUS SEGMENT')
        return found[0] if found else None
    pending={};gestalt={};envs=[];memory=[];trace=[];sound=[];randoms=0
    for line in log.split('MILESTONE opening-text-1')[0].splitlines():
        fields={k:int(v,16) for k,v in re.findall(r'\b(\w+)=([0-9A-F]+)\b',line)}
        if line.startswith(('SERVICE pc=','TRAP pc=')):
            pc,trap=fields['pc'],fields['trap']
            selector=fields.get('selector',fields.get('d0'))
            if line.startswith('SERVICE pc='):
                pending[(pc+2,fields['sp']+8)]=(pc,trap,selector)
            if trap==0xa800:sound.append(selector)
            loc=location(pc)
            if loc:
                seg,offset=loc
                # Only semantic selectors, never volatile registers/pointers/ticks.
                suffix=f' selector={selector:08X}' if trap in (0xa1ad,0xa090,0xa05c,0xa800) else ''
                trace.append(f'CODE {seg:02d}+{offset:04X} trap={trap:04X}{suffix}\n')
                if trap==0xa861:randoms+=1
        elif line.startswith('SERVICE_RETURN '):
            entry=pending.pop((fields['pc'],fields['sp']),None)
            if not entry:continue  # A return probe may also be visited outside the guarded application.
            pc,trap,selector=entry
            if location(pc) is None:continue
            if trap==0xa1ad:
                result=(fields['d0']&0xffff,fields['a0'])
                require(selector not in gestalt or gestalt[selector]==result,'GESTALT CHANGED')
                gestalt[selector]=result
            elif trap==0xa090:
                require(fields['d0']&0xffff==0,'SYSENV ERROR')
                envs.append(re.search(r'env=([0-9A-F/]+)',line)[1])
            elif trap==0xa05c:
                memory.append((selector,fields['d0']&0xffff))
    expected={b'vm  ':0,b'proc':4,b'sysv':0x755,b'mach':11,b'mmu ':3,b'addr':4}
    for selector,value in expected.items():
        require(gestalt.get(int.from_bytes(selector,'big'))==(0,value),'GESTALT '+repr(selector))
    require(envs and set(envs)=={'00010009/07550004/01010005/003A8053'},'SYSENV RECORD')
    require(memory and all(pair==(2,0) for pair in memory),'MEMORYDISPATCH RESULT')
    require(sound and all(s&0xffff!=4 for s in sound) and 'MIDI_BRANCH input=00 output=00' in log,'MIDI ROUTE')
    require(any(s==0x000c0008 for s in sound),'SOUND DISPATCH POSITIVE CONTROL')
    require(randoms==1,'RANDOM TRAP CONTROL')
    facts={'gestalt':{k.to_bytes(4,'big').decode('mac_roman'):{'error':v[0],'value':v[1]} for k,v in sorted(gestalt.items())},
           'sysenvirons':envs[0], 'memory_dispatch_calls':len(memory),
           'midi_manager_calls':0,'sound_dispatch_calls':len(sound),'random_calls':randoms,
           'milestone':'opening-text-1','original_code_traps':len(trace)}
    return ''.join(trace).encode(),facts


def png(rgb):
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    rows=b''.join(b'\0'+rgb[y*1920:(y+1)*1920] for y in range(480))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',640,480,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b'')


def report(directory,profile="opening"):
    require(profile in ("opening","file-restart","display"),"RESULT PROFILE")
    trace,facts=analyze((directory/'session.log').read_text())
    frame=(directory/'opening.ppm').read_bytes()
    require(frame.startswith(b'P6\n640 480\n255\n') and len(frame)==921615,'CAPTURE FORMAT')
    outputs={'opening.ppm':frame,'traps.txt':trace}
    expected=pins(ROOT/('manifest/reference-results.sha256' if profile in ('opening','display') else 'manifest/reference-file-restart.sha256'))
    # Preserve useful diagnostic output even when the pinned comparison fails.
    (directory/'traps.txt').write_bytes(trace)
    (directory/'opening.png').write_bytes(png(frame[15:]))
    facts['sha256']={name:hashlib.sha256(data).hexdigest() for name,data in outputs.items()}
    (directory/'report.json').write_text(json.dumps(facts,indent=2)+'\n')
    if profile=='display':
        log=(directory/'session.log').read_text()
        calls=re.findall(r'^DISPLAY_CALL id=(\d+) ',log,re.M)
        returns=re.findall(r'^DISPLAY_RETURN id=(\d+) ',log,re.M)
        require(calls==returns and calls==[str(i) for i in range(1,len(calls)+1)],'DISPLAY PAIRED CALLS')
        require(log.count(f'DISPLAY_TRACE_COMPLETE calls={len(calls)}')==1,'DISPLAY COMPLETION')
    for name,data in outputs.items():
        if profile=='display' and name=='traps.txt':continue # observed waits depend on observer pauses
        require(name in expected and hashlib.sha256(data).hexdigest()==expected[name],'RESULT HASH '+name)
    print('REFERENCE PASS:',directory.relative_to(ROOT),facts['milestone'])
    return facts

if __name__=='__main__':
    import sys
    report(Path(sys.argv[1]))
