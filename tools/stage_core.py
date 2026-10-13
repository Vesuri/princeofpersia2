"""Stage only verified local core inputs into a fresh private emulator volume."""
import os,shutil,sys
from pathlib import Path
from extract_original_data import ROOT,DESTINATION,verify_output
verified=verify_output(DESTINATION)
# Verify generated plans before staging; native digest checks are opt-in.
from core_inputs import verify as verify_core
verify_core()
sources=[(DESTINATION/name,name) for name in verified]
sources.extend((ROOT/'build/core'/name,name) for name in ('relocations.bin','file-catalog.bin'))
for source,name in sources:
    target=Path(sys.argv[1])/name
    if target.exists():raise RuntimeError('CORE / PRIVATE INPUT ALREADY EXISTS')
    target.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(source,target)

from build_receipt import verify as verify_receipt
if '-DPOP2_DISPLAY' in verify_receipt()['flags'].split():
    import hashlib
    from extract_original_data import pins
    video=ROOT/'build/display/video-transfer.bin'
    if hashlib.sha256(video.read_bytes()).hexdigest()!=pins(ROOT/'manifest/display-inputs.sha256')['video-transfer.bin']:
        raise RuntimeError('DISPLAY / VIDEO TRANSFER PIN')
    shutil.copyfile(video,Path(sys.argv[1])/video.name)
    if '-DPOP2_DISPLAY_FRAMES_PROBE' in verify_receipt()['flags'].split():
        frames=ROOT/'build/display/display-frames.bin'
        if hashlib.sha256(frames.read_bytes()).hexdigest()!=pins(ROOT/'manifest/display-inputs.sha256')['display-frames.bin']:
            raise RuntimeError('DISPLAY / FRAME FIXTURE PIN')
        shutil.copyfile(frames,Path(sys.argv[1])/frames.name)

if '-DPOP2_UI' in verify_receipt()['flags'].split():
    from ui_inputs import generate
    generate(True)
    shutil.copyfile(ROOT/'build/ui/ui-seams.bin',Path(sys.argv[1])/'ui-seams.bin')

if '-DPOP2_AUDIO' in verify_receipt()['flags'].split():
    from audio_inputs import generate
    generate(True)
    shutil.copyfile(ROOT/'build/audio/audio-pitch.bin',Path(sys.argv[1])/'audio-pitch.bin')

if '-DPOP2_AUDIO_PROBE' in verify_receipt()['flags'].split():
    from audio_replay import verify
    verify()
    shutil.copyfile(ROOT/'build/audio/audio-replay.bin',Path(sys.argv[1])/'audio-replay.bin')
    shutil.copyfile(ROOT/'build/audio/audio-replay.json',Path(sys.argv[1])/'audio-replay.json')

control=os.environ.get('CORE_CONTROL')
if control:
    names={'bad-application':'Prince of Persia 2.rsrc','bad-plan':'relocations.bin','bad-transfer':'video-transfer.bin','bad-frames':'display-frames.bin','bad-ui-plan':'ui-seams.bin','bad-audio-pitch':'audio-pitch.bin','bad-audio-replay':'audio-replay.bin'}
    if control not in names:raise RuntimeError('CORE / UNKNOWN INPUT CONTROL')
    target=Path(sys.argv[1])/names[control]
    data=bytearray(target.read_bytes());data[0]^=1;target.write_bytes(data)

if os.environ.get('CORE_FILE_REPLAY')=='1':
    from file_replay import verify
    verify()
    import hashlib,json
    replay=ROOT/'build/core/file-replay.bin'
    metadata=ROOT/'build/core/file-replay.json'
    expected=json.loads(metadata.read_text())['sha256']
    if hashlib.sha256(replay.read_bytes()).hexdigest()!=expected:raise RuntimeError('REPLAY / INPUT HASH')
    for source in (replay,metadata):shutil.copyfile(source,Path(sys.argv[1])/source.name)

restart=os.environ.get('CORE_RESTART_FROM')
if restart:
    import hashlib,json
    previous=Path(restart)
    if not previous.is_absolute():previous=ROOT/previous
    previous=previous.resolve()
    manifest=json.loads((previous/'native-persistence.json').read_text())
    for name,digest in manifest.items():
        if name not in {f'Data/{n}{s}' for n in ('Prince2.opt','Prince.hof','Fixture Save') for s in ('','.rsrc','.finfo')}:
            raise RuntimeError('RESTART / UNEXPECTED STATE FILE')
        source=previous/'dh1'/name
        if hashlib.sha256(source.read_bytes()).hexdigest()!=digest:raise RuntimeError('RESTART / STATE HASH')
        target=Path(sys.argv[1])/name
        if target.exists():
            replaced=target.parent/'.replaced';replaced.mkdir(exist_ok=True)
            target.rename(replaced/target.name)
        shutil.copyfile(source,target)

    (Path(sys.argv[1])/'restart-state.json').write_text(json.dumps(manifest,indent=2)+'\n')


import_reference=os.environ.get('GAMEPLAY_IMPORT_REFERENCE')
if import_reference:
    if not {'-DPOP2_VERIFY','-DPOP2_GAMEPLAY_IMPORT_PROBE'}.issubset(verify_receipt()['flags'].split()):
        raise RuntimeError('GAMEPLAY IMPORT / DIAGNOSTIC BUILD REQUIRED')
    from gameplay_saved_file import install_native
    install_native(Path(import_reference), Path(sys.argv[1]))
