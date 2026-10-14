"""Export completed original-created profiling saves into the Amiga fork layout."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import subprocess
from extract_original_data import ROOT
from gameplay_saved_file import macbinary_files
from install_reference_volume import hfs


def export(reference,destination):
    reference=reference.resolve();destination=destination.resolve()
    if not reference.is_relative_to(ROOT/'ref/mame/runs') or not destination.is_relative_to(ROOT/'tmp'):
        raise ValueError('PROFILE EXPORT / IGNORED LOCATIONS REQUIRED')
    log=(reference/'session.log').read_text()
    assert log.count('GAMEPLAY_DISCOVERY_COMPLETE\n')==1 and 'FAIL REFERENCE' not in log, 'PROFILE EXPORT / COMPLETION'
    names=[f'Profile {level:02}' for level in range(1,15)]
    for name in names:assert log.count(f'PROFILE_SAVE_COMPLETE name={name}\n')==1, 'PROFILE EXPORT / SAVE COMPLETION'
    pid=(reference/'mame.pid').read_text().strip();assert pid.isdigit()
    process=subprocess.run(['ps','-p',pid,'-o','command='],capture_output=True,text=True)
    assert str(reference/'volume.hd') not in process.stdout, 'PROFILE EXPORT / EMULATOR STILL RUNNING'
    wrappers=reference/'profile-wrappers';wrappers.mkdir(exist_ok=True)
    missing=[name for name in names if not (wrappers/(name+'.macbin')).exists()]
    if missing:
        hfs('hmount',str(reference/'volume.hd'))
        try:
            parent=':Desktop Folder' if 'Desktop Folder' in hfs('hls','-1a',':').splitlines() else ''
            for name in missing:
                hfs('hcopy','-m',parent+':Prince of Persia 2:'+name,str(wrappers/(name+'.macbin')))
        finally:hfs('humount')
    checkpoints=re.findall(r'^PROFILE_SAVE_BEGIN name=(Profile \d\d) level=(\d+) room=(\d+)$',log,re.M)
    assert [(name,int(level)) for name,level,_ in checkpoints]==[(name,i) for i,name in enumerate(names,1)], 'PROFILE EXPORT / LEVEL ENTRIES'
    outputs={};sources={}
    for name in names:
        raw=(wrappers/(name+'.macbin')).read_bytes()
        outputs.update(macbinary_files(raw,name))
        assert len(outputs[name])==100 and outputs[name+'.rsrc']==b'', 'PROFILE EXPORT / SAVE SHAPE'
        assert outputs[name+'.finfo'][4:8]==b'SAVE', 'PROFILE EXPORT / SAVE TYPE'
        sources[name]=hashlib.sha256(raw).hexdigest()
    destination.mkdir(parents=True,exist_ok=True)
    for name,body in outputs.items():
        path=destination/name
        if path.exists():assert path.read_bytes()==body, 'PROFILE EXPORT / EXISTING FILE DIFFERS'
        else:path.write_bytes(body)
    receipt=dict(source=str(reference.relative_to(ROOT)),assisted_level_entries=True,
                 checkpoints=[dict(name=name,level=int(level),room=int(room)) for name,level,room in checkpoints],
                 wrappers=sources,files={name:hashlib.sha256(body).hexdigest() for name,body in outputs.items()})
    (destination/'manifest.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(f'PROFILE SAVE EXPORT PASS: 14 unchanged data forks with resource forks and Finder metadata in {destination}')


def install_profile(source,level,destination):
    """Stage one host-verified original save for release play or a load check."""
    assert 1<=level<=14, 'PROFILE IMPORT / LEVEL'
    receipt=json.loads((source/'manifest.json').read_text())
    name=f'Profile {level:02}'
    staged={}
    for suffix in ('','.rsrc','.finfo'):
        filename=name+suffix;body=(source/filename).read_bytes()
        assert hashlib.sha256(body).hexdigest()==receipt['files'][filename], 'PROFILE IMPORT / HASH'
        target=destination/filename
        assert not target.exists(), 'PROFILE IMPORT / FRESH DESTINATION'
        staged[target]=body
    for target,body in staged.items():target.write_bytes(body)
    print(f'PROFILE IMPORT STAGED: {name}')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('reference',type=Path)
    p.add_argument('--destination',type=Path,default=ROOT/'tmp/profiling-saves')
    a=p.parse_args();export(a.reference,a.destination)
