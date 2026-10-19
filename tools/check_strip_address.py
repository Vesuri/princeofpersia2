"""Original tagging/CCR contract and native physical-pointer adaptation."""
import argparse
import re
from pathlib import Path

VALUES=(0,0x123456,0x41c27330,0x40000000,0x7fff8000,0x81234567)

def check(text):
    assert 'FAIL REFERENCE' not in text
    assert text.count('STRIP ORIGINAL COMPLETE cases=192\n')==1
    rows=re.findall(r'^STRIP input=([0-9A-F]+) output=([0-9A-F]+) cc=([0-9A-F]+)->([0-9A-F]+)$',text,re.M)
    expected=[]
    for value in VALUES:
        result=value&0xffffff
        for cc in range(32):
            flags=(cc&16)|(4 if result&65535==0 else 0)|(8 if result&0x8000 else 0)
            expected.append((value,result,cc,flags))
    assert [tuple(int(v,16) for v in row) for row in rows]==expected

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);a=p.parse_args()
    text=(a.original/'session.log').read_text();check(text)
    for before,after in [('cases=192','cases=191'),('output=00C27330','output=41C27330'),('cc=00->04','cc=00->00')]:
        changed=text.replace(before,after,1);assert changed!=text
        try:check(changed)
        except AssertionError:pass
        else:raise AssertionError('STRIP / ACCEPTED ALTERED EVIDENCE')
    print('STRIP ORIGINAL PASS: 192 tagging/CCR cases; three altered records rejected')

if __name__=='__main__':main()
