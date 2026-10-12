#!/usr/bin/env python3
"""Decode positively completed, read-only Macintosh File Manager observations."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path
from report_reference import analyze, require
from extract_original_data import DESTINATION, verify_output


def report(directory):
    log=(directory/'session.log').read_text()
    analyze(log)  # The observer must retain the ordinary original-run proof.
    calls=re.findall(r'^FILE_CALL id=(\d+) pc=([0-9A-F]+) trap=([0-9A-F]+) selector=([0-9A-F]+) pb=([0-9A-F]+)$',log,re.M)
    returns=re.findall(r'^FILE_RETURN id=(\d+) result=([0-9A-F]+)$',log,re.M)
    complete=re.findall(r'^FILE_TRACE_COMPLETE calls=(\d+)$',log,re.M)
    require(len(complete)==1 and int(complete[0])==len(calls)>0,'FILE COMPLETION')
    require([int(c[0]) for c in calls]==list(range(1,len(calls)+1)),'FILE CALL ORDER')
    require([int(r[0]) for r in returns]==list(range(1,len(calls)+1)),'FILE RETURN ORDER')
    maps=[(int(s),int(b,16),int(n)) for s,b,n in re.findall(r'^MAP seg=(\d+) base=([0-9A-F]+) size=(\d+)$',log,re.M)]
    result=[]
    for call,returned in zip(calls,returns):
        serial,pc,trap,selector,_=call
        pc=int(pc,16)&0xffffff;trap=int(trap,16)
        before=(directory/f'file-{serial}-before.bin').read_bytes()
        after=(directory/f'file-{serial}-after.bin').read_bytes()
        require(len(before)==len(after)==128,'FILE PARAMETER BLOCK')
        error=struct.unpack_from('>h',after,16)[0]
        require(error&0xffff==int(returned[1],16),'FILE RETURN RESULT')
        loc=[f'CODE {s:02d}+{pc-b:04X}' for s,b,n in maps if b<=pc<b+n]
        require(len(loc)<=1,'FILE CALL LOCATION')
        row={'id':int(serial),'caller':loc[0] if loc else 'system','trap':f'{trap:04X}',
             'selector':selector,'error':error,'ref':struct.unpack_from('>h',before,24)[0]}
        name=directory/f'file-{serial}-name.bin'
        if name.exists() and (trap in (0xa000,0xa200,0xa004,0xa005,0xa006,0xa009) or
                              trap in (0xa060,0xa260) and int(selector,16)==26):
            row['name']=name.read_bytes().decode('mac_roman')
        row['returned_ref']=struct.unpack_from('>h',after,24)[0]
        if trap in (0xa002,0xa003):
            count=struct.unpack_from('>I',before,36)[0]
            actual=struct.unpack_from('>I',after,40)[0]
            payload=(directory/f'file-{serial}-transfer.bin').read_bytes()
            require(len(payload)==actual<=count,'FILE TRANSFER LENGTH')
            row.update(request=count,actual=actual,mode=struct.unpack_from('>H',before,44)[0],
                       offset=struct.unpack_from('>i',before,46)[0],
                       position=struct.unpack_from('>i',after,46)[0],
                       sha256=hashlib.sha256(payload).hexdigest())
            if trap==0xa003:
                requested=(directory/f'file-{serial}-requested.bin').read_bytes()
                require(len(requested)==count and requested[:actual]==payload,'FILE WRITE SOURCE')
        result.append(row)
    verify_transfers(directory,result)
    (directory/'files.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f'FILE SURVEY PASS: {len(result)} paired calls; {sum(r["caller"]!="system" for r in result)} from original CODE')
    return result


def verify_transfers(directory,rows):
    """Resolve original opens and prove every captured transfer against pinned forks.

    This proves the opening workload only, not native service implementation or
    save persistence. Mutable copies model writes without changing installed data.
    """
    verify_output(DESTINATION)
    opened={};contents={};read_paths=set();writes=0
    for row in rows:
        trap=int(row['trap'],16);serial=row['id'];ref=row['ref']
        before=(directory/f'file-{serial}-before.bin').read_bytes()
        after=(directory/f'file-{serial}-after.bin').read_bytes()
        opening=trap in (0xa000,0xa200) or trap in (0xa060,0xa260) and int(row['selector'],16)==26
        if opening:
            name=row.get('name','')
            relative='Data/'+name.removeprefix(':Data:')
            require('/' not in name and name.count(':') in (0,2),'FILE OPEN PATH')
            path=DESTINATION/relative
            if row['error']:
                require(row['error']==-43 and not path.exists(),'FILE OPEN ERROR')
                continue
            require(path.is_file(),'FILE OPEN SOURCE')
            contents.setdefault(relative,bytearray(path.read_bytes()))
            opened[row['returned_ref']]={'path':relative,'position':0}
        elif trap==0xa001:
            require(row['error']==0 and ref in opened,'FILE CLOSE')
            del opened[ref]
        elif trap==0xa011:
            require(row['error']==0 and ref in opened,'FILE EOF REF')
            require(struct.unpack_from('>I',after,28)[0]==len(contents[opened[ref]['path']]),'FILE EOF SIZE')
        elif trap in (0xa044,0xa002,0xa003):
            require(ref in opened,'FILE TRANSFER REF')
            stream=opened[ref];body=contents[stream['path']]
            mode=struct.unpack_from('>H',before,44)[0]
            offset=struct.unpack_from('>i',before,46)[0]
            require(mode in (0,1,2,3),'FILE POSITION MODE')
            position=(stream['position'] if mode==0 else offset if mode==1 else
                      len(body)+offset if mode==2 else stream['position']+offset)
            require(0<=position<=len(body),'FILE POSITION RANGE')
            if trap==0xa044:
                require(row['error']==0,'FILE SEEK ERROR')
            else:
                payload=(directory/f'file-{serial}-transfer.bin').read_bytes()
                if trap==0xa002:
                    expected=bytes(body[position:position+row['request']])
                    require(payload==expected,'FILE READ BYTES')
                    require(row['error']==(0 if len(expected)==row['request'] else -39),'FILE READ ERROR')
                    read_paths.add(stream['path'])
                else:
                    require(row['error']==0 and len(payload)==row['request'],'FILE WRITE RESULT')
                    body[position:position+len(payload)]=payload;writes+=1
                position+=len(payload)
                row['path']=stream['path']
            require(struct.unpack_from('>i',after,46)[0]==position,'FILE RETURNED POSITION')
            stream['position']=position
    require({'Data/Prince2.opt','Data/Mohawk.ini','Data/MIDISnd.dat','Data/DigiSnd.dat',
             'Data/NISMIDI.dat','Data/NISDIGI.dat'}<=read_paths,'FILE WORKLOAD COVERAGE')
    require(writes>0,'FILE WRITE CONTROL')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    report(parser.parse_args().directory)
