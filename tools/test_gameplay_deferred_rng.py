"""Corruption controls for the owner's narrowly pinned D11 exception."""
from pathlib import Path
import os,shutil,tempfile
from check_gameplay_route import check
from extract_original_data import ROOT

def main():
    source=ROOT/'amiga/.run-rXZcAu';reference=ROOT/'ref/mame/runs/gameplay-level1-complete-1'
    with tempfile.TemporaryDirectory(prefix='deferred-rng-controls-',dir=ROOT/'tmp') as temporary:
        target=Path(temporary)/'run';shutil.copytree(source,target,copy_function=os.link)
        report=target/'route-comparison.json'
        if report.exists():report.unlink() # The checker writes its own report.
        def verify():check(target,reference,'a1200-030',1,deferred_rng=True)
        verify()
        def reject(name,path,alter):
            original=path.read_bytes();path.unlink();path.write_bytes(alter(original))
            try:
                try:verify()
                except RuntimeError:pass
                else:raise AssertionError('D11 accepted corruption: '+name)
            finally:path.write_bytes(original)
            print('D11 rejection:',name)
        def seed(data):
            changed=bytearray(data);changed[-0x18fc]^=1;return bytes(changed)
        def pixel(data):
            changed=bytearray(data);changed[48*640+65]^=1;return bytes(changed)
        reject('different random seed',target/'route-64-a5.bin',seed)
        reject('pixels at an exempt RNG checkpoint',target/'route-64-screen.bin',pixel)
        reject('missing completion',target/'gdb-out.log',lambda data:data.replace(b'LEVEL 1 COMPLETE',b'LEVEL 1 INCOMPLETE'))
        verify()
    print('D11 CONTROLS PASS: only the pinned RNG pair is exempt; evidence remains required')
if __name__=='__main__':main()
