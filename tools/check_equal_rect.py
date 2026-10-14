"""Check the original EqualRect Boolean/stack fixture and shared helper."""
import argparse, hashlib, re, subprocess
from pathlib import Path
from extract_original_data import ROOT, pins

def check(run):
    for name,digest in pins(ROOT/'manifest/gameplay-equal-rect.sha256').items():
        assert hashlib.sha256((run/name).read_bytes()).hexdigest()==digest,'EQUAL RECT / HASH'
    text=(run/'session.log').read_text()
    assert text.count('EQUAL_RECT_COMPLETE calls=36\n')==1 and 'FAIL REFERENCE' not in text,'EQUAL RECT / COMPLETION'
    records=re.findall(r'^EQUAL_RECT_RESULT a=(\d+) b=(\d+) result=(\d) pad=([0-9A-F]{2})$',text,re.M)
    expected=[(str(a),str(b),str(int(a==b or {a,b}=={1,2})),'5A') for a in range(1,7) for b in range(1,7)]
    assert records==expected,'EQUAL RECT / RESULTS'
    output=ROOT/'build/host/test_equal_rect';output.parent.mkdir(parents=True,exist_ok=True)
    subprocess.run(['clang++','-std=c++17','-fsanitize=address,undefined','-I',str(ROOT/'src'),str(ROOT/'tools/test_equal_rect.cpp'),'-o',str(output)],check=True)
    subprocess.run([str(output)],check=True)
    print('EQUAL RECT PASS: 36 original cases, preserved arguments and Boolean padding; shared helper and alias controls')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('run',type=Path)
    check(parser.parse_args().run)
