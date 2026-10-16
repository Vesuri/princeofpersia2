"""Capture the original cinematic workload with explicit, logged assistance.

Carpet repeats the pinned D6-assisted route through level six. Throne repeats
its pinned approach through the cutscene, then stops at the next completed
frame boundary. The latter does not claim level-eight exit coverage.
"""
import argparse
import os
from pathlib import Path
import subprocess
import sys
from gameplay_assist import assist
from gameplay_assisted_inputs import metadata
from replay_gameplay_route import compare, status, wait
from extract_original_data import ROOT


def capture(name, scene, cache):
    assert name and all(c in 'abcdefghijklmnopqrstuvwxyz0123456789-_' for c in name)
    refname, pin = ('scenes-carpet-full','scenes-carpet-reference.sha256') if scene=='carpet' else ('gameplay-assisted-level8-clean-2','gameplay-assisted-level8-clean.sha256')
    reference=ROOT/'ref/mame/runs'/refname
    metadata(reference,ROOT/'manifest'/pin)
    destination=ROOT/'ref/mame/runs'/name
    assert not destination.exists(), 'SCENE REFERENCE / NAME EXISTS'
    env={k:v for k,v in os.environ.items() if not k.startswith('POP2_')}
    env.update(POP2_DISPLAY_ROUTE='1',POP2_GAMEPLAY='route',POP2_ROUTE_DISCOVERY='1',POP2_ROUTE_ASSIST='1',POP2_CUTSCENE_OBSERVER='1',POP2_SCENE_TIMERS='1')
    env['POP2_SCENE_SELECTOR']='1' if scene=='carpet' else '6'
    if cache=='generated':env['POP2_SCENE_CACHE']='generated'
    with (ROOT/'tmp'/f'{name}.log').open('w') as log:
        process=subprocess.Popen([sys.executable,'tools/run_reference.py','--name',name,'--deadline','3600'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT)
        try:
            wait(destination,0,600);compare(destination,reference,0)
            lines=(reference/'route-actions.txt').read_text().splitlines()
            if scene=='throne':lines=lines[:12]+['13 finish']
            for text in lines:
                fields=text.split();sequence=int(fields[0])
                assert sequence==int(status(destination)['sequence'])+1,'SCENE REFERENCE / COMMAND ORDER'
                if fields[1]=='assist':assist(destination,fields[2],int(fields[3]))
                else:
                    temp=destination/'route-command.tmp';temp.write_text(text+'\n');temp.replace(destination/'route-command.txt');wait(destination,sequence,600)
                if scene=='carpet' or sequence<12:compare(destination,reference,sequence)
            assert status(destination)['reason']=='discovery-finished','SCENE REFERENCE / COMPLETION'
            assert process.wait(timeout=60)==0,'SCENE REFERENCE / ORIGINAL GATE'
        finally:
            if process.poll() is None:
                pidfile=destination/'mame.pid'
                if pidfile.exists():
                    subprocess.run(['/bin/bash','-c','. amiga/env.sh; . "$FSUAE_COMMON"; fsuae_kill_recorded "$1" mame','scene-reference',str(pidfile)],cwd=ROOT,check=True)
                process.wait(timeout=60)
    print('SCENE REFERENCE COMPLETE',scene,cache,name)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('name');p.add_argument('scene',choices=['carpet','throne']);p.add_argument('--cache',choices=['clean','generated'],default='clean');a=p.parse_args();capture(a.name,a.scene,a.cache)
