"""Verify the original wait-cursor lifetime observation in all fourteen loads."""
import argparse, hashlib, re
from pathlib import Path
from extract_original_data import ROOT, pins

def check(run):
    for name,digest in pins(ROOT/'manifest/gameplay-cursor.sha256').items():
        assert hashlib.sha256((run/name).read_bytes()).hexdigest()==digest,'CURSOR / HASH'
    text=(run/'session.log').read_text()
    assert text.count('GAMEPLAY_DISCOVERY_COMPLETE\n')==1 and 'FAIL REFERENCE' not in text,'CURSOR / COMPLETION'
    rows=re.findall(r'^GAMEPLAY_CURSOR phase=(cursor-return|cursor-purgeable) handle=([0-9A-F]+) master=([0-9A-F]+) memerr=([0-9A-F]+)$',text,re.M)
    assert len(rows)==28,'CURSOR / LOAD COUNT'
    for index,(phase,handle,master,error) in enumerate(rows):
        assert phase==('cursor-return' if index%2==0 else 'cursor-purgeable'),'CURSOR / ORDER'
        assert int(handle,16)!=0 and int(master,16)!=0 and int(error,16)==0,'CURSOR / RESULT'
        if index%2:assert rows[index-1][1:]==rows[index][1:],'CURSOR / CLEANUP CHANGED OWNER'
    print('ORIGINAL CURSOR PASS: 14 loaded returns and matching unlock/purgeable cleanup')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('run',type=Path)
    check(parser.parse_args().run)
