"""Require repeatable original native startup, first-room input and loud failures."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from check_integration_result import check, reference_state
from check_integration_controls import evidence_controls
from display_frames import frames
from extract_original_data import ROOT, pins
from run_reference import run as reference_run


def compress_completed_volume(reference):
    source = reference/'volume.hd'
    target = reference/'volume.hd.gz'
    assert source.is_file() and not target.exists(), 'INTEGRATION / SCRATCH VOLUME OWNERSHIP'
    with source.open('rb') as stream:
        expected = hashlib.file_digest(stream, 'sha256').digest()
    with source.open('rb') as stream, gzip.open(target, 'wb', compresslevel=1) as out:
        shutil.copyfileobj(stream, out)
    with gzip.open(target, 'rb') as stream:
        assert hashlib.file_digest(stream, 'sha256').digest() == expected, 'INTEGRATION / COMPRESSED VOLUME'
    source.unlink()  # Only this gate's completed, verified scratch copy.


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--references', nargs=2, type=Path)
    parser.add_argument('--title-reference', type=Path)
    parser.add_argument('--deadline', type=int, default=3600)
    args = parser.parse_args()
    assert args.deadline > 0
    os.chdir(ROOT)
    logs = Path(tempfile.mkdtemp(prefix='integration-gate-', dir=ROOT/'tmp'))
    for key in list(os.environ):
        if key.startswith(('POP2_', 'CORE_', 'UI_', 'INPUT_', 'DISPLAY_', 'AUDIO_', 'INTEGRATION_')) or key in ('CORE', 'GDBSCRIPT', 'SNAPSHOT_AFTER'):
            os.environ.pop(key)
    if args.references:
        references = [p.resolve() for p in args.references]
    else:
        references = []
        os.environ.update(POP2_AUDIO='route', POP2_DISPLAY_ROUTE='1', POP2_NATURAL_OPENING='1')
        for _ in range(2):
            reference = reference_run()
            reference_state(reference)
            compress_completed_volume(reference)
            references.append(reference)
        os.environ.pop('POP2_AUDIO')
        os.environ.pop('POP2_DISPLAY_ROUTE')
        os.environ.pop('POP2_NATURAL_OPENING')
    assert reference_state(references[0]) == reference_state(references[1]), 'INTEGRATION / ORIGINAL REPEAT'
    if args.title_reference:
        title_reference=args.title_reference.resolve()
    else:
        os.environ.update(POP2_TITLE_STATE='16', POP2_DISPLAY_ROUTE='1', POP2_NATURAL_OPENING='1')
        title_reference=reference_run()
        for key in ('POP2_TITLE_STATE','POP2_DISPLAY_ROUTE','POP2_NATURAL_OPENING'):
            os.environ.pop(key)
        compress_completed_volume(title_reference)
    from check_title_state import check_reference, evidence_controls as title_controls
    check_reference(title_reference,16)
    title_rejections=title_controls(title_reference)
    flags = '-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO -DPOP2_INTEGRATION_PROBE'
    native = {}

    def build(extra=''):
        with (logs/('build-control.log' if extra else 'build.log')).open('w') as out:
            subprocess.run(['make', '-C', 'amiga', 'EXTRA_DEFINES='+flags+extra],
                           stdout=out, stderr=subprocess.STDOUT, check=True)

    def launch(label, right_expected=True):
        env = dict(os.environ, CORE='1', INTEGRATION='1',
                   INTEGRATION_REFERENCE=str(references[0]),
                   INTEGRATION_TITLE_REFERENCE=str(title_reference),
                   INTEGRATION_NO_RIGHT='0' if right_expected else '1',
                   DEADLINE=str(args.deadline), GDBSCRIPT=str(ROOT/'amiga/integration.gdb'))
        with (logs/(label+'.log')).open('w') as out:
            subprocess.run(['/bin/bash', 'amiga/run.sh'], env=env,
                           stdout=out, stderr=subprocess.STDOUT, check=True)
        run = ROOT/'amiga'/(ROOT/'amiga/.run-last.txt').read_text().strip()
        check(run, references[0], title_reference, right_expected=right_expected)
        native[label] = run
        return run

    build()
    first = launch('native')
    repeat = launch('repeat')
    # The state checks above compare curated game words; raw A5 images include pointers.
    for name in ('title-screen.bin', 'first-room-screen.bin', 'first-room-planes.bin', 'death-screen.bin'):
        assert (first/name).read_bytes() == (repeat/name).read_bytes(), 'INTEGRATION / NATIVE REPEAT '+name
    controls = evidence_controls(repeat, references[0], title_reference)
    build(' -DPOP2_INTEGRATION_NO_RIGHT')
    idle = launch('no-right', right_expected=False)
    assert (idle/'first-room-screen.bin').read_bytes() == (first/'first-room-screen.bin').read_bytes(), 'INTEGRATION / CONTROL START'
    assert (idle/'death-screen.bin').read_bytes() != (first/'death-screen.bin').read_bytes(), 'INTEGRATION / INPUT MUST CHANGE OUTCOME'
    build()  # Leave the verified integration build available.
    tracked = subprocess.check_output(['git', 'ls-files'], text=True).splitlines()
    assert not any(p.startswith(('tmp/', 'ref/', 'build/', 'amiga/.run', 'amiga/obj/', 'amiga/out/')) for p in tracked)
    ignored = subprocess.check_output(['git', 'status', '--ignored', '--short'], text=True)
    assert all('!! '+p+'/' in ignored for p in ('tmp', 'ref', 'build'))
    result = {'result': 'INTEGRATION GATE PASS', 'references': [str(p.relative_to(ROOT)) for p in references],
              'title_reference': str(title_reference.relative_to(ROOT)),
              'native': {k: str(p.relative_to(ROOT)) for k, p in native.items()},
              'title_reference_controls': title_rejections, 'evidence_controls': controls, 'input_control': 'same first room; zero Right reads; different completed outcome'}
    (logs/'report.json').write_text(json.dumps(result, indent=2)+'\n')
    print('INTEGRATION GATE PASS:', logs.relative_to(ROOT), flush=True)


if __name__ == '__main__':
    main()
