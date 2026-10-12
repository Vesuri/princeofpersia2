#!/usr/bin/env python3
"""Prove paired native writes survive a fresh emulator boot as the original does."""
import argparse
import hashlib
import json
from pathlib import Path
from check_native_file_replay import verify
from report_reference import require


def check(written,restarted,core_reference):
    require(written!=restarted,'NATIVE PERSISTENCE FRESH RUN')
    source=verify(written,core_reference);target=verify(restarted,core_reference)
    saved=json.loads((written/'native-persistence.json').read_text())
    require(json.loads((restarted/'dh1/restart-state.json').read_text())==saved,'NATIVE PERSISTENCE IMPORT MANIFEST')
    for relative,digest in saved.items():
        require(relative.startswith('Data/') and '..' not in Path(relative).parts,'NATIVE PERSISTENCE PATH')
        require(hashlib.sha256((written/'dh1'/relative).read_bytes()).hexdigest()==digest,'NATIVE PERSISTENCE SAVED HASH')
        if relative!='Data/Prince2.opt.finfo':
            require(hashlib.sha256((restarted/'dh1'/relative).read_bytes()).hexdigest()==digest,'NATIVE PERSISTENCE RESTARTED HASH')
        else:
            before=(written/'dh1'/relative).read_bytes();after=(restarted/'dh1'/relative).read_bytes()
            require(len(after)==32 and after[:4]==b'AFI1','NATIVE PERSISTENCE OPTIONS METADATA FORMAT')
            checksum=2166136261
            for byte in after[:28]:checksum=((checksum^byte)*16777619)&0xffffffff
            require(int.from_bytes(after[28:],'big')==checksum,'NATIVE PERSISTENCE OPTIONS METADATA CHECKSUM')
            require(before[:24]==after[:24] and int.from_bytes(after[24:28],'big')>=int.from_bytes(before[24:28],'big'),'NATIVE PERSISTENCE OPTIONS METADATA')
    for name in ('Prince2.opt','Prince.hof','Fixture Save'):
        writes=[e for e in source if e['name']==name and e['trap']=='A003']
        reads=[e for e in target if e['name']==name and e['trap']=='A002']
        require(len(writes)==1 and reads,'NATIVE PERSISTENCE COVERAGE '+name)
        require(any(r['sha256']==writes[0]['sha256'] and r['bytes']==writes[0]['bytes'] for r in reads),'NATIVE PERSISTENCE CONTENT '+name)
    print('NATIVE PERSISTENCE PASS: paired options, Hall of Fame and SaveGame data survive a fresh Amiga boot')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('written','restarted','core_reference'):p.add_argument(name,type=Path)
    a=p.parse_args();check(a.written.resolve(),a.restarted.resolve(),a.core_reference.resolve())
