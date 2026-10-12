"""Verify shared inputs before any build, launch or packaging use."""
import hashlib,csv,os,sys,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FILES={'FSUAE_ARM','FSUAE_COMMON','KICKSTART_A1200','SETPATCH','LHA','VASM'}
def verify(group):
    count=0
    for row in csv.DictReader((ROOT/'manifest/scaffold-inputs.tsv').read_text().splitlines(),delimiter='\t'):
        if row['group']!=group:continue
        key=row['root'];base=os.environ.get(key)
        if not base:raise RuntimeError('source amiga/env.sh first: missing '+key)
        path=Path(base) if key in FILES else Path(base)/row['path']
        if Path(row['path']).parent.as_posix() in {'opt/bin','.'} and path.name in {'m68k-amiga-elf-gcc','m68k-amiga-elf-as','m68k-amiga-elf-ld','m68k-amiga-elf-objdump','m68k-amiga-elf-gdb','elf2hunk'}:
            path=Path(shutil.which(path.name) or '')
        if hashlib.sha256(path.read_bytes()).hexdigest()!=row['sha256']:
            raise RuntimeError('SCAFFOLD INPUT / SHA-256 MISMATCH: '+key+'/'+row['path'])
        count+=1
    if not count:raise RuntimeError('unknown input group: '+group)
    print(f'INPUT PASS {group}: {count} pinned files')
if __name__=='__main__':verify(sys.argv[1])
