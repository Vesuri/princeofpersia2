"""Configure and validate explicit native Line-A service scopes (D19)."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path
from aprof_report import elf_section, load, require

EXITS = ('pop2QuickNativeReturn', 'pop2QuickCReturn', 'pop2UserServiceReturn')
TRAPS = {0x9b4:'SystemTask',0x069:'HGetState',0x029:'HLock',0x055:'StripAddress',0x05d:'SwapMMUMode'}


def observer(elf, template, output):
    table = subprocess.check_output(['m68k-amiga-elf-objdump','-t',str(elf)],text=True)
    names = {f[-1]:int(f[0],16) for line in table.splitlines()
             if len(f:=line.split())>=5 and re.fullmatch('[0-9a-f]+',f[0])}
    heads = subprocess.check_output(['m68k-amiga-elf-objdump','-h',str(elf)],text=True)
    sections = {f[1]:(int(f[3],16),int(f[2],16)) for line in heads.splitlines()
                if len(f:=line.split())>=7 and f[0].isdigit()}
    low,size=sections['.text']
    require(all(low<=names[n]<low+size for n in EXITS), 'NATIVE SCOPE / RETURN SYMBOLS')
    code=elf_section(elf,'.text')
    require([int.from_bytes(code[names[n]-low:names[n]-low+2],'big') for n in EXITS]
            ==[0x4e73,0x4e73,0x4e75], 'NATIVE SCOPE / RETURN OPCODES')
    commands = ''
    commands += f'set $nativeBase=(unsigned long)&pop2_line_a-0x{names["pop2_line_a"]:x}\n'
    commands += 'printf "NATIVE EXEC low=%lx high=%lx supervisor=%lx\\n", *(unsigned long*)4-*(unsigned short*)(*(unsigned long*)4+16), *(unsigned long*)4, *(unsigned long*)(*(unsigned long*)4-28)\n'
    commands += f'eval "monitor aprof native %x %x %x %x %x", $nativeBase+0x{low:x}, $nativeBase+0x{low+size:x}, '+', '.join('(unsigned long)&'+n for n in EXITS)+'\n'
    commands += 'echo NATIVE SERVICE SCOPE CONFIGURED\\n\n'
    source=template.read_text();needle='   monitor aprof start\n  end\n end\n if g_integrationStage==4'
    require(source.count(needle)==1,'NATIVE SCOPE / OBSERVER BOUNDARY')
    source=source.replace(needle,commands+needle)
    output.write_text(source)


def shared():
    from amigaxdev import native_service_scope as shared_scope
    pin=(Path(__file__).resolve().parents[1]/'manifest/native-scope-helper.sha256').read_text().split()[0]
    require(hashlib.sha256(Path(shared_scope.__file__).read_bytes()).hexdigest()==pin,'NATIVE SCOPE / SHARED HELPER PIN')
    return shared_scope


def linea(header, pcs, nodes):
    try:return shared().linea_entries(header,pcs,nodes)
    except ValueError as error:raise SystemExit(str(error)) from error


def headers(context, phases):
    shared_scope=shared()
    named=dict(zip(context.symbol_names,context.symbol_addrs))
    try:
        totals,diagnostics=shared_scope.validate([p.header for p in phases],
                                                [named[n] for n in EXITS],context.sections['.text'])
    except ValueError as error:
        raise SystemExit(str(error)) from error
    require(diagnostics['interrupts']>0 and diagnostics['irq_before']==diagnostics['irq_after']==0,
            'NATIVE SCOPE / LIVE INTERRUPT CONTROL')
    os_record=re.findall(r'^NATIVE EXEC low=([0-9a-f]+) high=([0-9a-f]+) supervisor=([0-9a-f]+)$',context.log,re.M)
    require(len(os_record)==1,'NATIVE SCOPE / EXEC IDENTITY')
    expected=tuple(int(v,16) for v in os_record[0])
    privilege=0
    for phase in phases:
        h=phase.header
        require(tuple(h.get('native_scope_'+key) for key in ('exec_low','exec_high','supervisor'))==expected,'NATIVE SCOPE / EXEC VECTORS')
        actual=sum(row[4] for row in phase.nodes.values() if row[2]==8)
        require(h.get('native_scope_privilege_calls')==actual,'NATIVE SCOPE / SUPERVISOR CALL COUNT')
        privilege+=actual
    require(privilege>0,'NATIVE SCOPE / LIVE SUPERVISOR CONTROL')
    result={}
    for trap,label in TRAPS.items():
        require(trap in totals,'NATIVE SCOPE / MISSING SERVICE '+label)
        row=totals[trap]
        require(row['partial']==row['foreign']==0,'NATIVE SCOPE / PARTIAL OR FOREIGN SERVICE')
        actual=sum(v[4] for p in phases for v in p.nodes.values() if v[2]==10 and v[1]==(0xa000a000|trap))
        require(row['calls']==actual and actual>0,'NATIVE SCOPE / EXCEPTION CALL COUNT '+label)
        rates={p.units_per_ms for p in phases};require(len(rates)==1,'NATIVE SCOPE / MIXED CLOCKS')
        result[label]=dict(calls=actual, microseconds=row['units']/rates.pop()*1000/actual,
                           excluded_interrupt_units=row['irq'])
    return result


def check(name,controls=False):
    context,phases=load(name,None)
    require(context.config.get('native_scope')=='1','NATIVE SCOPE / EXPLICIT CAPTURE')
    control=re.search(r'NATIVE SCOPE INVALID CONFIG BEGIN\n(.*?)NATIVE SCOPE INVALID CONFIG END\n',context.log,re.S)
    require(control and 'Protocol error with Rcmd' in control[1] and
            'NATIVE SERVICE SCOPE CONFIGURED\n' in context.log,'NATIVE SCOPE / CONFIGURATION CONTROLS')
    result=headers(context,phases)
    if controls:
        h=phases[0].header
        changes=[('native_scope_version',0),('native_scope_faults',1),('native_scope_exit_0',0),
                 ('native_scope_text_low',0),('native_scope_unscoped',h['native_scope_unscoped']+1),
                 ('native_scope_irq_exited',h['native_scope_irq_exited']+1),
                 ('native_scope_exec_low',0),('native_scope_exec_high',0),
                 ('native_scope_supervisor',0),('native_scope_privilege_calls',h['native_scope_privilege_calls']+1)]
        target=next(k for k in h if re.fullmatch('native_(029|055|05d|069|9b4)_calls',k) and h[k])
        prefix=target[:-5]
        changes += [(target,h[target]+1),(prefix+'foreign',1),(prefix+'units',h[prefix+'units']+1)]
        for key,value in changes:
            old=h[key];h[key]=value
            try:
                headers(context,phases)
            except SystemExit:pass
            else:raise AssertionError('NATIVE SCOPE / ACCEPTED ALTERATION '+key)
            finally:h[key]=old
        print('NATIVE SCOPE CONTROLS PASS:',len(changes),'alterations rejected')
    (context.dir/'native-service-costs.json').write_text(json.dumps(result,indent=2)+'\n')
    for label,row in result.items():print(f'NATIVE SERVICE {label}: {row["microseconds"]:.6f} us/call ({row["calls"]} calls)')
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    sub=parser.add_subparsers(dest='command',required=True)
    p=sub.add_parser('observer');p.add_argument('elf',type=Path);p.add_argument('template',type=Path);p.add_argument('output',type=Path)
    p=sub.add_parser('check');p.add_argument('name');p.add_argument('--controls',action='store_true')
    args=parser.parse_args()
    if args.command=='observer':observer(args.elf,args.template,args.output)
    else:check(args.name,args.controls)
