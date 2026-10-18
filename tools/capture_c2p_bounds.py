"""Capture opening conversion bounds and planes; debugger stops invalidate timings."""
import argparse
import hashlib
import json
import shutil
import os
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--replay",type=Path,help="receipt-verified frozen opening build")
    parser.add_argument("--setup",choices=("a1200-030","a1200-fast"),default="a1200-030")
    parser.add_argument("--frame-boundaries",action="store_true",help="capture every completed game frame, including skipped publications")
    args=parser.parse_args()
    if args.replay:args.replay=args.replay.resolve()
    os.chdir(ROOT)
    assert not (ROOT/"amiga/.run-lock").exists(),"another native run is active"
    out=Path(tempfile.mkdtemp(prefix='c2p-bounds-',dir=ROOT/'tmp'))
    flags='-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO -DPOP2_INTEGRATION_PROBE -DPOP2_TIMING_PROBE -DPOP2_INTEGRATION_NO_RIGHT -DPOP2_OPENING_PROFILE'
    if args.replay:
        receipt=json.loads((args.replay/'PrinceOfPersia2.json').read_text())
        assert set(receipt['flags'].split())==set(flags.split()),'replay flags'
        for ext in ('exe','elf'):
            assert hashlib.sha256((args.replay/('PrinceOfPersia2.'+ext)).read_bytes()).hexdigest()==receipt[ext],'replay identity'
        for ext in ('exe','elf','json'):
            shutil.copyfile(args.replay/('PrinceOfPersia2.'+ext),ROOT/'amiga/out'/('PrinceOfPersia2.'+ext))
        (ROOT/'amiga/obj/flags').write_text('frozen bounds replay; rebuild before ordinary make\n')
    else:
        with (out/'build.log').open('w') as log:
            subprocess.run(['make','-C','amiga','-j4','PROFILE=1','VERIFY=0','EXTRA_DEFINES='+flags],stdout=log,stderr=subprocess.STDOUT,check=True)
    observer=out/'observer.gdb'
    opening=(ROOT/'amiga/aprof_opening.gdb').read_text()
    entry=' if g_integrationStage==3\n'
    assert opening.count(entry)==1, 'C2P / UNIQUE GAMEPLAY ENTRY'
    opening=opening.replace(entry,entry+'  install-c2p-bounds\n')
    template=(ROOT/('amiga/frame_boundaries.gdb' if args.frame_boundaries else 'amiga/c2p_bounds.gdb')).read_text()
    if args.frame_boundaries:
        lines=(ROOT/'src/platform/amiga/CoreRuntime.cpp').read_text().splitlines()
        sites=[i+2 for i,line in enumerate(lines) if
               'if(!NativeDisplay::publish(displayBatchScene))return stop(' in line and
               lines[i+1].strip()=='displayBatchScene=false;']
        assert len(sites)==1,'FRAME / UNIQUE POST-PUBLICATION BOUNDARY'
        assert template.count('@BOUNDARY_LINE@')==1,'FRAME / TEMPLATE BOUNDARY'
        template=template.replace('@BOUNDARY_LINE@',str(sites[0]))
    observer.write_text(template+opening)
    env=dict(os.environ,CORE='1',OPENING_PROFILE='1',CPU_PROFILE='0',APROF_ACTIVE='0',AMIGA_SETUP=args.setup,GDBSCRIPT=str(observer),DEADLINE='1800')
    with (out/'run.log').open('w') as log:
        result=subprocess.run(['bash','amiga/run.sh'],env=env,stdout=log,stderr=subprocess.STDOUT)
    run=ROOT/'amiga'/(ROOT/'amiga/.run-last.txt').read_text().strip()
    (out/'run-dir.txt').write_text(str(run)+'\n')
    print(out,run, 'status',result.returncode,flush=True)
    result.check_returncode()
if __name__=='__main__':main()
