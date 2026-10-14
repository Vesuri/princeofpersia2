"""Check the original ending's reached solid-black default pen, not video captures."""
import argparse,hashlib,struct
from pathlib import Path
from extract_original_data import ROOT,pins

def check(run):
    for name,digest in pins(ROOT/'manifest/gameplay-ending-paint.sha256').items():
        assert hashlib.sha256((run/name).read_bytes()).hexdigest()==digest,'ENDING PEN / PIN'
    text=(run/'session.log').read_text()
    assert text.count('GAMEPLAY_PAINT_RETURN id=1\n')==1 and text.count('GAMEPLAY_ENDING_COMPLETE\n')==1 and 'FAIL REFERENCE' not in text,'ENDING PEN / COMPLETION'
    port=(run/'paint-1-port.bin').read_bytes();pattern=(run/'paint-1-pattern.bin').read_bytes()
    assert len(port)==108 and len(pattern)==28
    assert pattern[:2]==b'\0\0' and (run/'paint-1-pattern-data.bin').read_bytes()==b'\xff'*8,'ENDING PEN / SOLID'
    assert struct.unpack_from('>H',port,56)[0]==8 and struct.unpack_from('>I',port,80)[0]==255 and port[36:42]==bytes(6),'ENDING PEN / MODE COLOUR'
    print('ENDING PEN PASS: reached original solid black default; positive paint and ending returns')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path);check(p.parse_args().run)
