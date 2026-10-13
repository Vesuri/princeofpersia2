"""Deterministic scaffold-only LHA; explicit members, no installer/game claims."""
from pathlib import Path
import hashlib,os,sys,tempfile
from scaffold_inputs import verify
from build_receipt import verify as receipt
ROOT=Path(__file__).resolve().parents[1]
def package():
    verify('release')
    sys.path.insert(0,os.environ['AMIGA_PYLIB'])
    from amigaxdev import release
    # This development archive contains only the empty scaffold. Its retained
    # verification observers are allowed; gameplay and fault-injection flags are not.
    if set(receipt()['flags'].split())-{'-DPOP2_VERIFY'}:
        raise RuntimeError('PACKAGING / DIAGNOSTIC FLAGS')
    version=(ROOT/'VERSION').read_text().strip()
    files={'PrinceOfPersia2':(ROOT/'amiga/out/PrinceOfPersia2.exe').read_bytes(),
           'PrinceOfPersia2.slave':(ROOT/'build/whdload/PrinceOfPersia2.slave').read_bytes(),
           'ReadMe':(ROOT/'release/ReadMe').read_bytes()}
    hashes={line.split()[0] for name in ['originals.sha256','runtime-data.sha256'] for line in (ROOT/'manifest'/name).read_text().splitlines()}
    release.refuse_originals(files,hashes)
    tempfile.tempdir=str(ROOT/'tmp');(ROOT/'tmp').mkdir(exist_ok=True)
    stamp=release.dos_stamp(2026,10,12)
    data=b''.join(release.member('PrinceOfPersia2-scaffold/'+name,files[name],stamp) for name in sorted(files))+b'\0'
    out=ROOT/'dist'/('PrinceOfPersia2-scaffold-'+version+'.lha')
    if out.exists() and out.read_bytes()!=data:
        saved=ROOT/'.replaced'/('package-'+hashlib.sha256(out.read_bytes()).hexdigest())
        saved.mkdir(parents=True,exist_ok=True)
        out.rename(saved/out.name)
    release.write(out,data)
    print('SCAFFOLD ARCHIVE',out.name,hashlib.sha256(data).hexdigest())
    return out,files
if __name__=='__main__':package()
