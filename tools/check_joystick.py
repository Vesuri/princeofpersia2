"""Compare release WHDLoad joystick runs with their keyboard equivalents.

Each argument is ROUTE_INPUT=RUN from tools/test_whdload.py --route-input.
Every run must be a completed gameplay/quit pass of the same executable from
the same first-room state. The game ignores input over the first eight
updates, so the kid record is compared after 48 held updates. The first
room has a living guard, so fire there is Ctrl. Ctrl does not move the kid
within those updates, so differing from Shift-Right is the control that fire
did not choose Shift. It must match
between each joystick input and its keyboard equivalent, and differ between
inputs whose effects differ, so equality is not a stationary kid.
"""
import json,sys
from pathlib import Path

SAME=[('joy-right','key-right'),('joy-up','key-up'),('joy-second','key-up'),
      ('joy-fire-right','key-ctrl-right')]
DIFFERENT=[('key-right','none'),('key-up','none'),('key-up','key-right'),
           ('key-shift-right','key-right'),
           ('key-ctrl-right','key-shift-right')]

def kid(run,name):
    data=(run/name).read_bytes()
    assert len(data)==22266, 'JOYSTICK / WORLD EXTENT'
    return data[-0x50d2:-0x5094]

def main(args):
    runs={}
    for arg in args:
        kind,_,path=arg.partition('=')
        run=Path(path)
        options=json.loads((run/'gameplay-options.json').read_text())
        assert options.get('route_input')==kind and not options.get('save_action'), 'JOYSTICK / ROUTE INPUT '+kind
        result=json.loads((run/'gameplay-quit.json').read_text())
        assert result['result']=='WHDLOAD GAMEPLAY AND QUIT PASS' and result['updates']==48, 'JOYSTICK / GAMEPLAY PASS '+kind
        runs[kind]=(run,result['exe'])
    needed={k for pair in SAME+DIFFERENT for k in pair}
    assert needed<=set(runs), 'JOYSTICK / MISSING '+' '.join(sorted(needed-set(runs)))
    assert len({exe for _,exe in runs.values()})==1, 'JOYSTICK / ONE EXECUTABLE'
    assert len({kid(run,'first-a5.bin') for run,_ in runs.values()})==1, 'JOYSTICK / FIRST ROOM STATE'
    moved={k:kid(run,'moved-a5.bin') for k,(run,_) in runs.items()}
    for a,b in SAME:assert moved[a]==moved[b], f'JOYSTICK / {a} DIFFERS FROM {b}'
    for a,b in DIFFERENT:assert moved[a]!=moved[b], f'JOYSTICK / CONTROL {a} EQUALS {b}'
    print(f'JOYSTICK PASS: {len(SAME)} joystick/keyboard pairs, {len(DIFFERENT)} distinguishing controls')

if __name__=='__main__':main(sys.argv[1:])
