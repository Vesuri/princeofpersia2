#!/usr/bin/env python3
"""Run one isolated headless Macintosh reference and require positive evidence."""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import time
import uuid

from extract_original_data import ROOT, pins
from prepare_reference import prepare
from reference_metadata import generate


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def run(name=None, control=None):
    mame=Path(shutil.which(os.environ.get('MAME','mame')) or '')
    if not mame.is_file():
        raise ValueError('REFERENCE / MAME NOT FOUND')
    expected=pins(ROOT/'manifest/reference-tools.sha256')['mame']
    if digest(mame)!=expected:
        raise ValueError('REFERENCE / MAME HASH MISMATCH')
    name=name or time.strftime('%Y%m%d-%H%M%S-')+uuid.uuid4().hex[:8]
    if not name or any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_' for c in name):
        raise ValueError('REFERENCE / INVALID RUN NAME')
    destination=ROOT/'ref/mame/runs'/name
    prepare(destination,'755')
    if os.environ.get('POP2_RESTART_FROM'):
        from reference_saved_files import install_saved
        install_saved(Path(os.environ['POP2_RESTART_FROM']),destination)
    generate(destination/'metadata.lua')
    subprocess.run(['python3','tools/check_mame_literals.py','--syntax-wrapper',str(destination/'syntax.lua')],cwd=ROOT,check=True)
    env=os.environ.copy()
    env.update(SDL_VIDEODRIVER='dummy',POP2_REFERENCE_RUN=str(destination.relative_to(ROOT)))
    if control:env['POP2_REFERENCE_CONTROL']=control
    args=[str(mame),'maciici','-noreadconfig','-ramsize','8M','-nbc','mdc48',
          '-hard',str(destination/'volume.hd'),'-rompath','ref/mame/roms',
          '-nvram_directory',str(destination/'nvram'),'-cfg_directory',str(destination/'cfg'),
          '-snapshot_directory',str(destination/'snap'),'-video','none','-sound','none','-window',
          '-nothrottle','-skip_gameinfo','-debug','-debugger','none','-oslog','-nosamples',
          '-autoboot_delay','0','-autoboot_script',str(destination/'syntax.lua'),'-seconds_to_run','600' if env.get('POP2_AUDIO')=='cutscene' or env.get('POP2_NATURAL_OPENING') or env.get('POP2_GAMEPLAY') else '240' if env.get('POP2_AUDIO') else '180']
    pidfile=destination/'mame.pid'
    with (destination/'session.log').open('wb') as output:
        process=subprocess.Popen(args,cwd=ROOT,env=env,stdout=output,stderr=subprocess.STDOUT)
        pidfile.write_text(str(process.pid)+'\n')
        try:
            result=process.wait(timeout=480 if env.get('POP2_AUDIO') or env.get('POP2_GAMEPLAY') else 240)
        except BaseException:
            # Use the shared identity-checked helper; never kill by process name.
            subprocess.run(['/bin/bash','-c','. amiga/env.sh; . "$FSUAE_COMMON"; fsuae_kill_recorded "$1" mame','reference',str(pidfile)],cwd=ROOT,check=True)
            process.wait()
            raise
    if result:
        raise ValueError('REFERENCE / MAME EXIT '+str(result))
    if control:
        log=(destination/'session.log').read_text()
        if ('FAIL REFERENCE' not in log or 'PROBE BYTES' not in log) or 'REFERENCE_CAPTURED' in log:
            raise ValueError('REFERENCE / BYTE-GUARD CONTROL DID NOT REJECT')
        print('REFERENCE BYTE-GUARD CONTROL PASS:',destination.relative_to(ROOT))
        return destination
    if env.get("POP2_FILE_ROUTINES"):
        log=(destination/"session.log").read_text()
        markers=("FILE_ROUTINE_COMPLETE HallOfFame restart-read", "FILE_ROUTINE_COMPLETE ResumeGame restart-read") if env.get("POP2_RESTART_FROM") else (
            "FILE_ROUTINE_COMPLETE HallOfFame write-readback", "FILE_ROUTINE_COMPLETE SaveGame write", "FILE_ROUTINE_COMPLETE ResumeGame readback")
        if any(log.count(marker)!=1 for marker in markers+("FILE_ROUTINE_COMPLETE volume-flushed",)):
            raise ValueError("REFERENCE / FILE ROUTINE COMPLETION MISSING")
    if env.get('POP2_FIXED_PROBE'):
        log=(destination/'session.log').read_text()
        if log.count('FIXED_FIXTURE_COMPLETE calls=80\n')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / FIXED FIXTURE INCOMPLETE')
    if env.get('POP2_NULL_DISPOSE'):
        log=(destination/'session.log').read_text()
        for marker in ('DISPOSE_FIXTURE stage=1 d0=00000000 memerr=0000',
                       'DISPOSE_FIXTURE stage=2 d0=00000000 memerr=0000 a0=00000000',
                       'DISPOSE_FIXTURE_COMPLETE'):
            if log.count(marker)!=1:raise ValueError('REFERENCE / DISPOSE FIXTURE RESULT')
    if env.get('POP2_AUDIO'):
        log=(destination/'session.log').read_text()
        marker='AUDIO_FIXTURE_COMPLETE calls=27' if env['POP2_AUDIO']=='fixture' else 'AUDIO_CUTSCENE_COMPLETE' if env['POP2_AUDIO']=='cutscene' else 'AUDIO_DEMO_COMPLETE' if env.get('POP2_DEMO_ROUTE') else 'AUDIO_ROUTE_COMPLETE' if env.get('POP2_DISPLAY_ROUTE') else 'AUDIO_OBSERVER_COMPLETE'
        if log.count(marker+'\n')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / AUDIO COMPLETION MISSING')
        from driver_image import decrypt, expand
        from extract_original_data import DESTINATION
        from amigaxdev.mac.resource_fork import read_resource_fork
        driver=next(r.body for r in read_resource_fork(DESTINATION/'Prince of Persia 2.rsrc') if r.kind==b'MDRV' and r.rid==11)
        if (destination/'audio-driver.bin').read_bytes()!=expand(decrypt(driver)):
            raise ValueError('REFERENCE / AUDIO DRIVER IDENTITY')
    if env.get('POP2_GAMEPLAY'):
        log=(destination/'session.log').read_text()
        if log.count('GAMEPLAY_REFERENCE_COMPLETE route=restart first_frames=2\n')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / GAMEPLAY COMPLETION MISSING')
    if env.get('POP2_TITLE_STATE'):
        from check_title_state import check_reference
        check_reference(destination, int(env['POP2_TITLE_STATE']), pinned=False)
        from report_reference import report
        report(destination, 'opening')
        return destination
    if env.get('POP2_DISPLAY_ROUTE') and env.get('POP2_AUDIO')!='cutscene':
        log=(destination/'session.log').read_text()
        for label in ('title','first-room'):
            if log.count('DISPLAY_ROUTE_CAPTURED '+label+'\n')!=1:
                raise ValueError('REFERENCE / DISPLAY ROUTE INCOMPLETE '+label)
            if (destination/(label+'.indices')).stat().st_size!=640*480 or (destination/(label+'.rgb')).stat().st_size!=768:
                raise ValueError('REFERENCE / DISPLAY ROUTE EXTENT '+label)
    if env.get('POP2_UI_ROUTINES'):
        log=(destination/'session.log').read_text()
        for marker in ('UI_ROUTINE_COMPLETE copy-accepted flags=', 'UI_ROUTINE_COMPLETE packing-declined result=0','UI_ROUTINE_COMPLETE packing-game-declined result=0','UI_ROUTINE_COMPLETE about','UI_ROUTINE_COMPLETE alert-401-yes result=1','UI_ROUTINE_COMPLETE alert-404-yes result=1','UI_ROUTINE_COMPLETE alert-405-yes result=1','UI_ROUTINE_COMPLETE alert-405-no result=2','UI_ROUTINE_COMPLETE alert-406-yes result=1','UI_ROUTINE_COMPLETE alert-406-no result=2'):
            if log.count(marker)!=1:raise ValueError('REFERENCE / UI ROUTINE INCOMPLETE')
    if env.get('POP2_TIMING_TM'):
        log=(destination/'session.log').read_text()
        if log.count('TIMING_TM_COMPLETE cases=7')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / TIME MANAGER INCOMPLETE')
    if env.get('POP2_TIMING_VBL'):
        log=(destination/'session.log').read_text()
        if log.count('TIMING_VBL_COMPLETE removal=1')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / TIMING VBL INCOMPLETE')
    if env.get('POP2_INPUT_PROBE'):
        import re
        log=(destination/'session.log').read_text()
        if len(re.findall(r'^INPUT_FIXTURE_COMPLETE samples=\d+ events=\d+$',log,re.M))!=1 or 'REF ERROR' in log:
            raise ValueError('REFERENCE / INPUT FIXTURE INCOMPLETE')
    if env.get('POP2_VIDEO_TRANSFER'):
        from video_transfer import measure
        measure(destination)
    from report_reference import report
    report(destination,"demo" if env.get("POP2_DEMO_ROUTE") else "integration" if any(env.get(k) for k in ("POP2_INTEGRATION", "POP2_NEWGWORLD", "POP2_TITLE_COPY")) else "opening" if env.get("POP2_AUDIO") else "file-restart" if env.get("POP2_RESTART_FROM") else "ui" if env.get("POP2_UI") else "display" if env.get("POP2_DISPLAY") else "opening")
    return destination

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name')
    parser.add_argument('--control',choices=['bad-guard','bad-audio-guard'])
    args=parser.parse_args()
    run(args.name,args.control)
