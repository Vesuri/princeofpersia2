"""Bounded release measurements of pinned assisted cinematic routes (19.8).

Only the first dispatched service within NIS and its natural return stop the measured execution. Timer requests are buffered
in Fast RAM. This observer stops after the original NIS returns; it does not
claim completion of the containing level.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess

from extract_original_data import ROOT, pins
from gameplay_assisted_inputs import metadata

SCENES = {
    'carpet': (1, 'scenes-carpet-full', 'scenes-carpet-reference.sha256'),
    'throne': (6, 'gameplay-assisted-level8-clean-2', 'gameplay-assisted-level8-clean.sha256'),
}
BASE = '-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO'
FLAGS = BASE + ' -DPOP2_INTEGRATION_PROBE -DPOP2_TIMING_PROBE -DPOP2_GAMEPLAY_ROUTE_PROBE -DPOP2_SCENE_PROFILE'


def observer(selector):
    # Same original NIS sites as the paired cutscene observer. Never replace
    # its return result or advance the original to an invented endpoint.
    return r'''
set $selected=0
break pop2CoreStopped
commands
 silent
 printf "CORE REASON %s\n",g_coreReason
 printf "CORE EXCEPTION vector=%lu pc=%08lx\n",g_coreException,g_coreFaultPC
 printf "CORE RESTORE stage=%lu error=%lu vector=%lu heap=%lu zone=%lu dma=%u/%u irq=%u/%u view=%u vbi=%u\n",g_scaffoldStage,g_scaffoldError,g_coreVectorRestored,g_coreHeapOK,g_coreZoneBytes,g_savedDMA,g_actualDMA,g_savedIRQ,g_actualIRQ,g_viewRestored,g_vectorRestored
 printf "INTEGRATION AUDIT code=%lu input=%lu\n",g_coreCodeAudit,g_inputVectorRestored
 continue
end
break pop2ScaffoldRestored
commands
 silent
 if !$selected || !g_sceneFinished
  echo SCENE / INCOMPLETE\n
  detach
  quit 1
 end
 printf "SCENE RECORDED rows=%lu\n",g_sceneCount
 if g_sceneCount
  dump binary memory @RUN@/scene-timers.bin &g_sceneRows[0][0] &g_sceneRows[g_sceneCount][0]
 end
 printf "SCENE CLOCKS rows=%lu\n",g_sceneClockCount
 if g_sceneClockCount
  dump binary memory @RUN@/scene-clocks.bin &g_sceneClocks[0][0] &g_sceneClocks[g_sceneClockCount][0]
 end
 echo SCENE RESTORED\n
 detach
 quit 0
end
break pop2SceneBoundary
commands
 silent
 if g_sceneActive
  if $selected
   echo SCENE / DUPLICATE ENTRY\n
   quit 1
  end
  set $selected=1
 printf "SCENE START selector=@SELECTOR@ fields=%lu ticks=%lu frames=%lu keys=%lu\n",g_scaffoldFields,inputTicks,g_routeFrames,g_routeKeys
 printf "PROFILE A5 %08lx\n",g_coreA5
 set $i=0
 while $i<24
  if g_coreSegments[$i][0]
   printf "PROFILE SEG %u %08lx %08lx\n",$i,g_coreSegments[$i][0],g_coreSegments[$i][1]
  end
  set $i=$i+1
 end
 maint info sections
 dump binary memory @RUN@/jt.bin g_coreA5+32 g_coreA5+32+8*1018
 dump binary memory @RUN@/scene-start-a5.bin g_coreA5-g_coreBelow g_coreA5
 if @PROFILE@
  monitor aprof start
 end
 else
  if !$selected || !g_sceneFinished || g_sceneResult!=0
   echo SCENE / RETURN MISMATCH\n
   quit 1
  end
 if @PROFILE@
  monitor aprof stop @RUN@/scene.prof
 end
 printf "SCENE STOP selector=@SELECTOR@ result=%u fields=%lu ticks=%lu frames=%lu keys=%lu\n",g_sceneResult,g_scaffoldFields,inputTicks,g_routeFrames,g_routeKeys
 set $i=0
 while $i<24
  if g_coreSegments[$i][0]
   printf "PROFILE ENDSEG %u %08lx %08lx\n",$i,g_coreSegments[$i][0],g_coreSegments[$i][1]
  end
  set $i=$i+1
 end
 dump binary memory @RUN@/scene-end-a5.bin g_coreA5-g_coreBelow g_coreA5
 dump binary memory @RUN@/scene-end-screen.bin g_integrationScreen g_integrationScreen+307200
 set $colors=*(unsigned long*)g_integrationColors
 dump binary memory @RUN@/scene-end-colors.bin $colors $colors+2056
 end
 continue
end
continue
'''.replace('@SELECTOR@', str(selector))


def cache(destination):
    """Install only the pinned original-generated cache, preserving staged input."""
    original = ROOT / 'tmp/resource-write-original'
    for name, digest in pins(ROOT / 'manifest/resource-write-output.sha256').items():
        assert hashlib.sha256((original / name).read_bytes()).hexdigest() == digest, 'SCENE / CACHE PIN'
    from check_resource_write import check
    check(original / 'Prince2.opt.rsrc')
    target = destination / 'Data/Prince2.opt.rsrc'
    replaced = target.parent / '.replaced'
    replaced.mkdir(exist_ok=True)
    target.rename(replaced / target.name)
    shutil.copyfile(original / 'Prince2.opt.rsrc', target)
    print('SCENE CACHE generated: pinned original DSLV 6401')


def reference_cache(destination):
    from install_reference_volume import hfs, macbinary
    from extract_original_data import DESTINATION
    original = ROOT / 'tmp/resource-write-original'
    for name, digest in pins(ROOT / 'manifest/resource-write-output.sha256').items():
        assert hashlib.sha256((original/name).read_bytes()).hexdigest() == digest, 'SCENE / ORIGINAL CACHE PIN'
    imported=destination/'scene-cache';imported.mkdir()
    for suffix in ('','.rsrc','.finfo'):
        source=(original if suffix=='.rsrc' else DESTINATION/'Data')/('Prince2.opt'+suffix)
        shutil.copyfile(source,imported/('Prince2.opt'+suffix))
    packed=imported/'Prince2.opt.macbin';packed.write_bytes(macbinary(imported/'Prince2.opt'))
    hfs('hmount', str(destination/'volume.hd'))
    try:
        parent = ':Desktop Folder' if 'Desktop Folder' in hfs('hls','-1a',':').splitlines() else ''
        replaced=destination/'.replaced';replaced.mkdir(exist_ok=True)
        hfs('hcopy','-m',parent+':Prince of Persia 2:Data:Prince2.opt',str(replaced/'Prince2.opt.macbin'))
        hfs('hcopy','-m',str(packed),parent+':Prince of Persia 2:Data:Prince2.opt')
    finally:
        hfs('humount')


def check(run):
    from aprof_report import machine, require
    text = (run / 'gdb-out.log').read_text()
    for marker in ('CORE REASON SCENE PROFILE / COMPLETE\n', 'CORE EXCEPTION vector=0 pc=00000000\n',
                   'INTEGRATION AUDIT code=0 input=1\n', 'SCENE RESTORED\n'):
        require(text.count(marker) == 1, 'SCENE COMPLETION ' + marker.strip())
    m = re.search(r'^CORE RESTORE stage=3 error=0 vector=1 heap=0 zone=3145728 dma=(\d+)/(\d+) irq=(\d+)/(\d+) view=1 vbi=1$', text, re.M)
    require(m and m[1] == m[2] and m[3] == m[4], 'SCENE HARDWARE RESTORE')
    receipt = json.loads((run / 'build-receipt.json').read_text())
    require(set(receipt['flags'].split()) == set((FLAGS + ' -DPOP2_GAMEPLAY_ROUTE_LEVEL=0 -DPOP2_SCENE_SELECTOR='+re.search(r'SCENE START selector=(1|6) ', text)[1]).split()), 'SCENE FLAGS')
    for key, name in [('exe', 'dh1/PrinceOfPersia2'), ('elf', 'PrinceOfPersia2.elf')]:
        require(hashlib.sha256((run/name).read_bytes()).hexdigest() == receipt[key], 'SCENE BUILD HASH')
    config = machine(run)
    require(config['cpu_model'] == '68030' and config['mmu_model'] in (None, '0') and config['fastmem_size'] == '8', 'SCENE MACHINE')
    require(config['cpu_frequency'] == '50000000' and config['ntsc'] == 'false' and config['cpu_cycle_exact'] == 'true', 'SCENE CLOCK')
    start = re.findall(r'^SCENE START selector=(\d+) fields=(\d+) ticks=(\d+) frames=(\d+) keys=(\d+)$', text, re.M)
    end = re.findall(r'^SCENE STOP selector=(\d+) result=0 fields=(\d+) ticks=(\d+) frames=(\d+) keys=(\d+)$', text, re.M)
    require(len(start) == len(end) == 1 and start[0][0] == end[0][0] and start[0][3:] == end[0][3:], 'SCENE BOUNDARIES')
    records = re.findall(r'^SCENE RECORDED rows=(\d+)$', text, re.M)
    data = (run / 'scene-timers.bin').read_bytes() if (run / 'scene-timers.bin').exists() else b''
    require(len(records) == 1 and 0 <= int(records[0]) < 4096 and len(data) == int(records[0]) * 32, 'SCENE TIMER EXTENT')
    require((run/'dh1/Shell.done').read_text().strip() == 'SHELL FAIL', 'SCENE NAMED STOP MUST RETURN DOS FAIL')
    clocks=re.findall(r'^SCENE CLOCKS rows=(\d+)$',text,re.M)
    clock_data=(run/'scene-clocks.bin').read_bytes() if (run/'scene-clocks.bin').exists() else b''
    require(len(clocks)==1 and 0<=int(clocks[0])<2048 and len(clock_data)==32*int(clocks[0]), 'SCENE CLOCK EXTENT')
    if start[0][0]=='6':require(int(records[0])==9 and int(clocks[0])>0,'SCENE THRONE COVERAGE')
    else:require(int(records[0])==0 and int(clocks[0])==0,'SCENE CARPET TIMER CONTRACT')
    print('SCENE RUN PASS: natural NIS return, bounded timer records and normal restoration')


def reference_check(directory, selector):
    from report_reference import analyze
    log=(directory/'session.log').read_text()
    trace,facts=analyze(log)
    assert hashlib.sha256((directory/'opening.ppm').read_bytes()).hexdigest()==pins(ROOT/'manifest/reference-results.sha256')['opening.ppm'], 'SCENE / ORIGINAL STARTUP PIXELS'
    assert log.count('GAMEPLAY_DISCOVERY_COMPLETE\n')==1 and re.search(r'^CUTSCENE_OBSERVER_COMPLETE calls=\d+$',log,re.M), 'SCENE / ORIGINAL COMPLETION'
    records=re.findall(r'^CUTSCENE_RETURN ordinal=(\d+) selector='+str(selector)+r' result=0 copy=(\d+) text=(\d+)$',log,re.M)
    assert len(records)==1 and int(records[0][1])>0, 'SCENE / ORIGINAL TARGET RETURN'
    ordinal=records[0][0]
    timers=re.findall(r'^SCENE_TIMER ordinal='+ordinal+r' selector='+str(selector)+r' ',log,re.M)
    assert (len(timers)==9 if selector==6 else not timers), 'SCENE / ORIGINAL TIMER CONTRACT'
    if selector==1:
        assert re.search(r'^SCENE_TIMER ordinal=1 selector=7 ',log,re.M), 'SCENE / TIMER OBSERVER POSITIVE CONTROL'
    if selector==6:
        assert re.search(r'^SCENE_CLOCK ordinal='+ordinal+r' selector=6 site=321A ',log,re.M), 'SCENE / ORIGINAL DISSOLVE DEADLINES'
    # This scoped gate does not waive or overwrite the startup trap-trace pin.
    # Generated-cache loading shifts polling/callback interleaving; retain the
    # entire unnormalised trace and its hash beside the fixed startup pixels.
    (directory/'scene-startup-traps.txt').write_bytes(trace)
    facts['scene_selector']=selector;facts['startup_traps_sha256']=hashlib.sha256(trace).hexdigest()
    (directory/'scene-reference.json').write_text(json.dumps(facts,indent=2)+'\n')
    print('SCENE ORIGINAL REFERENCE PASS:',directory.name,'selector',selector)


def pair(name, reference):
    from check_gameplay_opening import compare_capture
    from aprof_scene_report import read
    out = ROOT/'tmp/aprof'/name
    state = read(name)
    prefix=str(reference.resolve().relative_to(ROOT))+'/'
    recorded={name:digest for name,digest in pins(ROOT/'manifest/scenes-reference.sha256').items() if name.startswith(prefix)}
    assert recorded, 'SCENE / UNPINNED ORIGINAL'
    for artifact,digest in recorded.items():
        assert hashlib.sha256((ROOT/artifact).read_bytes()).hexdigest()==digest, 'SCENE / ORIGINAL PIN '+artifact
    log = (reference/'session.log').read_text()
    assert log.count('GAMEPLAY_DISCOVERY_COMPLETE\n') == 1 and 'FAIL REFERENCE' not in log, 'SCENE / ORIGINAL COMPLETION'
    records = re.findall(r'^CUTSCENE_RETURN ordinal=(\d+) selector=(\d+) result=0 copy=(\d+) text=(\d+)$', log, re.M)
    records = [r for r in records if int(r[1]) == state['selector']]
    assert len(records) == 1, 'SCENE / ORIGINAL RETURN'
    ordinal, selector, copies, texts = records[0]
    label = f'nis-{ordinal}-{selector}-return-1'
    source = ROOT/(out/'run-dir.txt').read_text().strip()
    compare_capture((out/'scene-end-screen.bin').read_bytes(), (out/'scene-end-colors.bin').read_bytes(),
                    (source/'dh1/video-transfer.bin').read_bytes(), (reference/(label+'.indices')).read_bytes(),
                    (reference/(label+'.rgb')).read_bytes())
    actual = (out/'scene-end-a5.bin').read_bytes()
    expected = (reference/(label+'-a5.bin')).read_bytes()
    for distance, size in ((0x18fc,4),(0x4fd2,2),(0x4fc2,2),(0x245c,2),(0x5208,372)):
        assert actual[-distance:-distance+size] == expected[-distance:-distance+size], 'SCENE / ORIGINAL STATE '+hex(distance)
    original = re.findall(r'^SCENE_TIMER ordinal='+ordinal+r' selector='+selector+r' timer=(\d+) delay=(\d+) ticks=(\d+)$',log,re.M)
    native = list(struct.iter_unpack('>8I',(out/'scene-timers.bin').read_bytes())) if (out/'scene-timers.bin').exists() else []
    assert [(int(r[0]),int(r[1])) for r in original] == [(r[2],r[3]) for r in native], 'SCENE / ORIGINAL TIMER REQUESTS'
    from aprof_scene_report import report
    measured=report(name)
    assert (measured['nis_copies'],measured['nis_texts'])==(int(copies),int(texts)), 'SCENE / ORIGINAL COPY AND TEXT WORKLOAD'
    original_clocks=re.findall(r'^SCENE_CLOCK ordinal='+ordinal+r' selector='+selector+r' site=([0-9A-F]+) ticks=(\d+) duration=(\d+) progress=(\d+) target=(\d+) delay=(\d+)$',log,re.M)
    if state['selector']==6:
        signature=[[int(r[0],16),int(r[2]),int(r[3])] for r in original_clocks]
        assert signature==state['clock_signature'], 'SCENE / ORIGINAL DISSOLVE WORKLOAD'
        draws=[]
        for site,tick,duration,progress,target,delay in original_clocks:
            tick,duration,progress,target=map(int,(tick,duration,progress,target))
            if site=='3152':draws.append(dict(duration=duration,start=tick,late=[]))
            elif site=='321A':
                assert draws and target==draws[-1]['start']+progress, 'SCENE / ORIGINAL DEADLINE'
                draws[-1]['late'].append(tick-target)
        summary=[dict(duration=d['duration'],quanta=len(d['late']),late_quanta=sum(v>0 for v in d['late']),max_late_ticks=max(d['late'])) for d in draws]
    else:
        summary=[]
        animations=re.findall(r'^SCENE_ANIMATION ordinal='+ordinal+r' selector=1 frames=(\d+) result=0$',log,re.M)
        assert len(animations)==3 and list(map(int,animations))==[118,80,84], 'SCENE / ORIGINAL ANIMATION COMPLETION'
        assert measured['animation_returns']==len(animations) and measured['animation_frames']==sum(map(int,animations)), 'SCENE / ORIGINAL ANIMATION WORKLOAD'
    (out/'original-pair.json').write_text(json.dumps(dict(reference=reference.name,selector=int(selector),timer_requests=len(native),draws=summary),indent=2)+'\n')
    print(f'SCENE ORIGINAL PAIR PASS: selector {selector}, visible return, actor/RNG state, {len(native)} timer requests; original copy/text={copies}/{texts}')


def run(name, scene, state, cycles):
    assert re.fullmatch('[a-z0-9][a-z0-9_-]*', name), 'SCENE / NAME'
    selector, refname, manifestname = SCENES[scene]
    reference, manifest = ROOT/'ref/mame/runs'/refname, ROOT/'manifest'/manifestname
    metadata(reference, manifest)
    out = ROOT/'tmp/aprof'/name
    out.mkdir()  # Never overwrite a measurement.
    env = os.environ.copy()
    subprocess.run(['python3', 'tools/gameplay_route_inputs.py', '--level', '0', '--reference', str(reference), '--manifest', str(manifest)], cwd=ROOT, check=True)
    with (out/'build.log').open('w') as log:
        subprocess.run(['make', '-C', 'amiga', '-j4', 'PROFILE=1', 'VERIFY=0', 'GAMEPLAY_LEVEL=0', 'GAMEPLAY_REFERENCE='+str(reference), 'GAMEPLAY_MANIFEST='+str(manifest), 'EXTRA_DEFINES='+FLAGS+' -DPOP2_SCENE_SELECTOR='+str(selector)], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
    script=out/'observer.gdb';script.write_text(observer(selector))
    env.update(SCENE_SELECTOR=str(selector), AMIGA_SETUP='a1200-030', CORE='1', CPU_PROFILE=str(cycles), PROFILE_NO_MMU='1', SCENE_PROFILE='1', SCENE_CACHE=state, GDBSCRIPT=str(script), DEADLINE='3600')
    with (out/'run.log').open('w') as log:
        result=subprocess.run(['bash','amiga/run.sh'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT)
    source=ROOT/'amiga'/(ROOT/'amiga/.run-last.txt').read_text().strip()
    (out/'run-dir.txt').write_text(str(source.relative_to(ROOT))+'\n')
    (out/'config.txt').write_text(f'setup=a1200-030 cycles={cycles} no_mmu=1 scene={scene} cache={state}\n')
    for path in source.iterdir():
        if path.name in ('gdb-out.log','PrinceOfPersia2.elf','build-receipt.json','jt.bin') or path.name.startswith('scene-') or path.suffix=='.prof':
            shutil.copyfile(path,out/path.name)
    assert result.returncode==0, 'SCENE / RUN FAILED: '+str(out/'run.log')
    check(source)
    print('SCENE PROFILE COMPLETE',name)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='cmd',required=True)
    r=sub.add_parser('run');r.add_argument('name');r.add_argument('scene',choices=SCENES);r.add_argument('--cache',choices=['clean','generated'],default='clean');r.add_argument('--cycles',type=int,choices=[0,1],default=1)
    c=sub.add_parser('check');c.add_argument('run',type=Path)
    q=sub.add_parser('pair');q.add_argument('name');q.add_argument('reference',type=Path)
    a=p.parse_args()
    if a.cmd=='run':run(a.name,a.scene,a.cache,a.cycles)
    elif a.cmd=='pair':pair(a.name,a.reference)
    else:check(a.run)
