"""Verify original startup crosses InitGraf and stops at the next UI service."""
import re,sys
from pathlib import Path
from check_core_result import restored
from display_frames import frames

def check(run,reference):
    frames(reference)
    log=(run/'gdb-out.log').read_text();restored(run,log)
    assert 'CORE STOP trap=a8fe segment=2 offset=3e92 count=31\n' in log
    assert 'CORE REASON UNIMPLEMENTED / InitFonts\n' in log
    assert 'CORE EXCEPTION vector=0 pc=00000000\n' in log
    original=(reference/'traps.txt').read_text().splitlines()[:31]
    native=re.findall(r'^CODE \d+\+[0-9A-F]+ trap=[0-9A-F]+$',log,re.M)
    assert native==original,'DISPLAY / ORIGINAL STARTUP TRAP PREFIX'
    print('DISPLAY STARTUP PASS: 31 original calls through InitGraf; named InitFonts stop, restored OS')
if __name__=='__main__':check(Path(sys.argv[1]),Path(sys.argv[2]))
