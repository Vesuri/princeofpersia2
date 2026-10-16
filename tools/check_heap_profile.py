"""19.7 NewHandle budget, including its dispatch and nested callback costs."""
import argparse
from aprof_report import load, scope, require


def budget(calls, ms, limit=0.5):
    require(calls==7 and ms>0,'missing landing NewHandle workload')
    require(ms/calls<=limit,f'NewHandle {ms/calls:.6f} ms/call exceeds {limit:.6f}')


def check(name,controls=False):
    ctx,phases=load(name,0)
    require(ctx.config==dict(setup='a1200-030',cycles='1',no_mmu='1',active='0'),
            'requires full exact-cycle 68030 profile')
    frames,_=scope(ctx,phases,0);require(frames==6,'landing scope')
    calls=0;ms=0.0
    for phase in phases:
        for node in phase.order:
            if phase.labels[node]=='TRAP NewHandle':
                calls+=phase.nodes[node][4];ms+=phase.inclusive[node]/phase.units_per_ms
    budget(calls,ms)
    if controls:
        for bad in ((0,0),(6,ms),(7,0),(7,3.501)):
            try:budget(*bad)
            except SystemExit:pass
            else:raise AssertionError('accepted missing work or over-budget timing')
        print('HEAP BUDGET CONTROLS PASS: four missing/over-budget measurements rejected')
    print(f'HEAP PROFILE PASS {name}: {calls} NewHandle calls, {ms/calls:.6f} ms/call <= 0.5 ms')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('name');p.add_argument('--controls',action='store_true')
    a=p.parse_args();check(a.name,a.controls)
