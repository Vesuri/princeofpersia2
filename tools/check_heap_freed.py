"""Replay the measured freed-palette alias during the original expired story."""
import argparse
from pathlib import Path
import re
import tempfile
from check_handle_state_contract import check as original_contract
from check_gameplay_route import check as route
from extract_original_data import ROOT


def observer(path):
    source=(ROOT/'amiga/gameplay_route.gdb').read_text()
    marker=' printf "ROUTE RESERVE'
    assert source.count(marker)==1
    source=source.replace(marker,' printf "HEAP FREED unlocks=%lu\\n",g_routeFreedUnlocks\n'+marker,1)
    path.write_text(source)


def check(run):
    original_contract(ROOT/'ref/mame/runs/gameplay-assisted-handles-1',live=True)
    route(run,ROOT/'ref/mame/runs/gameplay-story20-expired-3','a1200-030',level=0,
          manifest=ROOT/'manifest/gameplay-story20-reference.sha256')
    rows=re.findall(r'^HEAP FREED unlocks=(\d+)$',(run/'gdb-out.log').read_text(),re.M)
    assert len(rows)==1 and int(rows[0])>0,'freed-handle path not reached'
    print('HEAP FREED PASS: measured alias reached; original-paired expired-story route completes and restores heap')


def controls(run):
    run=run.resolve()
    text=(run/'gdb-out.log').read_text()
    marker=re.search(r'^HEAP FREED unlocks=\d+$',text,re.M).group()
    with tempfile.TemporaryDirectory(dir=ROOT/'tmp',prefix='heap-freed-controls-') as directory:
        target=Path(directory)
        for source in run.iterdir():
            if source.name!='gdb-out.log':
                (target/source.name).symlink_to(source,target_is_directory=source.is_dir())
        for old,new in ((marker,'HEAP FREED unlocks=0'),
                        (marker,'HEAP FREED MISSING'),
                        ('EXPIRED STORY COMPLETE','EXPIRED STORY INCOMPLETE')):
            changed=text.replace(old,new,1);assert changed!=text
            (target/'gdb-out.log').write_text(changed)
            try:check(target)
            except (AssertionError,RuntimeError):pass
            else:raise AssertionError('accepted altered freed-handle evidence')
    print('HEAP FREED CONTROLS PASS: zero hits, missing hits and missing completion rejected')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path,nargs='?');p.add_argument('--observer',type=Path)
    p.add_argument('--controls',action='store_true');a=p.parse_args()
    if a.observer:observer(a.observer)
    else:
        assert a.run;check(a.run)
        if a.controls:controls(a.run)
