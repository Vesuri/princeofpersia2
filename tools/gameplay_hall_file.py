"""Export the positively completed original Hall of Fame write without altering its disk."""
import argparse,struct,subprocess,hashlib
from pathlib import Path
from extract_original_data import ROOT,pins
from install_reference_volume import hfs

def export(reference):
    reference=reference.resolve()
    assert reference.is_relative_to(ROOT/'ref/mame/runs'),'HALL / REFERENCE LOCATION'
    log=(reference/'session.log').read_text()
    for marker in ('GAMEPLAY_HALL_NAME_ACCEPTED','GAMEPLAY_HALL_WRITTEN','GAMEPLAY_HALL_RETURN','GAMEPLAY_HALL_COMPLETE'):
        assert log.count(marker+'\n')==1,'HALL / COMPLETION '+marker
    assert 'FAIL REFERENCE' not in log,'HALL / REFERENCE FAILURE'
    pid=(reference/'mame.pid').read_text().strip();assert pid.isdigit()
    process=subprocess.run(['ps','-p',pid,'-o','command='],capture_output=True,text=True)
    assert str(reference/'volume.hd') not in process.stdout,'HALL / EMULATOR RUNNING'
    wrapper=reference/'saved-Prince.hof.macbin'
    if not wrapper.exists():
        hfs('hmount',str(reference/'volume.hd'))
        try:
            parent=':Desktop Folder' if 'Desktop Folder' in hfs('hls','-1a',':').splitlines() else ''
            hfs('hcopy','-m',parent+':Prince of Persia 2:Data:Prince.hof',str(wrapper))
        finally:hfs('humount')
    raw=wrapper.read_bytes();assert len(raw)>=128 and raw[2:2+raw[1]]==b'Prince.hof','HALL / WRAPPER'
    size=struct.unpack_from('>I',raw,83)[0];assert len(raw)>=128+size,'HALL / FORK EXTENT'
    data=raw[128:128+size];out=reference/'saved-Prince.hof.data'
    if out.exists():assert out.read_bytes()==data,'HALL / EXISTING EXPORT DIFFERS'
    else:out.write_bytes(data)
    print('HALL EXPORT PASS: preserved original-written data fork')

def install_reference(reference,destination):
    export(reference)
    data=(reference/'saved-Prince.hof.data').read_bytes()
    assert hashlib.sha256(data).hexdigest()==pins(ROOT/'manifest/gameplay-hall-reference.sha256')['saved-Prince.hof.data'],'HALL / IMPORT PIN'
    wrapper=reference/'saved-Prince.hof.macbin'
    raw=wrapper.read_bytes();size=struct.unpack_from('>I',raw,83)[0]
    assert raw[128:128+size]==data,'HALL / IMPORT WRAPPER'
    hfs('hmount',str(destination/'volume.hd'))
    try:
        parent=':Desktop Folder' if 'Desktop Folder' in hfs('hls','-1a',':').splitlines() else ''
        hfs('hcopy','-m',str(wrapper.resolve()),parent+':Prince of Persia 2:Data:Prince.hof')
    finally:hfs('humount')
    (destination/'imported-Prince.hof.data').write_bytes(data)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('reference',type=Path);export(p.parse_args().reference)
