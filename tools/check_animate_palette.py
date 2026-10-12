"""Compare every captured original palette animation against the shared helper."""
from pathlib import Path
import re,subprocess,sys
from extract_original_data import ROOT,scratch_directory
p=Path(sys.argv[1]);log=(p/'session.log').read_text()
assert 'DISPLAY_TRACE_COMPLETE calls=' in log and 'FAIL REFERENCE' not in log
out=scratch_directory()/'animate-check'
subprocess.run(['c++','-std=c++17','-O2','-fsanitize=address,undefined','-Isrc','tools/animate_palette_check.cpp','-o',str(out)],cwd=ROOT,check=True)
count=0
for ident in re.findall(r'DISPLAY_CALL id=(\d+) .*trap=AA9A ',log):
    files=[p/f'display-{ident}-{label}.bin' for label in ('before-palette','before-screen-colors','before-animation','before-stack','after-palette','after-screen-colors')]
    active=int.from_bytes(files[0].read_bytes()[42:44],'big')==0x800c
    subprocess.run([str(out),*map(str,files),str(int(active))],cwd=ROOT,check=True);count+=1
assert count>1
print(f'PALETTE ANIMATION PASS: {count} complete original palette/CLUT transitions')
