"""Reapply item 19's unchanged budgets to corrected-clock opening captures."""
import argparse
import hashlib
import json
import struct

from aprof_report import ROOT, bucket, load, original_requests, require, scope
from check_accelerated_clock import check as check_clock
from extract_original_data import pins


def inclusive(phases, labels):
    calls, milliseconds = 0, 0.0
    for phase in phases:
        for node in phase.order:
            if phase.labels[node] not in labels:
                continue
            ancestor = phase.nodes[node][0]
            while ancestor and phase.labels[ancestor] not in labels:
                ancestor = phase.nodes[ancestor][0]
            if not ancestor:
                calls += phase.nodes[node][4]
                milliseconds += phase.inclusive[node]/phase.units_per_ms
    return calls, milliseconds


def check(name):
    check_clock(name)
    context, phases = load(name, None)
    frames, _ = scope(context, phases, None)
    require(frames == 81, 'BUDGET / COMPLETE OPENING WORKLOAD')
    reference = ROOT/'ref/mame/runs/opening-noright-1'
    expected = pins(ROOT/'manifest/opening-profile.sha256')
    key = str((reference/'session.log').relative_to(ROOT))
    require(hashlib.sha256((reference/'session.log').read_bytes()).hexdigest() == expected[key],
            'BUDGET / ORIGINAL PIN')
    timers = list(struct.iter_unpack('>5I',(context.dir/'timing-set.bin').read_bytes()))
    requests = original_requests(reference)
    require(len(timers) == 82 and [r[3] for r in timers[1:]] == requests,
            'BUDGET / ORIGINAL TIMER WORKLOAD')
    require(all(b[1]-a[1] == a[3] for a,b in zip(timers,timers[1:])),
            'BUDGET / ORIGINAL CADENCE')
    metrics = {}
    def record(label, value, limit, unit, calls=None):
        metrics[label] = dict(value=value, limit=limit, unit=unit,
                              passed=0 < value <= limit, calls=calls)
    if context.config.get('active') == '1':
        require('ACTIVE SCOPE COMPLETE' in context.log, 'BUDGET / ACTIVE COMPLETION')
        dispatch = sum(row[0]/phase.units_per_ms for phase in phases
                       for pc, row in phase.pcs.items()
                       if bucket(*context.classify(pc)) == 'Trap entry and dispatch')
        # Frozen PCs place the original Line-A opcode at its actual CODE site.
        # Keep CPU exception entry charged to dispatch, without counting native
        # cached veneers twice when their PC already belongs to that bucket.
        entry_total=entry_extra=0.0
        from native_service_scope import linea
        for phase in phases:
            if phase.header.get('executed_pc_frozen') == 1:
                for pc,units in linea(phase.header,phase.pcs,phase.nodes).items():
                    cost=units/phase.units_per_ms
                    entry_total+=cost
                    if bucket(*context.classify(pc)) != 'Trap entry and dispatch':entry_extra+=cost
        dispatch+=entry_extra
        record('active dispatch', dispatch/frames, 12, 'ms/frame')
        metrics['active dispatch'].update(cpu_linea_entry_ms_per_frame=entry_total/frames,
                                         added_entry_ms_per_frame=entry_extra/frames)
        calls, cost = inclusive(phases, {'pop2WallTicks', 'pop2ServiceTicks',
                                         'anon::timerNow', 'anon::timerDueNow'})
        record('active clock sampling', cost/frames, 2, 'ms/frame', calls)
    else:
        native = None
        if context.config.get('native_scope') == '1':
            from native_service_scope import check as check_native
            native = check_native(name, controls=True)
        full_trap_costs = {}
        for label in ('pop2SectRect','pop2UnionRect','pop2EmptyRect',
                      'TRAP SystemTask','TRAP HGetState','TRAP HLock',
                      'TRAP StripAddress','TRAP SwapMMUMode'):
            calls, cost = inclusive(phases, {label})
            require(calls > 0, 'BUDGET / MISSING '+label)
            measured = cost*1000/calls
            if native is not None and label.startswith('TRAP '):
                full_trap_costs[label] = dict(value=measured, unit='us/call', calls=calls,
                                             scope='legacy full-trap inclusive estimate')
                row=native[label[5:]]
                record(label+' native service',row['microseconds'],25,'us/call',row['calls'])
            else:
                record(label, measured, 15 if label.startswith('pop2') else 25,
                       'us/call', calls)
        if native is not None:
            (context.dir/'full-trap-costs.json').write_text(json.dumps(full_trap_costs,indent=2)+'\n')
        _, landing = load(name, 0)
        for label, denominator, limit, unit in (
                ('TRAP NewHandle',7,0.5,'ms/call'),
                ('NativeAudio::dispatch',6,2,'ms/frame'),
                ('AmigaHardware::blitterDrain',6,2,'ms/frame')):
            calls, cost = inclusive(landing, {label})
            require(calls > 0 and (label != 'TRAP NewHandle' or calls == 7),
                    'BUDGET / LANDING WORKLOAD '+label)
            record(label,cost/denominator,limit,unit,calls)
    (context.dir/'budget-check.json').write_text(json.dumps(metrics,indent=2)+'\n')
    for label, row in metrics.items():
        print(f'{"PASS" if row["passed"] else "FAIL"} {label}: '
              f'{row["value"]:.6f} {row["unit"]}, limit {row["limit"]}')
    require(all(row['passed'] for row in metrics.values()), 'ACCELERATED COST BUDGETS')


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('name')
    check(parser.parse_args().name)
