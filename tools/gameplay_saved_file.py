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


def macbinary_files(raw, name):
    """Decode a preserved save wrapper into the port's existing fork layout."""
    from datetime import datetime, timedelta, timezone
    from extract_original_data import metadata
    encoded=name.encode('mac-roman')
    if len(raw)<128 or raw[1]!=len(encoded) or raw[2:2+len(encoded)]!=encoded:
        raise ValueError('GAMEPLAY IMPORT / WRAPPER NAME')
    size,resource=struct.unpack_from('>II',raw,83)
    offset=128+((size+127)//128)*128
    if offset+resource>len(raw):raise ValueError('GAMEPLAY IMPORT / FORK EXTENTS')
    data=raw[128:128+size];rsrc=raw[offset:offset+resource];kind=raw[65:73]
    # Inverse of install_reference_volume.macbinary; use the existing AFI1 encoder.
    finder = raw[65:73] + bytes((raw[73], raw[101])) + raw[75:81] + bytes(16)
    epoch = datetime(1904, 1, 1, tzinfo=timezone.utc)
    date = lambda start: (epoch + timedelta(seconds=int.from_bytes(raw[start:start+4], 'big'))).strftime('%Y-%m-%d %H:%M:%S %z')
    info = metadata([dict(XADFileType=int.from_bytes(kind[:4], 'big'),
                          XADFileCreator=int.from_bytes(kind[4:], 'big'),
                          XADFinderFlags=int.from_bytes(finder[8:10], 'big'),
                          XADCreationDate=date(91), XADLastModificationDate=date(95))], finder)
    return {name: data, name+'.rsrc': rsrc, name+'.finfo': info}


def install_native(reference, destination):
    """Stage an original-created save before a fresh diagnostic Amiga boot."""
    import hashlib
    import json
    from extract_original_data import pins
    reference = reference.resolve()
    for name, expected in pins(ROOT / 'manifest/gameplay-save-reference.sha256').items():
        if hashlib.sha256((reference / name).read_bytes()).hexdigest() != expected:
            raise ValueError('GAMEPLAY IMPORT / REFERENCE PIN ' + name)
    for name, expected in pins(ROOT / 'manifest/gameplay-import-reference.sha256').items():
        if hashlib.sha256((reference / name).read_bytes()).hexdigest() != expected:
            raise ValueError('GAMEPLAY IMPORT / WRAPPER PIN ' + name)
    raw = (reference / 'saved-Level-1.macbin').read_bytes()
    if len(raw) < 128 or raw[1] != 7 or raw[2:9] != b'Level 1':
        raise ValueError('GAMEPLAY IMPORT / WRAPPER')
    size, resource = struct.unpack_from('>II', raw, 83)
    offset = 128 + ((size + 127) // 128) * 128
    data = (reference / 'saved-Level-1.data').read_bytes()
    rsrc = (reference / 'saved-Level-1.rsrc').read_bytes()
    kind = (reference / 'saved-Level-1.type-creator').read_bytes()
    if (size != len(data) or resource != len(rsrc) or raw[128:128+size] != data or
            raw[offset:offset+resource] != rsrc or raw[65:73] != kind):
        raise ValueError('GAMEPLAY IMPORT / WRAPPER FORKS')
    files = macbinary_files(raw, 'Level 1')
    for name in files:
        if (destination / name).exists():
            raise ValueError('GAMEPLAY IMPORT / FRESH DESTINATION REQUIRED')
    for name, body in files.items():
        (destination / name).write_bytes(body)
    receipt = {'source': str(reference.relative_to(ROOT)),
               'files': {name: hashlib.sha256(body).hexdigest() for name, body in files.items()}}
    (destination.parent / 'mac-save-import.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('GAMEPLAY MAC SAVE STAGED: unchanged original forks and Finder metadata')


def install_reference(reference, destination):
    """Import the pinned save into a fresh Macintosh reference volume."""
    import tempfile
    # Reuse fork, wrapper and metadata validation before touching the new disk.
    with tempfile.TemporaryDirectory(prefix='mac-save-stage-', dir=ROOT / 'tmp') as directory:
        stage = Path(directory) / 'dh1'; stage.mkdir()
        install_native(reference, stage)
    hfs('hmount', str(destination / 'volume.hd'))
    try:
        parent = ':Desktop Folder' if 'Desktop Folder' in hfs('hls', '-1a', ':').splitlines() else ''
        folder = parent + ':Prince of Persia 2'
        if 'Level 1' in hfs('hls', '-1a', folder).splitlines():
            raise ValueError('GAMEPLAY IMPORT / ORIGINAL SAVE ALREADY EXISTS')
        hfs('hcopy', '-m', str(reference.resolve() / 'saved-Level-1.macbin'), folder + ':Level 1')
    finally:
        hfs('humount')
    print('GAMEPLAY MAC SAVE IMPORTED INTO FRESH ORIGINAL VOLUME')


if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference',type=Path)
    export(parser.parse_args().reference)
