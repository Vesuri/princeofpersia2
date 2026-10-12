"""Compare complete buffers for every captured original CopyBits transition."""
from pathlib import Path
import re,subprocess,sys
from extract_original_data import ROOT,scratch_directory

def check(reference):
    log=(reference/'session.log').read_text()
    assert 'DISPLAY_TRACE_COMPLETE calls=' in log and 'FAIL REFERENCE' not in log
    out=scratch_directory()/'copybits-check'
    subprocess.run(['c++','-std=c++17','-O2','-fsanitize=address,undefined','-Isrc','tools/copybits_check.cpp','-o',str(out)],cwd=ROOT,check=True)
    calls=re.findall(r'DISPLAY_CALL id=(\d+) .*trap=A8EC ',log);assert len(calls)==5
    for ident in calls:
        labels=('before-source-pixels','before-destination-pixels','before-source','before-destination','before-from','before-to','before-port','before-vis','before-clip','after-destination-pixels','before-stack')
        files=[reference/f'display-{ident}-{label}.bin' for label in labels]
        subprocess.run([str(out),*map(str,files)],cwd=ROOT,check=True)
    print('COPYBITS PASS: all five original calls; complete destination buffers including unchanged pixels')
if __name__=='__main__':check(Path(sys.argv[1]))
