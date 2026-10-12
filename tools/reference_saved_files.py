"""Preserve original-written forks and import them into a fresh reference boot."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
from extract_original_data import ROOT
from install_reference_volume import hfs
from report_reference import analyze
NAMES=('Prince2.opt','Prince.hof','Fixture Save')


def export_saved(source):
    source=source.resolve()
    if not source.is_relative_to(ROOT/'ref/mame/runs'):
        raise ValueError('PERSISTENCE / SOURCE LOCATION')
    log=(source/'session.log').read_text();analyze(log)
    for marker in ('HallOfFame write-readback','SaveGame write','ResumeGame readback','volume-flushed'):
        if log.count('FILE_ROUTINE_COMPLETE '+marker)!=1:
            raise ValueError('PERSISTENCE / SOURCE COMPLETION')
    pid=(source/'mame.pid').read_text().strip()
    if not pid.isdigit():raise ValueError('PERSISTENCE / PID RECORD')
    process=subprocess.run(['ps','-p',pid,'-o','command='],capture_output=True,text=True)
    if str(source/'volume.hd') in process.stdout:
        raise ValueError('PERSISTENCE / SOURCE EMULATOR STILL RUNNING')
    output=Path(tempfile.mkdtemp(prefix='persisted-',dir=source)) # Preserve earlier exports.
    hashes={}
    hfs('hmount',str(source/'volume.hd'))
    try:
        parent=':Desktop Folder' if 'Desktop Folder' in hfs('hls','-1a',':').splitlines() else ''
        for name in NAMES:
            path=output/(name+'.macbin')
            hfs('hcopy','-m',parent+':Prince of Persia 2:Data:'+name,str(path))
            raw=path.read_bytes();size=struct.unpack_from('>I',raw,83)[0]
            if len(raw)<128+size or raw[2:2+raw[1]].decode('mac_roman')!=name:
                raise ValueError('PERSISTENCE / EXPORTED FILE')
            (output/(name+'.data')).write_bytes(raw[128:128+size])
        for path in output.iterdir():hashes[path.name]=hashlib.sha256(path.read_bytes()).hexdigest()
        (output/'sha256.json').write_text(json.dumps(hashes,indent=2)+'\n')
    finally:hfs('humount')
    return output


def install_saved(source,destination):
    output=export_saved(source)
    copies=destination/'persisted';copies.mkdir()
    hashes=json.loads((output/'sha256.json').read_text())
    expected={n+suffix for n in NAMES for suffix in ('.macbin','.data')}
    if set(hashes)!=expected:raise ValueError('PERSISTENCE / MANIFEST MEMBERS')
    for name,digest in hashes.items():
        data=(output/name).read_bytes()
        if hashlib.sha256(data).hexdigest()!=digest:raise ValueError('PERSISTENCE / HASH')
        (copies/name).write_bytes(data)
    (copies/'sha256.json').write_text(json.dumps(hashes,indent=2)+'\n')
    hfs('hmount',str(destination/'volume.hd'))
    try:
        parent=':Desktop Folder' if 'Desktop Folder' in hfs('hls','-1a',':').splitlines() else ''
        for name in NAMES:
            hfs('hcopy','-m',str(copies/(name+'.macbin')),parent+':Prince of Persia 2:Data:'+name)
    finally:hfs('humount')
