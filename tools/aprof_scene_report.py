"""Report cinematic cycles and timer intervals without conflating transitions.

Uses the existing shared-derived exact-cycle parser and call tree. Timer-to-timer
intervals are comparable only within a consecutive run of the same original
caller and selector. A boundary/one-off is reported separately, never as a frame.
"""
import argparse
import collections
import json
import re
import struct

from aprof_report import OUT, Context, Phase, bucket, require


def read(name):
    d=OUT/name
    config=dict(x.split('=',1) for x in (d/'config.txt').read_text().split())
    log=(d/'gdb-out.log').read_text()
    start=re.findall(r'^SCENE START selector=(\d+) fields=(\d+) ticks=(\d+) frames=(\d+) keys=(\d+)$',log,re.M)
    stop=re.findall(r'^SCENE STOP selector=(\d+) result=0 fields=(\d+) ticks=(\d+) frames=(\d+) keys=(\d+)$',log,re.M)
    require(len(start)==len(stop)==1 and start[0][0]==stop[0][0] and start[0][3:]==stop[0][3:], 'SCENE ENDPOINTS')
    require(log.count('SCENE RESTORED\n')==1 and log.count('CORE REASON SCENE PROFILE / COMPLETE\n')==1, 'SCENE COMPLETION')
    data=(d/'scene-timers.bin').read_bytes() if (d/'scene-timers.bin').exists() else b''
    require(len(data)%32==0 and (data or config['scene']=='carpet'),'SCENE TIMER ROWS')
    rows=list(struct.iter_unpack('>8I',data))
    require(re.findall(r'^SCENE RECORDED rows=(\d+)$',log,re.M)==[str(len(rows))], 'SCENE ROW COUNT')
    require(all(r[7]==1 for r in rows), 'SCENE ACTIVE SCOPE')
    groups=collections.defaultdict(list);boundaries=[]
    for a,b in zip(rows,rows[1:]):
        elapsed=(b[1]-a[1])&0xffffffff
        require(elapsed<0x100000 and b[4]>=a[4], 'SCENE MONOTONIC CLOCK')
        key=(a[5],a[6],a[2])
        if key==(b[5],b[6],b[2]):groups[key].append((elapsed,a[3],b[4]-a[4]))
        else:boundaries.append((key,elapsed))
    loops=[]
    for (seg,off,timer),values in sorted(groups.items()):
        gaps=sorted(v[0] for v in values)
        quant=lambda p:gaps[min(len(gaps)-1,int((len(gaps)-1)*p))]
        loops.append(dict(caller=f'CODE{seg}+${off:04X}',timer=timer,intervals=len(values),
            requested_ticks=sorted(set(v[1] for v in values)),mean_ticks=sum(gaps)/len(gaps),
            p50_ticks=quant(.5),p95_ticks=quant(.95),p99_ticks=quant(.99),max_ticks=max(gaps),
            over_request=sum(a>b for a,b,_ in values),total_fields=sum(v[2] for v in values)))
    # Curated original state, excluding platform addresses and clocks. Retain
    # full capture hashes separately; no claim of equivalence from a hash alone.
    states=[];method_slots=[]
    for label in ('start','end'):
        a5=(d/f'scene-{label}-a5.bin').read_bytes()
        require(len(a5)==22266,'SCENE STATE EXTENT')
        method_slots.append(a5[-0x226c:-0x2268])
        states.append(b''.join(a5[-off:-off+n] for off,n in ((0x18fc,4),(0x4fd2,2),(0x4fc2,2),(0x349a,2),(0x5208,372))).hex())
    clocks=[];draws=[];calibrations=[];pending=None
    clockfile=d/'scene-clocks.bin'
    if clockfile.exists():
        raw=clockfile.read_bytes();require(len(raw)%32==0,'SCENE CLOCK EXTENT')
        clocks=list(struct.iter_unpack('>8I',raw));current=None
        require(re.findall(r'^SCENE CLOCKS rows=(\d+)$',log,re.M)==[str(len(clocks))], 'SCENE CLOCK COUNT')
        for site,tick,field,duration,progress,target,delay,counter in clocks:
            if site==0x3984:
                require(pending is None,'SCENE CALIBRATION NESTING');pending=(tick,field,duration)
            elif site==0x3b72:
                require(pending is not None,'SCENE CALIBRATION OWNER')
                calibrations.append(dict(ticks=tick-pending[0],fields=field-pending[1],delay=duration));pending=None
            elif site==0x3152:
                current=dict(duration=duration,start_tick=tick,start_field=field,quanta=[]);draws.append(current)
            elif site==0x321a:
                require(current is not None and duration==current['duration'],'SCENE DISSOLVE OWNER')
                require(progress==len(current['quanta'])+1 and target==current['start_tick']+progress,'SCENE DISSOLVE DEADLINE')
                current['quanta'].append(dict(progress=progress,late_ticks=tick-target,field=field,delay=delay))
            else:require(False,'SCENE CLOCK SITE')
        require(pending is None,'SCENE CALIBRATION INCOMPLETE')
        for draw in draws:
            require(draw['quanta'],'SCENE DISSOLVE EMPTY')
            draw['late_quanta']=sum(q['late_ticks']>0 for q in draw['quanta'])
            draw['max_late_ticks']=max(q['late_ticks'] for q in draw['quanta'])
            draw['through_last_quantum_fields']=draw['quanta'][-1]['field']-draw['start_field']
    if config['scene']=='throne':
        require([d['duration'] for d in draws]==[240,180,20,20,20,180] and len(calibrations)==6,'SCENE DISSOLVE COVERAGE')
    receipt=json.loads((d/'build-receipt.json').read_text())
    animation_signature=[]
    if '-DPOP2_SCENE_DEADLINES' not in receipt['flags'].split():
        require(not (d/'scene-animation.bin').exists() and 'SCENE ANIMATION rows=' not in log, 'SCENE UNDECLARED DEADLINE OBSERVER')
    if '-DPOP2_SCENE_DEADLINES' in receipt['flags'].split():
        from carpet_deadlines import native
        require(method_slots[0]==method_slots[1] and any(method_slots[0]), 'SCENE METHOD TABLE RESTORATION')
        animation_signature=[[(r['before_frame'],r['frame'],r['delay']) for r in group] for group in native(d)]
    return dict(name=name,config=config,build_identity=receipt,animation_signature=animation_signature,clock_signature=[[r[0],r[3],r[4]] for r in clocks],calibrations=calibrations,draws=draws,fields=int(stop[0][1])-int(start[0][1]),
        ticks=int(stop[0][2])-int(start[0][2]),selector=int(start[0][0]),route_start=list(map(int,start[0][3:])),
        timer_requests=len(rows),loops=loops,boundary_intervals=len(boundaries),states=states,
        request_signature=[[r[0],r[2],r[3],r[5],r[6]] for r in rows])


def compare(a,b):
    for key in ('config','selector','route_start','timer_requests','states','request_signature','build_identity','clock_signature','animation_signature'):
        require(a[key]==b[key], 'SCENE MATCHED WORKLOAD '+key)
    require(abs(a['fields']-b['fields'])<=max(1,a['fields']*.02),'SCENE FIELD REPEATABILITY')


def report(name, other=None):
    out=read(name)
    d=OUT/name
    if other:compare(read(other),out)
    if out['config']['cycles']=='1':
        ctx=Context(name);p=Phase(ctx,d/'scene.prof');ctx.add_targets(p.nodes);ctx.finish()
        p.labels={n:p.label(n) for n in p.nodes}
        upm=p.units_per_ms
        buckets=collections.Counter()
        for pc,row in p.pcs.items():
            region,fn=ctx.classify(pc);buckets[bucket(region,fn)]+=row[0]
        out['profile_ms']=p.units/upm;out['gap_ms']=p.header['gap']/upm
        out['capture_tail_ms']=p.header['tail']/upm
        out['unmatched_returns']={'rts':p.header['unmatched_rts'],'rte':p.header['unmatched_rte']}
        require(p.header.get('high',0)==0,'SCENE UNMAPPED PROFILE PCS')
        buckets['Exception entry and STOP gaps']+=p.header['gap']
        buckets['Unassigned capture boundary']+=p.header['tail']
        out['buckets_ms']={k:v/upm for k,v in buckets.most_common()}
        # Long cinematic scopes contain non-local/Pascal returns that this
        # profiler cannot pair reliably. Flat PC counters remain usable; do
        # not publish an invented active-work denominator from its call tree.
        out['call_tree_usable']=False
        if out['animation_signature']:
            observer=collections.Counter()
            for pc,row in p.pcs.items():
                region,fn=ctx.classify(pc)
                if 'SceneAnimation' in fn or 'scene_animation' in fn:observer[fn]+=row[0]/upm
            out['deadline_observer_self_ms']=dict(observer)
            # Flat self costs exclude called helpers; they are not an inclusive
            # overhead estimate and are not subtracted from measured cadence.
        base15=next(b for b,e,seg in ctx.segs if seg==15)
        out['nis_copies']=p.pcs.get(base15+0x32f4,(0,0))[1]
        out['nis_texts']=p.pcs.get(base15+0x170,(0,0))[1]
        out['dissolve_entries']={label:p.pcs.get(base15+off,(0,0))[1] for label,off in
            (('init',0x2c40),('generate',0x2d7a),('draw',0x303c),('calibrate',0x396c))}
        base16=next(b for b,e,seg in ctx.segs if seg==16)
        out['animation_frames']=sum(p.pcs.get(base16+off,(0,0))[1] for off in (0x12de,0x1464))
        out['animation_returns']=p.pcs.get(base16+0x159a,(0,0))[1]

    (d/'scene-report.json').write_text(json.dumps(out,indent=2)+'\n')
    print(f"{name}: {out['fields']} PAL fields ({out['fields']*20} ms), {out['timer_requests']} timer requests")
    if 'profile_ms' in out:
        print(f"Profile total {out['profile_ms']:.3f} ms; exception/STOP gaps {out['gap_ms']:.3f} ms")
        for key,value in out['buckets_ms'].items():print(f'  {key}: {value:.3f} ms')
    for loop in out['loops']:print(json.dumps(loop))
    for draw in out['draws']:print(f"Dissolve requested {draw['duration']} ticks: {len(draw['quanta'])} quanta; {draw['late_quanta']} late, maximum {draw['max_late_ticks']} ticks behind")
    print('Boundary intervals, excluded from steady-loop cadence:',out['boundary_intervals'])
    if other:print('SCENE REPEAT PASS:',other,name)
    return out


if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('name');ap.add_argument('--compare');a=ap.parse_args();report(a.name,a.compare)
