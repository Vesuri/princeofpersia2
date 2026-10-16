"""Functional MMU coverage per AmigaXDev's WHDLoad MMU playbook.

This checks completed file operations and normal exit, not individual audio
return values or timing. check_release_quit retains its stricter paired contract.
"""
import argparse,json,shutil,tempfile
from pathlib import Path
from extract_original_data import ROOT
from whdload_gameplay import check


def controls(source):
    assert json.loads((source/'gameplay-options.json').read_text()).get('mmu_functional'), 'MMU / FUNCTIONAL FIXTURE REQUIRED'
    with tempfile.TemporaryDirectory(prefix='mmu-controls-',dir=ROOT/'tmp') as temporary:
        base=Path(temporary)
        names=['gameplay-options.json','gdb.log','receipt.json','game/PrinceOfPersia2',
               'game/.whdl_register','game/Level 1','game/Level 1.rsrc','game/Level 1.finfo',
               'logs/debug.uae','boot/s/startup-sequence','boot/passed','boot/returned',
               'first-context.bin','first-a5.bin','moved-context.bin','moved-a5.bin']
        action=json.loads((source/'gameplay-options.json').read_text())['save_action']
        names+=[action+'-return-a5.bin']
        if action=='load':names+=['save-input.json','save-input-a5.bin','loaded-first-a5.bin']
        for name in names:
            target=base/name;target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(source/name,target)
        check(base)
        cases=[('game/.whdl_register',b'TC=8000 ',b'TC=0000 '),
               ('game/.whdl_register',b'CACR=80008000 ',b'CACR=00000000 '),
               ('game/.whdl_register',b'Return OK.',b'DEBUG caused.'),
               ('logs/debug.uae',b'cpu_model=68040',b'cpu_model=68030'),
               ('logs/debug.uae',b'cachesize=0',b'cachesize=8192'),
               ('boot/s/startup-sequence',b' MMU ',b' NOMMU '),
               ('gdb.log',b'PLAY RESTORED error=0',b'PLAY RESTORED error=1'),
               ('gdb.log',b'COMPLETED result=1',b'COMPLETED result=0')]
        for name,old,new in cases:
            target=base/name;original=target.read_bytes()
            assert old in original, 'MMU / CONTROL TARGET'
            target.write_bytes(original.replace(old,new))
            try:check(base)
            except AssertionError:pass
            else:raise AssertionError('MMU / DAMAGED EVIDENCE ACCEPTED '+name)
            target.write_bytes(original)
    print('MMU FUNCTIONAL REJECTION CONTROLS PASS:',len(cases))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=Path)
    parser.add_argument('--controls',action='store_true')
    args=parser.parse_args()
    assert json.loads((args.run/'gameplay-options.json').read_text()).get('mmu_functional')
    check(args.run)
    if args.controls:controls(args.run)
