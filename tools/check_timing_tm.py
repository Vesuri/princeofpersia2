"""Check the paired Time Manager fixture, separately from gameplay cadence."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path
from check_core_result import restored
from extract_original_data import ROOT, pins


def require(ok, reason):
    if not ok:
        raise RuntimeError('TIMING TM / ' + reason)


def records(data):
    require(len(data) == 7*64, 'RECORD EXTENT')
    result = []
    callbacks = set()
    # DERIVED: TickCount quantizes the requested durations to 60 Hz. Allow
    # either boundary phase, not a rescaled delay or unbounded late dispatch.
    bounds = [(2,3),(2,3),(4,5),(1,2),None,(0,1),(1,2)]
    for i in range(7):
        row = data[64*i:64*(i+1)]
        u16 = lambda at: struct.unpack_from('>H',row,at)[0]
        u32 = lambda at: struct.unpack_from('>I',row,at)[0]
        i32 = lambda at: struct.unpack_from('>i',row,at)[0]
        calls = u16(24)
        require(calls == (0 if i==4 else 3 if i==6 else 1), 'CALLBACK COUNT '+str(i+1))
        require(all(u16(at)==0 for at in (26,28,30,56,58)), 'SERVICE RESULTS '+str(i+1))
        require(all(u16(at)==0 for at in (4,44,52)), 'INACTIVE FLAGS '+str(i+1))
        require(i32(10)==0, 'FINAL REMOVAL COUNT '+str(i+1))
        callbacks.add(u32(6))
        first = (u32(48)-u32(32))&0xffffffff
        last = (u32(36)-u32(32))&0xffffffff
        if i==4:
            require(u32(48)==u32(36)==0, 'CANCELLED CALLBACK CLOCK')
            require(-200000 <= i32(40) < 0, 'CANCELLED MICROSECOND REMAINDER')
            first = last = None
        else:
            require(i32(40)==0, 'EXPIRED COUNT '+str(i+1))
            lo,hi = bounds[i]
            require(lo <= first <= hi, 'FIRST DEADLINE '+str(i+1))
            if i==6:
                require(3 <= last <= 4 and 2 <= last-first <= 3, 'REARMED DEADLINES')
            else:
                require(first==last, 'SINGLE CALLBACK CLOCK '+str(i+1))
        result.append(dict(calls=calls,first=first,last=last,remaining=i32(40)))
    require(len(callbacks)==1 and 0 not in callbacks, 'CALLBACK IDENTITY')
    return result


def original(reference):
    log = (reference/'session.log').read_text()
    require(log.count('TIMING_TM_COMPLETE cases=7\n')==1, 'ORIGINAL COMPLETION')
    require('FAIL REFERENCE' not in log and '[LUA ERROR]' not in log, 'ORIGINAL FAILURE')
    require(log.count('REFERENCE_CAPTURED opening-text-1\n')==1, 'ORIGINAL OPENING')
    rows = records((reference/'timing-tm.bin').read_bytes())
    contract = json.dumps(rows,sort_keys=True,separators=(',',':')).encode()+b'\n'
    expected = pins(ROOT/'manifest/timing-tm-reference.sha256')
    require(set(expected)=={'tm-contract.json'}, 'REFERENCE PIN COVERAGE')
    require(hashlib.sha256(contract).hexdigest()==expected['tm-contract.json'], 'REFERENCE CONTRACT PIN')
    return rows


def evidence(log,data):
    require(log.count("CLOCK FAULT 0\n")==1, "MONOTONIC CLOCK")
    require(log.count('CORE REASON TIMING / TIME MANAGER FIXTURE COMPLETE\n')==1, 'POSITIVE COMPLETION')
    require('CORE EXCEPTION vector=0 pc=00000000\n' in log, 'EXCEPTION')
    require('INTEGRATION AUDIT code=1 input=1\n' in log, 'CODE AND INPUT AUDITS')
    require(log.count('TIMING TM RESTORED\n')==1, 'RESTORED MILESTONE')
    require(re.findall(r'^TIMING TM result=(\d+) user_calls=(\d+)$',log,re.M)==[('1','10')], 'USER MODE AND ABI')
    return records(data)


def check(run,reference,setup,controls=False):
    original(reference)
    log = (run/'gdb-out.log').read_text()
    data = (run/'timing-tm.bin').read_bytes()
    restored(run,log,setup)
    rows = evidence(log,data)
    if controls:
        changed=[]
        for old,new in [('TIME MANAGER FIXTURE COMPLETE','TIMEOUT'),('TIMING TM RESTORED','NOT RESTORED'),
                        ('result=1','result=0'),('user_calls=10','user_calls=9'),('CLOCK FAULT 0','CLOCK FAULT 1')]:
            require(old in log,'INACTIVE LOG CONTROL')
            changed.append((log.replace(old,new),data))
        for at,size,value in [(24,2,0),(4*64+40,4,1000),(2*64+48,4,struct.unpack_from('>I',data,2*64+32)[0]),
                              (6*64+24,2,1),(52,2,0x8000),(58,2,0xffff)]:
            bad=bytearray(data);struct.pack_into('>H' if size==2 else '>I',bad,at,value)
            require(bad!=data,'INACTIVE RECORD CONTROL');changed.append((log,bytes(bad)))
        changed.append((log,data[:-1]))
        for bad_log,bad_data in changed:
            try:evidence(bad_log,bad_data)
            except RuntimeError:continue
            raise RuntimeError('TIMING TM / ACCEPTED ALTERED EVIDENCE')
        evidence(log,data)
        print(f'TIMING TM CONTROLS PASS: {len(changed)} alterations rejected; restored positive passed')
    print('TIMING TM PASS: delay units, active prime, cancellation, zero delay, rearm, user-mode ABI and restoration; not gameplay cadence')
    return rows


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=Path)
    parser.add_argument('reference',type=Path)
    parser.add_argument('--setup',choices=['a1200-fast','a1200-030','a1200-040'],required=True)
    parser.add_argument('--controls',action='store_true')
    args=parser.parse_args()
    check(args.run,args.reference,args.setup,args.controls)
