"""Export the flushed original session save; preserve its source disk and wrapper."""
from pathlib import Path
import struct
import subprocess
from extract_original_data import ROOT
from install_reference_volume import hfs


def export(reference):
    reference=reference.resolve()
    if not reference.is_relative_to(ROOT/'ref/mame/runs'):
        raise ValueError('GAMEPLAY / REFERENCE LOCATION')
    log=(reference/'session.log').read_text()
    for marker in ('GAMEPLAY_VOLUME_FLUSHED','GAMEPLAY_REFERENCE_COMPLETE route=save-load loaded_frames=1'):
        if log.count(marker+'\n')!=1:raise ValueError('GAMEPLAY / ORIGINAL COMPLETION '+marker)
    if 'FAIL REFERENCE' in log:raise ValueError('GAMEPLAY / ORIGINAL FAILURE')
    pid=(reference/'mame.pid').read_text().strip()
    if not pid.isdigit():raise ValueError('GAMEPLAY / PID RECORD')
    process=subprocess.run(['ps','-p',pid,'-o','command='],capture_output=True,text=True)
    if str(reference/'volume.hd') in process.stdout:raise ValueError('GAMEPLAY / ORIGINAL STILL RUNNING')
    wrapper=reference/'saved-Level-1.macbin'
    if not wrapper.exists():
        hfs('hmount',str(reference/'volume.hd'))
        try:
            parent=':Desktop Folder' if 'Desktop Folder' in hfs('hls','-1a',':').splitlines() else ''
            hfs('hcopy','-m',parent+':Prince of Persia 2:Level 1',str(wrapper))
        finally:hfs('humount')
    raw=wrapper.read_bytes()
    if len(raw)<128 or raw[1]!=7 or raw[2:9]!=b'Level 1':raise ValueError('GAMEPLAY / SAVED WRAPPER')
    size,resource=struct.unpack_from('>II',raw,83)
    resource_start=128+((size+127)//128)*128
    if resource_start+resource>len(raw):raise ValueError('GAMEPLAY / SAVED FORK EXTENTS')
    outputs={'data':raw[128:128+size],'rsrc':raw[resource_start:resource_start+resource],
             'type-creator':raw[65:73]}
    for suffix,data in outputs.items():
        path=reference/('saved-Level-1.'+suffix)
        if path.exists():
            if path.read_bytes()!=data:raise ValueError('GAMEPLAY / EXISTING EXPORT DIFFERS '+suffix)
        else:path.write_bytes(data)
    print('GAMEPLAY SAVE EXPORT PASS: data/resource forks and type/creator; wrapper preserved')


if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference',type=Path)
    export(parser.parse_args().reference)
