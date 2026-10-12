#!/usr/bin/env python3
"""Build ignored native service inputs from positively completed original calls."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path
from extract_original_data import ROOT, pins
from report_reference import report, require


def generate(reference,phase):
    report(reference,'file-restart' if phase=='restart' else 'opening')
    log=(reference/'session.log').read_text()
    if phase!='opening':
        marker='ResumeGame restart-read' if phase=='restart' else 'ResumeGame readback'
        require('FILE_ROUTINE_COMPLETE '+marker in log and 'FILE_ROUTINE_COMPLETE volume-flushed' in log,'REPLAY ORIGINAL COMPLETION')
    opening=int(re.search(r'FILE_TRACE_COMPLETE calls=(\d+)',log)[1])
    calls=re.findall(r'^FILE_CALL id=(\d+) pc=\w+ trap=([0-9A-F]+) selector=([0-9A-F]+) pb=\w+$',log,re.M)
    directories={0:0,2:2}
    if phase=='opening':
        from report_files import report as file_report
        file_report(reference)
        for serial,trap,selector in calls:
            if trap!='A260' or selector!='0009':continue
            before=(reference/f'file-{serial}-before.bin').read_bytes();after=(reference/f'file-{serial}-after.bin').read_bytes()
            if struct.unpack_from('>h',after,16)[0]:continue
            returned=reference/f'file-{serial}-returned-name.bin'
            name=returned.read_bytes() if returned.exists() else b''
            if name==b'Prince of Persia 2':directories[int.from_bytes(after[48:52],'big')]=3
            elif name==b'Desktop Folder':directories[int.from_bytes(after[48:52],'big')]=2
            elif name==b'Mohawk.ini':directories[int.from_bytes(after[100:104],'big')]=4
    records=[];descriptions=[];refs={};slot=0;info_dates=None
    for serial,trap,selector in calls:
        # The opening options transaction is paired as well as the routines
        # after the checkpoint. Volume-capacity queries are a separate probe.
        if phase=='opening':
            if int(serial)>opening:continue
        elif int(serial)<=opening and int(serial) not in (1,3,4,5,6,7,9,10,11,12):continue
        trap=int(trap,16);selector=int(selector,16)
        require(trap in (0xa014,0xa214,0xa015,0xa008,0xa00c,0xa00d,0xa060,0xa001,0xa002,0xa003,0xa011,0xa200,0xa207,0xa260,0xa044),'REPLAY TRAP COVERAGE')
        pb=bytearray((reference/f'file-{serial}-before.bin').read_bytes())
        after=(reference/f'file-{serial}-after.bin').read_bytes()
        error=struct.unpack_from('>h',after,16)[0]
        flags=0;current=0;payload=b''
        selector=selector if trap in (0xa060,0xa260) else 0
        name_path=reference/f'file-{serial}-name.bin'
        name=name_path.read_bytes() if name_path.exists() else b''
        require(len(name)<256,'REPLAY NAME LENGTH')
        if name or (int.from_bytes(pb[18:22],'big') and trap in (0xa014,0xa214,0xa207,0xa260)):flags|=4
        if trap in (0xa060,0xa200):
            require(trap==0xa200 or selector==26,'REPLAY OPEN SELECTOR');slot+=1;current=slot;flags|=2
            if not error:refs[struct.unpack_from('>h',after,24)[0]]=slot
        if trap in (0xa001,0xa002,0xa003,0xa011,0xa044) or trap==0xa260 and selector==8:
            original_ref=struct.unpack_from('>h',pb,24)[0]
            require(original_ref in refs,'REPLAY REFERENCE NUMBER')
            current=refs[original_ref];flags|=1
            if trap==0xa001:del refs[original_ref]
        if trap in (0xa002,0xa003):
            flags|=8;count=struct.unpack_from('>I',pb,36)[0]
            require(count<=1024*1024,'REPLAY BUFFER CAPACITY')
            payload=(reference/f'file-{serial}-requested.bin').read_bytes() if trap==0xa003 else b'\xcd'*count
            require(len(payload)==count,'REPLAY WRITE SIZE')
        # Preserve documented inputs, not unused bytes from the caller's stack.
        # Macintosh reference numbers/pointers are bound to native values below.
        original=pb;pb=bytearray(128)
        if trap in (0xa060,0xa200):pb[27]=original[27]
        if trap==0xa200 or trap==0xa260 and selector==9:
            source_directory=int.from_bytes(original[48:52],'big')
            require(source_directory in directories,'REPLAY DIRECTORY IDENTITY')
            pb[48:52]=struct.pack('>I',directories[source_directory])
        if trap==0xa260:pb[28:30]=original[28:30]
        if trap==0xa207:
            pb[28:30]=original[28:30]
            volume=struct.unpack_from('>h',original,22)[0]
            pb[22:24]=struct.pack('>h',volume if volume in (0,-1) else -32000)
        if trap==0xa044:pb[44:50]=original[44:50]
        if trap in (0xa002,0xa003):
            pb[36:40]=original[36:40];pb[44:50]=original[44:50]
        if trap==0xa008:pb[26]=original[26]
        if trap==0xa00c:
            pb[28:30]=original[28:30]
            if not error:info_dates=(name,after[72:80])
        if trap==0xa00d:
            require(info_dates==(name,original[72:80]),'REPLAY SET METADATA DATE DEPENDENCY')
            pb[32:48]=original[32:48];flags|=16
        if trap==0xa015:pb[22:24]=struct.pack('>h',-32000)
        if len(payload)&1:payload+=b"\0"
        record=bytearray(struct.pack('>HHIHHI',trap,selector,400+len(payload),flags,current,0xdeadbeef))
        record+=pb+bytes([len(name)])+name+b'\0'*(255-len(name))+payload
        records.append(record)
        descriptions.append({'id':int(serial),'trap':f'{trap:04X}','error':error,'name':name.decode('mac_roman'),'slot':current})
    # Explicit native flush call, paired to the original's positive flush marker.
    records.append(struct.pack('>HHIHHI',0xa013,0,400,0,0,0xdeadbeef)+bytes(384))
    descriptions.append({'id':None,'trap':'A013','error':0,'name':'','slot':0})
    require(slot<256 and records,'REPLAY CAPACITY')
    data=b'PFR1'+struct.pack('>I',len(records))+b''.join(records)
    output=ROOT/'build/core';output.mkdir(parents=True,exist_ok=True)
    digest=hashlib.sha256(data).hexdigest()
    require(digest==pins(ROOT/'manifest/file-replays.sha256').get(phase+'.bin'),'REPLAY PINNED INPUT '+digest)
    (output/'file-replay.bin').write_bytes(data)
    (output/'FileReplay.h').write_text('static const uint8_t fileReplaySHA256[32]={'+','.join('0x'+digest[i:i+2] for i in range(0,64,2))+'};\n')
    (output/'file-replay.json').write_text(json.dumps({'sha256':digest,'phase':phase,'reference':str(reference.relative_to(ROOT)),'records':descriptions},indent=2)+'\n')
    print(f'FILE REPLAY INPUT: {phase}; {len(records)} original-service calls; {digest}')


def verify():
    output=ROOT/'build/core';metadata=json.loads((output/'file-replay.json').read_text())
    digest=hashlib.sha256((output/'file-replay.bin').read_bytes()).hexdigest()
    require(metadata['phase'] in ('save','restart','opening'),'REPLAY PROFILE')
    require(digest==metadata['sha256']==pins(ROOT/'manifest/file-replays.sha256')[metadata['phase']+'.bin'],'REPLAY PINNED INPUT')
    header='static const uint8_t fileReplaySHA256[32]={'+','.join('0x'+digest[i:i+2] for i in range(0,64,2))+'};\n'
    require((output/'FileReplay.h').read_text()==header,'REPLAY COMPILED HASH')
    print('FILE REPLAY BUILD INPUT PASS')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('reference',type=Path,nargs='?');p.add_argument('phase',choices=['save','restart','opening'],nargs='?');p.add_argument('--verify',action='store_true')
    a=p.parse_args()
    if a.verify:verify()
    elif a.reference and a.phase:generate(a.reference.resolve(),a.phase)
    else:p.error('reference and phase are required')
