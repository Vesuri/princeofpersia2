"""A checkpoint and a subsequent Shell success are both required."""
from pathlib import Path
import argparse,re
from native_machine import check as check_machine
def check(run,setup="a1200-fast"):
    log=(run/'gdb-out.log').read_text()
    match=re.search(r'^SCAFFOLD PROBE PASS stage=3 fields=(\d+) dma=\d+ irq=\d+ view=1 vector=1$',log,re.M)
    if not match or int(match[1])<4:
        raise RuntimeError('SCAFFOLD / POSITIVE PROBE MISSING')
    if (run/'dh1/Shell.done').read_text().strip()!='SHELL PASS':
        raise RuntimeError('SCAFFOLD / SHELL DID NOT RECEIVE SUCCESS')
    check_machine(run,setup)
    print('SCAFFOLD BOOT/QUIT PASS: original OS view, VERTB, DMA and IRQ restored; Shell continued')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('run',type=Path)
    p.add_argument('--setup',default='a1200-fast',choices=['a1200-fast','a1200-030','a1200-040'])
    a=p.parse_args();check(a.run,a.setup)
