"""Require a positive, clean 4 MB release startup rejection, never a timeout."""
import argparse
import json
from pathlib import Path
import tempfile


def check(run):
    receipt=json.loads((run/'build-receipt.json').read_text())
    assert set(receipt['flags'].split())=={'-DPOP2_CORE','-DPOP2_DISPLAY','-DPOP2_INPUT','-DPOP2_UI','-DPOP2_AUDIO'}
    config=dict(line.split('=',1) for line in (run/'logs/debug.uae').read_text().splitlines() if '=' in line)
    for k,v in dict(cpu_model='68030',mmu_model='68030',fastmem_size='4',chipmem_size='4',cpu_frequency='50000000',cpu_data_cache='true').items():assert config.get(k)==v
    log=(run/'gdb-out.log').read_text()
    assert log.count('HEAP LOW MEMORY reason=CORE / MEMORY exception=0 pc=00000000\n')==1
    assert log.count('HEAP LOW MEMORY OBSERVED\n')==1
    assert log.count('HEAP LOW MEMORY preparation stage=1 error=0 vector=0 fields=0\n')==1
    print('HEAP LOW MEMORY PASS: 4 MB release rejects CORE / MEMORY without exception, before hardware takeover')


def controls(run):
    names=('build-receipt.json','logs/debug.uae','gdb-out.log')
    with tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parents[1]/'tmp',prefix='heap-low-memory-controls-') as directory:
        target=Path(directory)
        for name in names:
            path=target/name;path.parent.mkdir(parents=True,exist_ok=True)
            path.write_bytes((run/name).read_bytes())
        changes=(('build-receipt.json','-DPOP2_CORE','-DPOP2_VERIFY'),
                 ('logs/debug.uae','fastmem_size=4','fastmem_size=8'),
                 ('gdb-out.log','CORE / MEMORY','CORE / OTHER'),
                 ('gdb-out.log','exception=0','exception=2'),
                 ('gdb-out.log','stage=1','stage=8'),
                 ('gdb-out.log','HEAP LOW MEMORY OBSERVED','HEAP LOW MEMORY MISSING'))
        for name,old,new in changes:
            path=target/name;good=path.read_text();bad=good.replace(old,new,1)
            assert bad!=good
            path.write_text(bad)
            try:check(target)
            except AssertionError:pass
            else:raise AssertionError('accepted altered low-memory evidence')
            finally:path.write_text(good)
    print('HEAP LOW MEMORY CONTROLS PASS: six build/configuration/stop changes rejected')



if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path);p.add_argument('--controls',action='store_true')
    a=p.parse_args();check(a.run)
    if a.controls:controls(a.run)
