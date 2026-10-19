"""Deterministic installer candidate and legacy scaffold distributions."""
from pathlib import Path
import hashlib,os,sys,tempfile
from scaffold_inputs import verify
from build_receipt import verify as receipt
ROOT=Path(__file__).resolve().parents[1]
INSTALLER_SOURCE = ('arsenic.h decode.inc install.c install_config.h io.h io_amiga.c '
                    'io_unix.c originals.h sha256.c sha256.h sit13_tables.h start.s '
                    'support.h support.inc').split()
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

def release_package():
    """Installer distribution: generated originals are produced on the user's Amiga."""
    import json
    from extract_original_data import pins
    from amigaxdev import release
    verify('release')
    expected={'-DPOP2_CORE','-DPOP2_DISPLAY','-DPOP2_INPUT','-DPOP2_UI','-DPOP2_AUDIO'}
    if set(receipt()['flags'].split())!=expected:raise RuntimeError('PACKAGING / RELEASE FLAGS REQUIRED')
    version=(ROOT/'VERSION').read_text().strip()
    icons=ROOT/'release/icons'
    transfer=(ROOT/'build/display/video-transfer.bin').read_bytes()
    if hashlib.sha256(transfer).hexdigest()!=pins(ROOT/'manifest/display-inputs.sha256')['video-transfer.bin']:
        raise RuntimeError('PACKAGING / VIDEO TRANSFER PIN')
    files={
      'PrinceOfPersia2':(ROOT/'amiga/out/PrinceOfPersia2.exe').read_bytes(),
      'PrinceOfPersia2.slave':(ROOT/'build/whdload/PrinceOfPersia2.slave').read_bytes(),
      'Pop2InstallData':(ROOT/'build/install-data/Pop2InstallData.exe').read_bytes(),
      'PrinceOfPersia2.inf':release.installer_icon(icons,'Prince of Persia 2',game=True),
      'Install':(ROOT/'release/Install').read_bytes(),
      'Install.info':release.installer_icon(icons,'Prince of Persia 2'),
      'ReadMe':(ROOT/'release/ReadMe').read_bytes(),
      'ReadMe.info':release.template_icon(icons,'readme.info.b64'),
      'LICENSE.LGPL.txt':(ROOT/'tools/install-data/COPYING.LIB').read_bytes(),
      'video-transfer.bin':transfer,
    }
    for name in INSTALLER_SOURCE:
        files['InstallerSource/'+name]=(ROOT/'tools/install-data'/name).read_bytes()
    files['InstallerSource/Build.txt']=(ROOT/'release/InstallerSource.txt').read_bytes()
    original_hashes=set(pins(ROOT/'manifest/runtime-data.sha256').values())|set(pins(ROOT/'manifest/originals.sha256').values())
    release.refuse_originals(files,original_hashes)
    tempfile.tempdir=str(ROOT/'tmp')
    prefix='Prince of Persia 2 Install'
    data=release.archive(prefix,files,release.template_icon(icons,'drawer.info.b64'),release.dos_stamp(2026,10,19))
    out=ROOT/'dist'/('PrinceOfPersia2-'+version+'.lha')
    if out.exists() and out.read_bytes()!=data:
        backup=ROOT/'.replaced'/('release-'+hashlib.sha256(out.read_bytes()).hexdigest())
        backup.mkdir(parents=True,exist_ok=True);out.rename(backup/out.name)
    if not out.exists():release.write(out,data)
    report={'version':version,'date':'19.10.2026','sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data),'members':{n:hashlib.sha256(b).hexdigest() for n,b in sorted(files.items())},'qualification':'candidate; item 14 gates remain open'}
    out.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
    print('RELEASE CANDIDATE',out.name,report['sha256'])
    return out,files

if __name__=='__main__':
    if '--scaffold' in sys.argv:package()
    else:release_package()
