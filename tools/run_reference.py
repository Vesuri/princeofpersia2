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


def run(name=None, control=None, deadline=None):
    if os.environ.get('POP2_SAVE_RECT_FIXTURE'):
        if os.environ.get('POP2_INTEGRATION')!='1' or os.environ.get('POP2_NULL_DISPOSE')!='1':
            raise ValueError('SAVE RECT / REQUIRES INTEGRATION AND NULL_DISPOSE')
        from save_rect_fixture import verify_fixture
        verify_fixture(Path(os.environ['POP2_SAVE_RECT_FIXTURE']))
    if os.environ.get('POP2_WRAPPER_FIXTURE'):
        fixture=Path(os.environ['POP2_WRAPPER_FIXTURE']).resolve()
        recorded=pins(ROOT/'manifest/wrapper-ccr-inputs.sha256')
        for artifact in ('image.bin','metadata.json','metadata.lua'):
            path=fixture/artifact;key=str(path.relative_to(ROOT))
            if key not in recorded or digest(path)!=recorded[key]:
                raise ValueError('REFERENCE / WRAPPER FIXTURE INPUT PIN '+key)
    if os.environ.get('POP2_GAMEPLAY') and not os.environ.get('POP2_DISPLAY_ROUTE'):
        raise ValueError('REFERENCE / GAMEPLAY REQUIRES POP2_DISPLAY_ROUTE=1')
    if deadline is not None and not 1 <= deadline <= 43200:
        raise ValueError('REFERENCE / DEADLINE RANGE')
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
    if os.environ.get('POP2_GAMEPLAY_IMPORT_FROM'):
        from gameplay_saved_file import install_reference
        install_reference(Path(os.environ['POP2_GAMEPLAY_IMPORT_FROM']),destination)
    if os.environ.get('POP2_HALL_IMPORT_FROM'):
        from gameplay_hall_file import install_reference
        install_reference(Path(os.environ['POP2_HALL_IMPORT_FROM']),destination)
    if os.environ.get('POP2_SCENE_CACHE') == 'generated':
        from aprof_scenes import reference_cache
        reference_cache(destination)
    generate(destination/'metadata.lua')
    subprocess.run(['python3','tools/check_mame_literals.py','--syntax-wrapper',str(destination/'syntax.lua')],cwd=ROOT,check=True)
    env=os.environ.copy()
    env.update(SDL_VIDEODRIVER='dummy',POP2_REFERENCE_RUN=str(destination.relative_to(ROOT)))
    if control:env['POP2_REFERENCE_CONTROL']=control
    args=[str(mame),'maciici','-noreadconfig','-ramsize','8M','-nbc','mdc48',
          '-hard',str(destination/'volume.hd'),'-rompath','ref/mame/roms',
          '-nvram_directory',str(destination/'nvram'),'-cfg_directory',str(destination/'cfg'),
          '-snapshot_directory',str(destination/'snap'),'-state_directory',str(destination/'states'),'-video','none','-sound','none','-window',
          '-nothrottle','-skip_gameinfo','-debug','-debugger','none','-oslog','-nosamples',
          '-autoboot_delay','0','-autoboot_script',str(destination/'syntax.lua'),'-seconds_to_run','3600' if env.get('POP2_GAMEPLAY')=='route' else '600' if env.get('POP2_AUDIO')=='cutscene' or env.get('POP2_NATURAL_OPENING') or env.get('POP2_GAMEPLAY') else '240' if env.get('POP2_AUDIO') else '180']
    pidfile=destination/'mame.pid'
    with (destination/'session.log').open('wb') as output:
        process=subprocess.Popen(args,cwd=ROOT,env=env,stdout=output,stderr=subprocess.STDOUT)
        pidfile.write_text(str(process.pid)+'\n')
        try:
            result=process.wait(timeout=deadline if deadline is not None else 3600 if env.get('POP2_GAMEPLAY')=='route' else 480 if env.get('POP2_AUDIO') or env.get('POP2_GAMEPLAY') else 240)
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
    if env.get('POP2_QUIT_REFERENCE'):
        log=(destination/'session.log').read_text()
        if log.count('QUIT_REFERENCE_COMPLETE Finder\n')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / QUIT INCOMPLETE')
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
    if env.get('POP2_HEAP_PROBE'):
        log=(destination/'session.log').read_text()
        if log.count('HEAP_FIXTURE_COMPLETE movehhi=9 resize=5\n')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / HEAP FIXTURE INCOMPLETE')
    if env.get('POP2_HANDLE_COPY_PROBE'):
        log=(destination/'session.log').read_text()
        if log.count('HANDLE_COPY_FIXTURE_COMPLETE\n')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / HANDLE COPY FIXTURE INCOMPLETE')
    if env.get('POP2_RECTANGLE_PROBE'):
        log=(destination/'session.log').read_text()
        if log.count('RECTANGLE_COMPLETE calls=1220\n')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / RECTANGLE FIXTURE INCOMPLETE')
    if env.get('POP2_EQUAL_RECT_PROBE'):
        log=(destination/'session.log').read_text()
        if log.count('EQUAL_RECT_COMPLETE calls=36\n')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / EQUAL RECT FIXTURE INCOMPLETE')
    if env.get('POP2_FREED_PROBE'):
        log=(destination/'session.log').read_text()
        if log.count('FREED_FIXTURE_COMPLETE\n')!=1 or log.count('FREED_FIXTURE_RESTORED\n')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / FREED FIXTURE INCOMPLETE')
        from check_freed_handles import check as freed_check
        freed_check(log)
    if env.get('POP2_FREED_MEMERROR'):
        from check_freed_handles import memory_check
        memory_check((destination/'session.log').read_text())
    if env.get('POP2_HANDLE_STATE_PROBE'):
        log=(destination/'session.log').read_text()
        if log.count('HANDLE_STATE_FIXTURE_COMPLETE\n')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / HANDLE STATE FIXTURE INCOMPLETE')
    if env.get('POP2_SAVE_RECT_FIXTURE'):
        log=(destination/'session.log').read_text()
        if log.count('SAVE_RECT_COMPLETE cases=672\n')!=1 or log.count('SAVE_RECT_RESTORED\n')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / SAVE RECT INCOMPLETE')
    if env.get('POP2_WRAPPER_FIXTURE'):
        log=(destination/'session.log').read_text()
        if log.count('WRAPPER_FIXTURE_COMPLETE cases=640\n')!=1 or log.count('WRAPPER_FIXTURE_RESTORED\n')!=1 or 'FAIL REFERENCE' in log:
            raise ValueError('REFERENCE / WRAPPER FIXTURE INCOMPLETE')
    if env.get('POP2_NULL_DISPOSE') and not any(env.get(k) for k in ('POP2_SAVE_RECT_FIXTURE','POP2_FREED_PROBE','POP2_HEAP_PROBE','POP2_HANDLE_STATE_PROBE','POP2_HANDLE_COPY_PROBE','POP2_EQUAL_RECT_PROBE','POP2_RECTANGLE_PROBE','POP2_WRAPPER_FIXTURE')):
        log=(destination/'session.log').read_text()
        for marker in ('DISPOSE_FIXTURE stage=1 d0=00000000 memerr=0000',
                       'DISPOSE_FIXTURE stage=2 d0=00000000 memerr=0000 a0=00000000',
                       'DISPOSE_FIXTURE_COMPLETE'):
            if log.count(marker)!=1:raise ValueError('REFERENCE / DISPOSE FIXTURE RESULT')
    if env.get('POP2_AUDIO') or env.get('POP2_ROUTE_AUDIO_TRACE'):
        log=(destination/'session.log').read_text()
        marker='AUDIO_UNLOADED_NOTE_COMPLETE calls=8' if env.get('POP2_AUDIO_UNLOADED_NOTE')=='1' else 'AUDIO_CALLBACK_ORDER_COMPLETE calls=6' if env.get('POP2_AUDIO_CALLBACK_ORDER')=='1' else 'AUDIO_OBSERVER_COMPLETE' if env.get('POP2_ROUTE_AUDIO_TRACE') else 'AUDIO_FIXTURE_COMPLETE calls=27' if env['POP2_AUDIO']=='fixture' else 'AUDIO_CUTSCENE_COMPLETE' if env['POP2_AUDIO']=='cutscene' else 'AUDIO_DEMO_COMPLETE' if env.get('POP2_DEMO_ROUTE') else 'AUDIO_ROUTE_COMPLETE' if env.get('POP2_DISPLAY_ROUTE') else 'AUDIO_OBSERVER_COMPLETE'
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
        endpoint='first_frames=2' if env['POP2_GAMEPLAY']=='restart' else 'loaded_frames=1'
        marker='GAMEPLAY_DISCOVERY_COMPLETE' if env.get('POP2_ROUTE_DISCOVERY')=='1' and env['POP2_GAMEPLAY']=='route' else 'GAMEPLAY_ROUTE_COMPLETE level=1 next=2' if env['POP2_GAMEPLAY']=='route' else 'GAMEPLAY_REFERENCE_COMPLETE route='+env['POP2_GAMEPLAY']+' '+endpoint
        if env.get('POP2_ROUTE_ENDING')=='1':marker='GAMEPLAY_ENDING_COMPLETE'
        if env.get('POP2_ROUTE_EXPIRED_PROBE')=='1':
            marker='GAMEPLAY_EXPIRED_COMPLETE'
            if (log.count('GAMEPLAY_EXPIRED_RETURN result=1\n')!=1 or
                log.count('GAMEPLAY_ROUTE_NIS_ENTER selector=28\n')!=1 or
                log.count('GAMEPLAY_ROUTE_NIS_RETURN selector=28\n')!=1):
                raise ValueError('REFERENCE / EXPIRED STORY RETURN MISSING')
        if env.get('POP2_ROUTE_HALL')=='1':marker='GAMEPLAY_HALL_COMPLETE'
        if env['POP2_GAMEPLAY']=='hall-view':marker='GAMEPLAY_HALL_VIEW_COMPLETE'
        if log.count(marker+'\n')!=1 or 'FAIL REFERENCE' in log:
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
    if env.get('POP2_SCENE_SELECTOR'):
        from aprof_scenes import reference_check
        selector=int(env['POP2_SCENE_SELECTOR'])
        if selector not in (1,6) or env.get('POP2_SCENE_TIMERS')!='1':raise ValueError('SCENE / ORIGINAL SELECTOR')
        reference_check(destination,selector)
        return destination
    from report_reference import report
    report(destination,"quit" if env.get("POP2_QUIT_REFERENCE") else "load-import" if env.get("POP2_GAMEPLAY")=="load-import" else "demo" if env.get("POP2_DEMO_ROUTE") else "integration" if any(env.get(k) for k in ("POP2_INTEGRATION", "POP2_NEWGWORLD", "POP2_TITLE_COPY")) else "opening" if env.get("POP2_AUDIO") else "file-restart" if env.get("POP2_RESTART_FROM") else "ui" if env.get("POP2_UI") else "display" if env.get("POP2_DISPLAY") else "opening")
    return destination

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name')
    parser.add_argument('--control',choices=['bad-guard','bad-audio-guard'])
    parser.add_argument('--deadline', type=int, help='host deadline in seconds, 1–43200; expiration is a failure')
    args=parser.parse_args()
    run(args.name,args.control,args.deadline)
