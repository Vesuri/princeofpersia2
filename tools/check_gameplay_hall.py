"""Check the original completed name-entry/write path and optional native Hall endpoint."""
import argparse,hashlib,struct
from pathlib import Path
from extract_original_data import ROOT,pins

def rows(data):
    assert len(data)>=2,'HALL / TABLE HEADER'
    count=struct.unpack_from('>H',data)[0]
    assert 0<count<=5 and len(data)>=2+count*30,'HALL / TABLE EXTENT'
    result=[]
    for i in range(count):
        record=data[2+i*30:32+i*30];name=record[:28]
        assert b'\0' in name,'HALL / NAME TERMINATOR'
        result.append((name.split(b'\0',1)[0],struct.unpack_from('>H',record,28)[0]))
    return result

def check(reference,native=None,repeat=None):
    for name,digest in pins(ROOT/'manifest/gameplay-hall-reference.sha256').items():
        assert hashlib.sha256((reference/name).read_bytes()).hexdigest()==digest,'HALL / PIN '+name
    log=(reference/'session.log').read_text()
    for marker in ('GAMEPLAY_HALL_NAME_ACCEPTED','GAMEPLAY_HALL_WRITTEN','GAMEPLAY_HALL_RETURN','GAMEPLAY_HALL_COMPLETE'):
        assert log.count(marker+'\n')==1,'HALL / ORIGINAL ENDPOINT'
    assert 'FAIL REFERENCE' not in log
    before=(reference/'hall-before.bin').read_bytes();after=(reference/'hall-after.bin').read_bytes()
    assert before[:2]==after[:2] and before[30:]==after[30:],'HALL / ORIGINAL SCORE PRESERVED'
    expected=rows(after);assert expected==[(b'codex',75)],'HALL / ORIGINAL NAME AND SCORE'
    assert rows((reference/'saved-Prince.hof.data').read_bytes())==expected,'HALL / ORIGINAL PERSISTED'
    if repeat:
        repeated=(repeat/'session.log').read_text()
        for marker in ('GAMEPLAY_HALL_NAME_ACCEPTED','GAMEPLAY_HALL_WRITTEN','GAMEPLAY_HALL_RETURN','GAMEPLAY_HALL_COMPLETE'):
            assert repeated.count(marker+'\n')==1,'HALL / REPEAT ENDPOINT'
        assert 'FAIL REFERENCE' not in repeated
        assert rows((repeat/'hall-after.bin').read_bytes())==expected,'HALL / REPEAT TABLE'
        assert rows((repeat/'saved-Prince.hof.data').read_bytes())==expected,'HALL / REPEAT PERSISTED'
    if native:
        from check_core_result import restored
        text=(native/'gdb-out.log').read_text();restored(native,text,'a1200-030')
        assert text.count('CORE REASON GAMEPLAY ROUTE / HALL COMPLETE\n')==1,'HALL / NATIVE ENDPOINT'
        assert 'INTEGRATION AUDIT code=1 input=1\n' in text,'HALL / AUDIT'
        assert 'HALL INPUT edit=6 confirm=1\n' in text,'HALL / INPUT PATH'
        assert rows((native/'dh1/Data/Prince.hof').read_bytes())==expected,'HALL / NATIVE PERSISTED'
    print('GAMEPLAY HALL PASS: original name, score and flushed file'+('; native completion/input/persisted pair' if native else ' (native not checked)'))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('reference',type=Path);p.add_argument('--native',type=Path)
    p.add_argument('--repeat',type=Path)
    a=p.parse_args();check(a.reference,a.native,a.repeat)
