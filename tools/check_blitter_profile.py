"""19.4's CPU waiting budget, using the same aprof phase/frame denominator.

Conservatively charge the entire drain subtree, including queue advancement and
call overhead, as waiting. Deferred work is still included in report/work totals.
The opening checker separately establishes matched original requests and state.
"""
import argparse
import copy
import struct
from aprof_report import load, scope, require, summarize, OUT


def workload(base, out, old, new, timers):
    require(out['config']==base['config'],'different measurement configuration')
    require(len(old)==len(new)==out['frames']==base['frames']==81,'workload frame count')
    keys=('counter','phase','kid')
    require([tuple(r[k] for k in keys) for r in old]==
            [tuple(r[k] for k in keys) for r in new],'changed frame state')
    require(out['requests']==base['requests'],'changed timer requests')
    require(len(timers)==82 and all(b[1]-a[1]==a[3] for a,b in zip(timers,timers[1:])),
            'early or late original timer interval')
    require(len(base['phases'])==len(out['phases'])==5,'phase count')
    for a,b in zip(base['phases'],out['phases']):
        require((a['phase'],a['health'],a['frames'])==(b['phase'],b['health'],b['frames']),
                'changed phase state')
        # This is a before/after improvement check, not a repeatability check.
        # Retain the repeat check's quantization margin for increases only.
        require(b['fields']-a['fields']<=max(1,0.02*a['fields']),'phase became slower')
        require([c for c,_ in a['stalls']]==[c for c,_ in b['stalls']],'changed stalls')


def compare(name, baseline, controls=False):
    base,old=summarize(baseline)
    out,new=summarize(name)
    timers=list(struct.iter_unpack('>5I',(OUT/name/'timing-set.bin').read_bytes()))
    data=(base,out,old,new,timers)
    workload(*data)
    if controls:
        bads=[]
        bad=copy.deepcopy(data);bad[3].pop();bads.append(bad)
        bad=copy.deepcopy(data);bad[3][0]['kid']='00';bads.append(bad)
        bad=copy.deepcopy(data);bad[1]['requests'][0]+=1;bads.append(bad)
        for delta in (-1,1):
            bad=copy.deepcopy(data);row=list(bad[4][1]);row[1]+=delta
            bad[4][1]=tuple(row);bads.append(bad)
        bad=copy.deepcopy(data);bad[1]['phases'][0]['fields']=base['phases'][0]['fields']+2;bads.append(bad)
        for bad in bads:
            try:workload(*bad)
            except SystemExit:pass
            else:raise AssertionError('accepted changed workload')
        print('BLITTER WORKLOAD CONTROLS PASS: six changes rejected')
    print(f'BLITTER WORKLOAD PASS {name} vs {baseline}: 81 identical frame states, '
          f'81 exact original timer intervals; phase fields '
          f'{[p["fields"] for p in base["phases"]]} -> {[p["fields"] for p in out["phases"]]}')


def check(name, budget=2.0):
    ctx, phases=load(name,0)
    require(ctx.config==dict(setup='a1200-030',cycles='1',no_mmu='1',active='0'),
            'requires the full exact-cycle 68030 opening profile')
    frames,_=scope(ctx,phases,0)
    calls=0
    ms=0.0
    for phase in phases:
        for node in phase.order:
            if phase.labels[node]=='AmigaHardware::blitterDrain':
                calls+=phase.nodes[node][4]
                ms+=phase.inclusive[node]/phase.units_per_ms
    require(frames==6 and calls>=7 and ms>0,'missing landing publications/drains')
    mean=ms/frames
    require(mean<=budget,f'blitter drain {mean:.6f} ms/frame exceeds {budget:.6f}')
    print(f'BLITTER WAIT PASS {name}: {calls} drains, {mean:.6f} ms/reported landing frame <= {budget:g} ms')
    return mean


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('name')
    p.add_argument('--compare')
    p.add_argument('--controls',action='store_true')
    a=p.parse_args()
    check(a.name)
    if a.compare:compare(a.name,a.compare,a.controls)
