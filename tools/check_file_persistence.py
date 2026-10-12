#!/usr/bin/env python3
"""Prove original-written save/options/Hall forks are read after a fresh boot."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
from report_reference import report, require


def operations(directory):
    log=(directory/'session.log').read_text()
    calls=re.findall(r'^FILE_CALL id=(\d+) pc=([0-9A-F]+) trap=([0-9A-F]+) selector=([0-9A-F]+) pb=([0-9A-F]+)$',log,re.M)
    returns=re.findall(r'^FILE_RETURN id=(\d+) result=([0-9A-F]+)$',log,re.M)
    require([c[0] for c in calls]==[r[0] for r in returns],'PERSISTENCE PAIRED CALLS')
    refs={};events=[]
    for call,ret in zip(calls,returns):
        serial=int(call[0]);trap=int(call[2],16);selector=int(call[3],16)
        before=(directory/f'file-{serial}-before.bin').read_bytes()
        after=(directory/f'file-{serial}-after.bin').read_bytes()
        error=struct.unpack_from('>h',after,16)[0]
        require(error&0xffff==int(ret[1],16),'PERSISTENCE RETURN ERROR')
        ref=struct.unpack_from('>h',before,24)[0]
        opening=trap in (0xa000,0xa200) or trap in (0xa060,0xa260) and selector==26
        if opening and not error:
            name=(directory/f'file-{serial}-name.bin').read_bytes().decode('mac_roman')
            refs[struct.unpack_from('>h',after,24)[0]]=name.removeprefix(':Data:')
        elif trap==0xa001 and not error:refs.pop(ref,None)
        elif trap in (0xa002,0xa003):
            require(ref in refs,'PERSISTENCE OPEN REF')
            actual=struct.unpack_from('>I',after,40)[0]
            data=(directory/f'file-{serial}-transfer.bin').read_bytes()
            require(len(data)==actual,'PERSISTENCE ACTUAL BYTES')
            if trap==0xa003:
                requested=(directory/f'file-{serial}-requested.bin').read_bytes()
                require(data==requested and error==0,'PERSISTENCE WRITE')
            events.append((refs[ref],trap,data,error))
    return events


def check(written,restarted):
    report(written);report(restarted,'file-restart')
    for directory,markers in ((written,('HallOfFame write-readback','SaveGame write','ResumeGame readback')),
                              (restarted,('HallOfFame restart-read','ResumeGame restart-read'))):
        log=(directory/'session.log').read_text()
        for marker in markers+('volume-flushed',):
            require(log.count('FILE_ROUTINE_COMPLETE '+marker)==1,'PERSISTENCE COMPLETION '+marker)
    source=operations(written);restart=operations(restarted)
    manifest=json.loads((restarted/'persisted/sha256.json').read_text())
    for name,digest in manifest.items():
        require(Path(name).name==name,'PERSISTENCE MANIFEST PATH')
        require(hashlib.sha256((restarted/'persisted'/name).read_bytes()).hexdigest()==digest,'PERSISTENCE IMPORT HASH')
    result={}
    for name in ('Prince2.opt','Prince.hof','Fixture Save'):
        writes=[data for path,trap,data,error in source if path==name and trap==0xa003 and not error]
        reads=[data for path,trap,data,error in restart if path==name and trap==0xa002 and not error]
        require(len(writes)==1 and bool(reads),'PERSISTENCE COVERAGE '+name)
        exported=(restarted/'persisted'/(name+'.data')).read_bytes()
        require(writes[0]==exported and exported in reads,'PERSISTENCE CONTENT '+name)
        result[name]={'bytes':len(exported),'sha256':hashlib.sha256(exported).hexdigest()}
    (restarted/'persistence.json').write_text(json.dumps(result,indent=2)+'\n')
    print('ORIGINAL PERSISTENCE PASS: options, Hall of Fame, SaveGame/ResumeGame after fresh boot')
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('written',type=Path);parser.add_argument('restarted',type=Path)
    args=parser.parse_args();check(args.written.resolve(),args.restarted.resolve())
