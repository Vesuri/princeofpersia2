"""Check Mohawk eligible-update deadlines without calling them display frames."""
import argparse
import json
from pathlib import Path
import re
import struct

from aprof_report import require


def validate(rows):
    require(bool(rows), 'CARPET NO ELIGIBLE UPDATES')
    animations=[];current=None
    for row in rows:
        tick,target,before_delay,before_frame,initial,next_target,delay,frame,result,field=row
        signed=lambda v:v if v<0x80000000 else v-0x100000000
        delay=signed(delay);before_delay=signed(before_delay)
        require(tick>=target, 'CARPET EARLY UPDATE')
        require(next_target==(tick+delay)&0xffffffff, 'CARPET REARM FROM CALLER TICK')
        require(frame>=before_frame, 'CARPET FRAME REVERSED')
        if initial:
            require(target==tick, 'CARPET INITIAL DEADLINE')
            current=[];animations.append(current)
        require(current is not None, 'CARPET MISSING INITIAL UPDATE')
        if current:
            previous=current[-1]
            require(target==previous['next_target'], 'CARPET DEADLINE CONTINUITY')
            require(before_frame==previous['frame'] and before_delay==previous['delay'], 'CARPET STATE CONTINUITY')
        current.append(dict(tick=tick,target=target,next_target=next_target,delay=delay,
                            before_frame=before_frame,frame=frame,late_ticks=tick-target))
    return animations


def native(path):
    log=(path/'gdb-out.log').read_text()
    require(log.count('SCENE RESTORED\n')==1 and log.count('CORE REASON SCENE PROFILE / COMPLETE\n')==1,
            'CARPET NATIVE COMPLETION')
    data=(path/'scene-animation.bin').read_bytes()
    count=re.findall(r'^SCENE ANIMATION rows=(\d+)$',log,re.M)
    require(len(count)==1 and 0<int(count[0])<512 and len(data)==int(count[0])*40, 'CARPET RECORD EXTENT')
    rows=list(struct.iter_unpack('>10I',data))
    require(all(r[8] in (0,1) for r in rows) and sum(r[8] for r in rows)==282, 'CARPET DRAW RESULTS')
    groups=validate(rows)
    require(len(groups)==3, 'CARPET ANIMATION COVERAGE')
    return groups


def original(path):
    log=(path/'session.log').read_text()
    require(log.count('GAMEPLAY_DISCOVERY_COMPLETE\n')==1 and 'FAIL REFERENCE' not in log,
            'CARPET ORIGINAL COMPLETION')
    returns=re.findall(r'^CUTSCENE_RETURN ordinal=(\d+) selector=1 result=0 copy=\d+ text=\d+$',log,re.M)
    require(len(returns)==1, 'CARPET ORIGINAL RETURN')
    events=re.findall(r'^SCENE_DEADLINE ordinal='+returns[0]+r' selector=1 site=(0392|0454) tick=(\d+) target=(\d+) delay=(-?\d+) frame=(\d+) active=(\d+)$',log,re.M)
    rows=[];pending=None;last_frame=None
    for site,*values in events:
        tick,target,delay,frame,active=map(int,values)
        if site=='0392':
            require(pending is None and active!=0, 'CARPET ORIGINAL ELIGIBLE ENTRY')
            pending=(tick,target,delay,frame)
        else:
            require(pending is not None and tick==pending[0] and target==pending[1], 'CARPET ORIGINAL MATCHED REARM')
            t,d,b,f=pending
            # The object frame restarts at zero for each animation. Confirm
            # the first deadline is immediate rather than guessing its epoch.
            initial=1 if last_frame is None or f==0 else 0
            rows.append((t,d,b&0xffffffff,f,initial,(tick+delay)&0xffffffff,delay&0xffffffff,frame,0,0))
            last_frame=frame;pending=None
    require(pending is None,'CARPET ORIGINAL PENDING UPDATE')
    groups=validate(rows)
    frames=re.findall(r'^SCENE_ANIMATION ordinal='+returns[0]+r' selector=1 frames=(\d+) result=0$',log,re.M)
    require(list(map(int,frames))==[118,80,84] and len(groups)==3, 'CARPET ORIGINAL ANIMATION COVERAGE')
    return groups


def summary(animations):
    result=[]
    for a in animations:
        lags=sorted(r['late_ticks'] for r in a)
        quantile=lambda fraction:lags[int((len(lags)-1)*fraction)]
        result.append(dict(updates=len(a),delays=sorted(set(r['delay'] for r in a)),
            late_updates=sum(v>0 for v in lags),p50_late_ticks=quantile(.5),
            p95_late_ticks=quantile(.95),p99_late_ticks=quantile(.99),max_late_ticks=max(lags)))
    return result


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('reference',type=Path);p.add_argument('--native',type=Path);a=p.parse_args()
    from cinematic_deadlines import verify_inputs
    verify_inputs([a.reference/'session.log'],'cinematic-deadlines-reference.sha256')
    ref=original(a.reference);out=dict(original=summary(ref))
    if a.native:
        port=native(a.native)
        signature=lambda groups:[[(r['before_frame'],r['frame'],r['delay']) for r in g] for g in groups]
        require(signature(ref)==signature(port),'CARPET ORIGINAL WORKLOAD')
        out['native']=summary(port)
    print(json.dumps(out,indent=2))
