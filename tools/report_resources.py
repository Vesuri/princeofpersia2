#!/usr/bin/env python3
"""Verify original resource lookup bodies and metadata against pinned fork contents."""
import argparse
import hashlib
import re
import struct
from pathlib import Path
from extract_original_data import DESTINATION, verify_output
from amigaxdev.mac.resource_fork import read_resource_fork
from report_reference import analyze, require


def report(directory):
    log=(directory/'session.log').read_text()
    analyze(log)
    calls=re.findall(r'^RESOURCE_CALL id=(\d+) pc=([0-9A-F]+) trap=([0-9A-F]+) selector=([0-9A-F]+) current=([0-9A-F]+)$',log,re.M)
    returns=re.findall(r'^RESOURCE_RETURN id=(\d+) error=([0-9A-F]+) d0=([0-9A-F]+)$',log,re.M)
    bodies={int(i):(int(h,16),int(b,16),int(n)) for i,h,b,n in re.findall(r'^RESOURCE_BODY id=(\d+) handle=([0-9A-F]+) body=([0-9A-F]+) size=(\d+)$',log,re.M)}
    complete=re.findall(r'^RESOURCE_TRACE_COMPLETE calls=(\d+)$',log,re.M)
    require(len(complete)==1 and int(complete[0])==len(calls)>0,'RESOURCE COMPLETION')
    order=list(range(1,len(calls)+1))
    require([int(c[0]) for c in calls]==order and [int(r[0]) for r in returns]==order,'RESOURCE CALL/RETURN ORDER')
    verify_output(DESTINATION)
    sources={str(f.relative_to(DESTINATION))[:-5]:{(r.kind,r.rid):r for r in read_resource_fork(f)}
             for f in DESTINATION.rglob('*.rsrc') if f.stat().st_size}
    files={int(calls[0][4],16):'Prince of Persia 2'}
    chain=list(files);handles={};rows=[]
    for call,returned in zip(calls,returns):
        serial,_,trap,_,current=call
        trap=int(trap,16);current=int(current,16);error=int(returned[1],16)
        before=(directory/f'resource-{serial}-before.bin').read_bytes()
        after=(directory/f'resource-{serial}-after.bin').read_bytes()
        require(len(before)==64 and len(after)==16,'RESOURCE ARGUMENT CAPTURE')
        require(current in chain,'RESOURCE CURRENT FILE')
        if trap==0xa997:
            name=(directory/f'resource-{serial}-name.bin').read_bytes().decode('mac_roman').lstrip(':').replace(':','/')
            ref=struct.unpack_from('>h',after)[0]
            require(error==0 and ref>0 and name in sources,'RESOURCE OPEN')
            if ref not in files:chain.insert(0,ref)
            files[ref]=name
        elif trap==0xa99a:
            ref=struct.unpack_from('>h',before)[0]
            require(error==0 and ref in files,'RESOURCE CLOSE')
            del files[ref];chain.remove(ref)
        elif trap==0xa9a0:
            key=(before[2:6],struct.unpack_from('>h',before)[0])
            found=[(files[ref],sources[files[ref]][key]) for ref in chain[chain.index(current):] if key in sources[files[ref]]]
            handle=struct.unpack_from('>I',after)[0]&0xffffff
            if not found:
                # The measured Jnth probe returns nil while this low-memory
                # observation remains zero; do not infer a ResError call result.
                require(key==(b'Jnth',11) and handle==0 and error==0,'RESOURCE MISSING RESULT')
                continue
            require(handle!=0 and error==0,'RESOURCE LOOKUP RESULT')
            source,resource=found[0];handles[handle]=resource
            payload=directory/f'resource-{serial}-payload.bin'
            require(int(serial) in bodies,'RESOURCE BODY OBSERVATION')
            observed_handle,address,size=bodies[int(serial)]
            require(observed_handle==handle and size==len(resource.body),'RESOURCE BODY IDENTITY')
            if address==0:
                require(not payload.exists(),'RESOURCE UNLOADED BODY')
                continue
            body=payload.read_bytes()
            require(body==resource.body,'RESOURCE BODY')
            rows.append({'id':int(serial),'source':source,'type':key[0].decode('ascii'),
                         'rid':key[1],'size':len(body),'sha256':hashlib.sha256(body).hexdigest()})
        elif trap==0xa9a8:
            handle=struct.unpack_from('>I',before,12)[0]&0xffffff
            require(error==0 and handle in handles,'RESOURCE INFO HANDLE')
            resource=handles[handle]
            require((directory/f'resource-{serial}-info-id.bin').read_bytes()==struct.pack('>h',resource.rid),'RESOURCE INFO ID')
            require((directory/f'resource-{serial}-info-type.bin').read_bytes()==resource.kind,'RESOURCE INFO TYPE')
            require((directory/f'resource-{serial}-info-name.bin').read_bytes()==(resource.name or '').encode('mac_roman'),'RESOURCE INFO NAME')
    require({('Data/NIS.rsrc','SHAP',25002),('Data/Prince2.opt','DSLV',6400)}<=
            {(r['source'],r['type'],r['rid']) for r in rows},'RESOURCE PAYLOAD COVERAGE')
    print(f'RESOURCE SURVEY PASS: {len(calls)} paired calls; {len(rows)} verified payloads')
    return rows


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path)
    report(parser.parse_args().directory.resolve())
