"""Reject corrupted frame-boundary evidence without modifying retained captures."""
import contextlib
import io
import re
from pathlib import Path
from unittest.mock import patch
import argparse
from check_frame_boundaries import check

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('run',type=Path)
args=parser.parse_args()
read_bytes,read_text=Path.read_bytes,Path.read_text

def reject(name,mutate,why):
    def altered_bytes(path,*a,**kw):
        data=read_bytes(path,*a,**kw)
        return mutate(data) if path==args.run/name else data
    def altered_text(path,*a,**kw):
        text=read_text(path,*a,**kw)
        return mutate(text) if path==args.run/name else text
    with patch.object(Path,'read_bytes',altered_bytes),patch.object(Path,'read_text',altered_text),contextlib.redirect_stdout(io.StringIO()):
        try:check(args.run)
        except (AssertionError,SystemExit):return
    raise AssertionError('accepted corrupted evidence: '+why)

reject('gdb-out.log',lambda s:re.sub(r'^FRAME_BOUNDARY index=1 .*\n','',s,flags=re.M),'missing frame')
reject('gdb-out.log',lambda s:s.replace('FRAME_BOUNDARY index=1 game=1','FRAME_BOUNDARY index=2 game=1'),'duplicate frame')
reject('gdb-out.log',lambda s:s.replace('FRAME_BOUNDARY index=1 game=1','FRAME_BOUNDARY index=1 game=999'),'wrong game counter')
reject('gdb-out.log',lambda s:re.sub(r'(FRAME_BOUNDARY index=1 game=1 crop=)\d+',r'\g<1>97',s),'invalid viewport')
reject('frame-1-planes.bin',lambda b:bytes([b[0]^1])+b[1:],'wrong plane byte')
reject('frame-1-copper.bin',lambda b:b[:-1]+bytes([b[-1]^1]),'invalid copper end')
reject('frame-1-colors.bin',lambda b:b[:-1],'short colour table')
reject('frame-1-kid.bin',lambda b:bytes([b[0]^1])+b[1:],'wrong player record')
print('PASS frame-boundary checker: eight missing/order/counter/viewport/pixel/copper/colour/player controls')
