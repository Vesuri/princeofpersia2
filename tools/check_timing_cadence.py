"""Require the original five-tick game-step cadence, including the first step."""
import argparse
import hashlib
import re
import struct
from collections import Counter
from pathlib import Path
from check_performance import check as performance
from extract_original_data import ROOT, pins


def require(ok, reason):
    if not ok:
        raise RuntimeError('TIMING CADENCE / '+reason)


def original(reference):
    log=(reference/'session.log').read_text()
    require(log.count('AUDIO_ROUTE_COMPLETE\n')==1 and 'FAIL REFERENCE' not in log
            and '[LUA ERROR]' not in log, 'ORIGINAL COMPLETION')
    parts=log.split('DISPLAY_ROUTE_CAPTURED first-room\n')
    require(len(parts)==2, 'ORIGINAL FIRST ROOM')
    rows=[tuple(int(v,16) for v in row) for row in re.findall(
        r'^TIMING_SET ticks=([0-9A-F]+) timer=([0-9A-F]+) delay=([0-9A-F]+)$',parts[1],re.M)]
    require(len(rows)==100 and all(r[1:]==(1,5) for r in rows), 'ORIGINAL TIMER REQUESTS')
    intervals=[(b[0]-a[0])&0xffffffff for a,b in zip(rows,rows[1:])]
    require(intervals==[5]*99, 'ORIGINAL CADENCE')
    # Exclude absolute epochs and emulator addresses, retain every measured gap.
    contract='timer=1 delay=5\n'+''.join(f'{gap}\n' for gap in intervals)
    require(pins(ROOT/'manifest/timing-cadence-reference.sha256')==
            {'game-step-cadence.txt':hashlib.sha256(contract.encode()).hexdigest()}, 'ORIGINAL PIN')
    return intervals


def cadence(rows):
    require(len(rows)>=32, 'ENOUGH COMPLETED STEPS')
    require(all(r[2:4]==(1,5) for r in rows), 'FIVE-TICK TIMER REQUESTS')
    require(all(((b[0]-a[0])&0xffff)==1 for a,b in zip(rows,rows[1:])), 'CONSECUTIVE GAME STEPS')
    gaps=[(b[1]-a[1])&0xffffffff for a,b in zip(rows,rows[1:])]
    require(all(gap==5 for gap in gaps), 'REFERENCE CADENCE INCLUDING FIRST STEP')
    return gaps


def controls():
    # These are checker controls, not substituted native or original evidence.
    good=[(i,100+5*i,1,5,200+4*i) for i in range(40)]
    cadence(good)
    changed=[good[:31]]
    for index,column,value in [(0,1,99),(20,1,201),(20,0,21),(20,2,2),(20,3,6)]:
        bad=list(good);row=list(bad[index]);row[column]=value;bad[index]=tuple(row);changed.append(bad)
    for bad in changed:
        try:
            cadence(bad)
        except RuntimeError:
            continue
        raise RuntimeError('TIMING CADENCE / ACCEPTED ALTERED RECORD')
    print(f'TIMING CADENCE CONTROLS PASS: {len(changed)} alterations rejected')


def check(run, reference, setup, run_controls=False):
    original(reference)
    performance(run,reference,setup)
    data=(run/'timing-set.bin').read_bytes()
    require(len(data)%20==0, 'TIMER RECORD EXTENT')
    rows=list(struct.iter_unpack('>5I',data))
    log=(run/'gdb-out.log').read_text()
    require(re.findall(r'^TIMING SETTERS rows=(\d+)$',log,re.M)==[str(len(rows))], 'TIMER RECORD COUNT')
    gaps=[(b[1]-a[1])&0xffffffff for a,b in zip(rows,rows[1:])]
    print('TIMING CADENCE OBSERVED: '+str(dict(sorted(Counter(gaps).items())))+' ticks:intervals')
    if run_controls:
        controls()
    cadence(rows)
    print(f'TIMING CADENCE PASS: {len(gaps)} consecutive original five-tick intervals')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('run',type=Path)
    p.add_argument('reference',type=Path)
    p.add_argument('--setup',required=True,choices=['a1200-fast','a1200-030','a1200-040'])
    p.add_argument('--controls',action='store_true')
    a=p.parse_args()
    check(a.run,a.reference,a.setup,a.controls)
