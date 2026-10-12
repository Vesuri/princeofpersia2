"""Compare CPU-executed display contracts with the captured original records."""
from pathlib import Path
import argparse,re,struct
from check_core_result import restored

def check(run,reference):
    log=(run/'gdb-out.log').read_text();restored(run,log)
    assert 'CORE REASON DISPLAY / FIXTURE COMPLETE\n' in log
    assert 'CORE EXCEPTION vector=0 pc=00000000\n' in log
    m=re.search(r'DISPLAY SERVICES probe=([0-9a-f]+) device=([0-9a-f]+) port=([0-9a-f]+) map=([0-9a-f]+) result=(\d+) lock=(\d+) base=([0-9a-f]+) unlocked=([0-9a-f]+)',log)
    assert m and int(m[5])==0 and int(m[6])==256 and m[7]==m[8] and int(m[7],16),'DISPLAY / TRAP RESULTS'
    rlog=(reference/'session.log').read_text()
    assert 'DISPLAY_TRACE_COMPLETE calls=' in rlog and 'FAIL REFERENCE' not in rlog,'DISPLAY / ORIGINAL COMPLETION'
    calls=re.findall(r'DISPLAY_CALL id=(\d+) .*trap=AB1D selector=0000 ',rlog)
    assert calls,'DISPLAY / ORIGINAL NEWGWORLD'
    ident=next(i for i in calls if (reference/f'display-{i}-new-map.bin').read_bytes()[6:14]==struct.pack('>hhhh',0,0,384,512))
    for name,offsets,size in [('port',(2,8,24,28,32,58,62),108),('map',(0,42),50)]:
        actual=bytearray((run/f'display-service-{name}.bin').read_bytes())
        expected=bytearray((reference/f'display-{ident}-new-{name}.bin').read_bytes())
        assert len(actual)==len(expected)==size
        for at in offsets:
            assert int.from_bytes(actual[at:at+4],'big') and int.from_bytes(expected[at:at+4],'big'),'DISPLAY / NULL OWNED POINTER'
            actual[at:at+4]=expected[at:at+4]=bytes(4)
        assert actual==expected,'DISPLAY / '+name.upper()+' RECORD'
    originalDevice=(reference/'display-1-before-device.bin').read_bytes()
    device=(run/'display-service-device.bin').read_bytes()
    assert len(device)==62
    for start,end in ((0,6),(10,22),(26,50),(58,62)):
        assert device[start:end]==originalDevice[start:end],'DISPLAY / GDEVICE PUBLIC FIELDS'
    screen=(run/'display-service-screen.bin').read_bytes()
    originalScreen=(reference/'display-1-before-screen-map.bin').read_bytes()
    assert len(screen)==50 and screen[4:42]==originalScreen[4:42] and screen[46:]==originalScreen[46:],'DISPLAY / MAIN PIXMAP'
    probe=(run/'display-service-probe.bin').read_bytes()
    original=(reference/'display-1-after-globals.bin').read_bytes()
    assert probe[76:80]==original[76:80] and probe[84:94]==original[84:94] and probe[162:206]==original[162:206],'DISPLAY / PUBLIC GLOBALS'
    print('DISPLAY SERVICES PASS: public globals, complete port/map records with owned pointers normalized, locked/unlocked pixel identity')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path);p.add_argument('reference',type=Path);a=p.parse_args();check(a.run,a.reference)
