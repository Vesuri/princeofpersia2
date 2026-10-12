"""Bind diagnostic flags, executable and debugger symbols; reject stale pairs."""
from pathlib import Path
import hashlib,json,sys
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'amiga/out/PrinceOfPersia2'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def verify():
    receipt=json.loads(BASE.with_suffix('.json').read_text())
    for kind in ['exe','elf']:
        if digest(BASE.with_suffix('.'+kind))!=receipt[kind]:raise RuntimeError('BUILD RECEIPT / '+kind+' MISMATCH')
    return receipt
if __name__=='__main__':
    if sys.argv[1]=='write':
        BASE.with_suffix('.json').write_text(json.dumps(dict(exe=digest(BASE.with_suffix('.exe')),elf=digest(BASE.with_suffix('.elf')),flags=sys.argv[2]),indent=2)+'\n')
    else:verify()
