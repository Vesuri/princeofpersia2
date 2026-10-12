#!/usr/bin/env python3
"""Pair native Macintosh file-service returns and payloads with original calls."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path
from check_core_result import check
from extract_original_data import ROOT, pins
from report_reference import report, require


def verify(run,core_reference):
    check(run,core_reference)
    metadata=json.loads((run/'dh1/file-replay.json').read_text())
    reference=ROOT/metadata['reference']
    report(reference,'file-restart' if metadata['phase']=='restart' else 'opening')
    original_input=(run/'dh1/file-replay.bin').read_bytes()
    require(hashlib.sha256(original_input).hexdigest()==metadata['sha256']==pins(ROOT/'manifest/file-replays.sha256')[metadata['phase']+'.bin'],'REPLAY INPUT HASH')
    output=(run/'file-replay-out.bin').read_bytes()
    log=(run/'gdb-out.log').read_text()
    complete=re.findall(r'^FILE REPLAY completed=(\d+) count=(\d+) size=(\d+)$',log,re.M)
    count=len(metadata['records'])
    require(complete==[(str(count),str(count),str(len(output)))],'REPLAY POSITIVE COMPLETION')
    require(output[:8]==original_input[:8]==b'PFR1'+struct.pack('>I',count),'REPLAY HEADER')
    require(len(output)==len(original_input),'REPLAY OUTPUT SIZE')
    volumes=re.findall(r'^FILE VOLUME index=(\d+) blocks=(\d+) used=(\d+) blockbytes=(\d+) created=(\d+) modified=(\d+) locked=(\d+)$',log,re.M)
    volume_queries=0
    at=8;events=[];slots={};writes={};dates=None;finder={}
    names=sorted(name[:-6] for name in pins(ROOT/'manifest/runtime-data.sha256') if name.endswith('.finfo'))
    file_ids={name:5+i for i,name in enumerate(names)}
    for row in metadata['records']:
        trap,selector,length,flags,slot,result=struct.unpack_from('>HHIHHI',output,at)
        require(output[at:at+12]==original_input[at:at+12],'REPLAY RECORD IDENTITY')
        require(trap==int(row['trap'],16) and 400<=length<=len(output)-at,'REPLAY RECORD EXTENT')
        pb=output[at+16:at+144];payload=output[at+400:at+length]
        error=struct.unpack_from('>h',pb,16)[0]
        require(error==row['error'] and result&0xffff==error&0xffff,'REPLAY PAIRED ERROR '+str(row['id']))
        if row['id'] is not None:
            before=(reference/f'file-{row["id"]}-before.bin').read_bytes()
            after=(reference/f'file-{row["id"]}-after.bin').read_bytes()
            require(error==struct.unpack_from('>h',after,16)[0],'REPLAY ORIGINAL ERROR')
            if flags&2 and not error:slots[slot]=row['name'].removeprefix(':Data:')
            if trap==0xa044 and not error:require(pb[46:50]==after[46:50],'REPLAY SEEK POSITION')
            if trap==0xa011 and not error:require(pb[28:32]==after[28:32],'REPLAY EOF')
            if trap in (0xa002,0xa003):
                require(slot in slots,'REPLAY OPEN STREAM')
                require(pb[40:44]==after[40:44] and pb[46:50]==after[46:50],'REPLAY ACTUAL/POSITION')
                actual=int.from_bytes(pb[40:44],'big')
                captured=(reference/f'file-{row["id"]}-transfer.bin').read_bytes()
                require(payload[:actual]==captured and actual==len(captured),'REPLAY PAIRED PAYLOAD')
                if trap==0xa002:require(payload[actual:]==original_input[at+400+actual:at+length],'REPLAY READ OVERRUN')
                else:
                    require(payload==original_input[at+400:at+length],'REPLAY WRITE SOURCE PRESERVED')
                    writes[slots[slot]]=captured
                events.append({'name':slots[slot],'trap':row['trap'],'sha256':hashlib.sha256(captured).hexdigest(),'bytes':actual})
            if trap==0xa260 and selector==8 and not error:
                for offset,size in ((36,2),(40,4),(48,4)):
                    require(pb[offset:offset+size]==after[offset:offset+size],'REPLAY FCB FLAGS/LENGTH/MARK')
                require(pb[52:54]==b'\xff\xff' and int.from_bytes(pb[58:62],'big')==4,'REPLAY FCB VOLUME/PARENT')
            if trap==0xa260 and selector==9 and not error:
                directory=bool(after[30]&16)
                require(bool(pb[30]&16)==directory,'REPLAY CATALOG KIND')
                if directory:
                    native_id=int.from_bytes(pb[48:52],'big')
                    expected={2:(1,b'Runtime',1),3:(2,b'Prince of Persia 2',2),4:(3,b'Data',31)}
                    require(native_id in expected,'REPLAY CATALOG DIRECTORY')
                    parent,name,children=expected[native_id]
                    require(int.from_bytes(pb[100:104],'big')==parent and int.from_bytes(pb[52:54],'big')==children,'REPLAY CATALOG HIERARCHY')
                    if flags&4:require(output[at+144:at+145+len(name)]==bytes([len(name)])+name,'REPLAY DIRECTORY NAME')
                else:
                    for offset,size in ((30,18),(54,4),(64,4),(72,28)):
                        require(pb[offset:offset+size]==after[offset:offset+size],'REPLAY CATALOG FILE FACTS')
                    returned=(reference/f'file-{row["id"]}-returned-name.bin').read_bytes()
                    relative='Data/'+returned.decode('mac_roman')
                    require(int.from_bytes(pb[48:52],'big')==file_ids[relative] and int.from_bytes(pb[100:104],'big')==4,'REPLAY CATALOG FILE IDENTITY')
            if flags&4 and not error and trap in (0xa014,0xa214,0xa207):
                require(output[at+144:at+152]==b'\x07Runtime','REPLAY VOLUME NAME')
            if trap in (0xa014,0xa214) and not error:
                require(struct.unpack_from('>h',pb,22)[0]==-32000,'REPLAY DEFAULT VOLUME')
                if trap==0xa214:require(pb[32:34]==b'\xff\xff' and int.from_bytes(pb[48:52],'big')==3,'REPLAY DEFAULT DIRECTORY')
            if trap==0xa207 and not error:
                require(volume_queries<len(volumes) and int(volumes[volume_queries][0])==volume_queries,'REPLAY VOLUME BACKING EVIDENCE')
                _,blocks,used,blockbytes,created,modified,locked=map(int,volumes[volume_queries]);volume_queries+=1;group=1
                while blocks//group>65535:group*=2;blockbytes*=2
                require(0<=used<=blocks and blockbytes>0,'REPLAY BACKING CAPACITY')
                word=lambda offset:int.from_bytes(pb[offset:offset+2],'big')
                lng=lambda offset:int.from_bytes(pb[offset:offset+4],'big')
                requested=struct.unpack_from('>h',original_input,at+16+22)[0]
                require(word(22)==65535 and word(38)==(128 if locked else 0) and word(40)==(0 if requested==-1 else 2),'REPLAY VOLUME IDENTITY/ATTRIBUTES')
                require(word(46)==blocks//group and lng(48)==blockbytes and word(62)==(blocks-used)//group,'REPLAY VOLUME CAPACITY')
                require(lng(30)==created and lng(34)==modified and lng(82)==32 and lng(86)==2 and lng(90)==0 and lng(98)==3,'REPLAY VOLUME CATALOG')
            if flags&4 and not error and (trap==0xa260 and (selector==8 or selector==9 and not (after[30]&16))):
                returned=(reference/f'file-{row["id"]}-returned-name.bin').read_bytes()
                require(output[at+144:at+145+len(returned)]==bytes([len(returned)])+returned,'REPLAY CATALOG RETURNED NAME')
            if trap==0xa00c and not error:
                dates=(row['name'],pb[72:80])
                for offset,size in ((30,18),(54,4),(64,4)):
                    require(pb[offset:offset+size]==after[offset:offset+size],'REPLAY FILE METADATA')
            if trap==0xa00d and not error:
                require(dates==(row['name'],pb[72:80]),'REPLAY METADATA DATE DEPENDENCY')
                require(pb[32:48]==before[32:48],'REPLAY FINDER INPUT')
                finder[row['name'].removeprefix(':Data:')]=pb[32:48]+pb[72:80]
            if trap==0xa001 and not error:slots.pop(slot)
        at+=length
    require(at==len(output),'REPLAY TRAILING BYTES')
    if metadata['phase']=='opening':
        require(len(volumes)==volume_queries>0 and re.findall(r'^FILE VOLUMES count=(\d+)$',log,re.M)==[str(volume_queries)],'REPLAY VOLUME QUERY COVERAGE')
        require({'Prince2.opt','Mohawk.ini','MIDISnd.dat','DigiSnd.dat','NISMIDI.dat','NISDIGI.dat'}<={e['name'] for e in events if e['trap']=='A002'},'REPLAY ARCHIVE COVERAGE')
    for name,body in writes.items():
        require((run/'dh1/Data'/name).read_bytes()==body,'REPLAY FLUSHED WRITE '+name)
    for name,fields in finder.items():
        encoded=(run/'dh1/Data'/(name+'.finfo')).read_bytes()
        require(len(encoded)==32 and encoded[:4]==b'AFI1','REPLAY METADATA FORMAT')
        checksum=2166136261
        for byte in encoded[:28]:checksum=((checksum^byte)*16777619)&0xffffffff
        require(encoded[28:]==struct.pack('>I',checksum),'REPLAY METADATA CHECKSUM')
        require(encoded[4:24]==fields[:20] and int.from_bytes(encoded[24:28],'big')>=int.from_bytes(fields[20:24],'big'),'REPLAY PERSISTED METADATA '+name)
    if metadata['phase']=='save':
        require({'Prince.hof','Fixture Save'}<=set(writes),'REPLAY SAVE COVERAGE')
        manifest={}
        for name in ('Prince2.opt','Prince.hof','Fixture Save'):
            for suffix in ('','.rsrc','.finfo'):
                relative='Data/'+name+suffix
                manifest[relative]=hashlib.sha256((run/'dh1'/relative).read_bytes()).hexdigest()
        saved=run/'native-persistence.json'
        if saved.exists():require(json.loads(saved.read_text())==manifest,'REPLAY SAVED STATE CHANGED')
        else:saved.write_text(json.dumps(manifest,indent=2)+'\n')
    (run/'file-replay-report.json').write_text(json.dumps({'phase':metadata['phase'],'calls':count,'events':events},indent=2)+'\n')
    print(f'NATIVE FILE REPLAY PASS: {count} paired calls; {len(events)} exact transfers; {metadata["phase"]}')
    return events


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path);p.add_argument('core_reference',type=Path)
    a=p.parse_args();verify(a.run.resolve(),a.core_reference.resolve())
