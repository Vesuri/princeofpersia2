"""A checkpoint and a subsequent Shell success are both required."""
from pathlib import Path
import sys,re
def check(run):
    log=(run/'gdb-out.log').read_text()
    match=re.search(r'^SCAFFOLD PROBE PASS stage=3 fields=(\d+) dma=\d+ irq=\d+ view=1 vector=1$',log,re.M)
    if not match or int(match[1])<4:
        raise RuntimeError('SCAFFOLD / POSITIVE PROBE MISSING')
    if (run/'dh1/Shell.done').read_text().strip()!='SHELL PASS':
        raise RuntimeError('SCAFFOLD / SHELL DID NOT RECEIVE SUCCESS')
    config=dict(line.split('=',1) for line in (run/'logs/debug.uae').read_text().splitlines() if '=' in line)
    expected={'cpu_model':'68020','chipset':'aga','chipmem_size':'4','fastmem_size':'8','ntsc':'false','cpu_speed':'real','cachesize':'0','cpu_frequency':'14187580'}
    if any(config.get(k)!=v for k,v in expected.items()):raise RuntimeError('SCAFFOLD / EFFECTIVE MACHINE MISMATCH')
    print('SCAFFOLD BOOT/QUIT PASS: original OS view, VERTB, DMA and IRQ restored; Shell continued')
if __name__=='__main__':check(Path(sys.argv[1]))
