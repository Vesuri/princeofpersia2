"""Ask the original game to save each assisted level entry for profiling."""
import argparse
from pathlib import Path
from replay_gameplay_route import status, wait
from gameplay_assist import assist


def command(run, body):
    sequence=int(status(run)['sequence'])+1
    temporary=run/'route-command.tmp'
    temporary.write_text(f'{sequence} {body}\n');temporary.replace(run/'route-command.txt')
    return wait(run,sequence,180)


def create(run):
    assert wait(run,0,600)['reason']=='first-frame'
    for level in range(1,15):
        if level>1:
            assist(run,'level',level)
            entered=command(run,'1 none')
            assert entered['reason']=='next-level' and int(entered['level'])==level
        saved=command(run,f'profile-save {level}')
        assert saved['reason']=='profile-saved' and int(saved['level'])==level
        print(f'Original profiling save complete: level {level}',flush=True)
    assert command(run,'finish')['reason']=='discovery-finished'


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path)
    create(p.parse_args().run)
