"""Verify live corrected-clock metadata and accounting in a 68030 profile."""
import argparse
import copy
import hashlib
import json
import re
from pathlib import Path

from aprof_report import OUT, ROOT, check_run, parse_profile, require


def clock(header, config):
    require(config.get('cpu_model') == '68030', 'CLOCK / CPU CONFIGURATION')
    require(config.get('cpu_frequency') == '50000000', 'CLOCK / REQUESTED FREQUENCY')
    require(config.get('ntsc') == 'false', 'CLOCK / PAL CONFIGURATION')
    # UAE omits the default no-MMU field; aprof itself refuses an active MMU.
    require(config.get('mmu_model') in (None, '0'), 'CLOCK / PROFILE MMU')
    require(config.get('cpu_cycle_exact') == 'true', 'CLOCK / EXACT CPU')
    require(header['cpu_model'] == 68030, 'CLOCK / LIVE CPU')
    require(header['CYCLE_UNIT'] == 512 and header['colorclock_hz'] == 3546895,
            'CLOCK / LIVE COLOUR CLOCK')
    unit = 512 * 3546895 // 50000000
    require(header['cycleunit'] == unit == 36, 'CLOCK / LIVE CPU CYCLE UNIT')
    require(header['units'] > 0 and header['vsyncs'] >= 0, 'CLOCK / EMPTY SCOPE')
    # Each boundary may fall anywhere in its field; interlace alternates 312/313 lines.
    fields = header['units'] / (512 * 227 * 312.5)
    require(abs(fields-header['vsyncs']) <= 1.01, 'CLOCK / FIELD ACCOUNTING')
    return fields


def frozen_returns(header, pcs, exits, entry):
    require(header.get('executed_pc_frozen') == 1, 'CLOCK / EXECUTED PC IDENTITY')
    require(header.get('native_scope_faults') == 0, 'CLOCK / SERVICE RETURN FAULT')
    require(tuple(header.get('native_scope_exit_'+str(i)) for i in range(3)) == tuple(exits),
            'CLOCK / FROZEN RETURN ADDRESSES')
    returns = sum(value for key,value in header.items()
                  if re.fullmatch(r'native_[0-9a-f]{3}_returns',key))
    prefix = header.get('native_scope_prefix_returns')
    require(isinstance(prefix,int) and prefix >= 0, 'CLOCK / PREFIX RETURN ACCOUNTING')
    require(sum(pcs.get(pc,(0,0))[1] for pc in exits) == returns+prefix,
            'CLOCK / FLAT RETURN INSTRUCTION COUNTS')
    calls=sum(value for key,value in header.items() if re.fullmatch(r'linea_[0-9a-f]{8}_calls',key))
    require(pcs.get(entry,(0,0))[1]==calls,'CLOCK / FIRST HANDLER INSTRUCTION COUNT')


def check(name, controls=False):
    directory = OUT/name
    run = ROOT/'amiga'/(directory/'run-dir.txt').read_text().strip()
    check_run(run)
    capture = dict(row.split('=',1) for row in (directory/'config.txt').read_text().split())
    group = 'scoped' if capture.get('native_scope') == '1' or capture.get('frozen_pc') == '1' else 'calibrated'
    require(f'INPUT PASS {group}: 1 pinned files\n' in (directory/'run.log').read_text(),
            'CLOCK / CALIBRATED EMULATOR INPUT')
    receipt = json.loads((run/'build-receipt.json').read_text())
    require(receipt == json.loads((directory/'build-receipt.json').read_text()),
            'CLOCK / COPIED BUILD RECEIPT')
    for key, path in [('elf',run/'PrinceOfPersia2.elf'),
                      ('elf',directory/'PrinceOfPersia2.elf'),
                      ('exe',run/'dh1/PrinceOfPersia2')]:
        require(hashlib.sha256(path.read_bytes()).hexdigest() == receipt[key],
                'CLOCK / FROZEN BUILD IDENTITY')
    config = dict(line.split('=', 1) for line in (run/'logs/debug.uae').read_text().splitlines() if '=' in line)
    profiles = sorted(directory.glob('*.prof'))
    require(profiles, 'CLOCK / NO PROFILES')
    headers = []
    exits = None
    entry = None
    frozen_rows = []
    if capture.get('native_scope') == '1':
        from aprof_report import load
        from native_service_scope import EXITS
        context,_ = load(name,None)
        named=dict(zip(context.symbol_names,context.symbol_addrs))
        exits=[named[n] for n in EXITS]
        entry=named["pop2_line_a"]
    for profile in profiles:
        header, pcs, nodes = parse_profile(profile.read_text())
        clock(header, config)
        if capture.get('frozen_pc') == '1':
            require(header.get('executed_pc_frozen') == 1,'CLOCK / EXECUTED PC IDENTITY')
        headers.append(header)
        if exits is not None:
            frozen_returns(header,pcs,exits,entry)
            from native_service_scope import linea
            linea(header,pcs,nodes)
            frozen_rows.append((header,pcs))
    if exits is not None:
        require(all(sum(pcs.get(pc,(0,0))[1] for _,pcs in frozen_rows)>0 for pc in exits),
                'CLOCK / LIVE RETURN INSTRUCTION CONTROL')
    if controls:
        trials = []
        for field, value in [('cycleunit',32), ('cpu_model',68020), ('CYCLE_UNIT',256),
                             ('colorclock_hz',3579545), ('units',0), ('vsyncs',100000)]:
            changed = copy.deepcopy(headers[0]); changed[field] = value
            trials.append((changed, config))
        for field, value in [('cpu_frequency','25000000'), ('cpu_model','68040'),
                             ('ntsc','true'), ('mmu_model','68030'), ('cpu_cycle_exact','false')]:
            changed = dict(config); changed[field] = value
            trials.append((headers[0], changed))
        for header, settings in trials:
            try:
                clock(header, settings)
            except SystemExit:
                pass
            else:
                raise AssertionError('CLOCK / ALTERED EVIDENCE ACCEPTED')
        print(f'CLOCK CONTROLS PASS: {len(trials)} alterations rejected')
        if exits is not None:
            h,pcs=frozen_rows[0]
            mutations=[]
            for key,value in [('executed_pc_frozen',0),('native_scope_prefix_returns',h['native_scope_prefix_returns']+1)]:
                changed=dict(h);changed[key]=value;mutations.append((changed,pcs))
            changed=dict(pcs);row=list(changed[exits[2]]);row[1]+=1;changed[exits[2]]=tuple(row)
            mutations.append((h,changed))
            changed=dict(pcs);row=list(changed[entry]);row[1]+=1;changed[entry]=tuple(row)
            mutations.append((h,changed))
            for header,rows in mutations:
                try:frozen_returns(header,rows,exits,entry)
                except SystemExit:pass
                else:raise AssertionError('CLOCK / ALTERED RETURN EVIDENCE ACCEPTED')
            print('EXECUTED PC CONTROLS PASS: 4 altered identity/return/entry records rejected')
    report = dict(profiles=len(profiles), requested_hz=50000000, cycleunit=36,
                  effective_cycle_budget_hz=512*3546895/36,
                  note='Integer emulator cycle budget; not physical accelerator calibration')
    (directory/'clock-check.json').write_text(json.dumps(report,indent=2)+'\n')
    print('ACCELERATED CLOCK PASS: '+json.dumps(report,sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('name')
    parser.add_argument('--controls', action='store_true')
    args = parser.parse_args()
    check(args.name, args.controls)
