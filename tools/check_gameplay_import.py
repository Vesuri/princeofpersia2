"""Require a fresh native session to load an unchanged Macintosh-created save."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from check_core_result import restored
from check_display_scenes import decode_copper
from check_gameplay_restart import paired_frame
from check_integration_result import require
from extract_original_data import ROOT, pins


def check_reference(reference):
    reference = reference.resolve()
    from report_reference import report
    report(reference, 'load-import')
    for name, digest in pins(ROOT / 'manifest/gameplay-import-frames.sha256').items():
        require(hashlib.sha256((reference / name).read_bytes()).hexdigest() == digest, 'IMPORT FRAME PIN ' + name)
    original = (reference / 'session.log').read_text()
    require(original.count('GAMEPLAY_REFERENCE_COMPLETE route=load-import loaded_frames=1\n') == 1
            and 'FAIL REFERENCE' not in original, 'IMPORT ORIGINAL COMPLETION')
    require(re.findall(r'^GAMEPLAY_FILE_ENTER (.*)$', original, re.M) == ['load-enter'], 'IMPORT ORIGINAL LOAD WITHOUT SAVE')
    require(re.findall(r'^GAMEPLAY_MENU ordinal=\d+ command=(.*)$', original, re.M) == ['01910004'], 'IMPORT ORIGINAL OPEN COMMAND')
    keys = re.findall(r'^GAMEPLAY_KEYS (.*)$', original, re.M)
    expected = [f'step={step} left={8 if 14 <= step <= 17 else 0} right=0 command={128 if step == 23 else 0}'
                for step in range(24)] + ['step=23 left=0 right=0 command=0']
    require(keys == expected, 'IMPORT ORIGINAL CONSUMED KEYS')
    require(re.findall(r'^GAMEPLAY_FILE_STEP label=load-enter step=(\d+)$', original, re.M) == ['23'], 'IMPORT ORIGINAL LOAD STEP')
    require(original.count('GAMEPLAY_VOLUME_FLUSHED\n') == 1, 'IMPORT ORIGINAL FLUSH')
    require(original.count('GAMEPLAY_LOADED_FRAME result=FFFE\n') == 1, 'IMPORT ORIGINAL LOADED RETURN')
    return original


def check(run, reference, paired):
    for manifest in ('gameplay-save-reference.sha256', 'gameplay-import-reference.sha256'):
        for name, digest in pins(ROOT / 'manifest' / manifest).items():
            require(hashlib.sha256((reference / name).read_bytes()).hexdigest() == digest, 'IMPORT REFERENCE PIN ' + name)
    original = check_reference(paired)
    receipt = json.loads((run / 'build-receipt.json').read_text())
    require({'-DPOP2_VERIFY', '-DPOP2_GAMEPLAY_IMPORT_PROBE', '-DPOP2_GAMEPLAY_SAVE_PROBE',
             '-DPOP2_GAMEPLAY_RESTART_PROBE', '-DPOP2_INTEGRATION_PROBE'}.issubset(receipt['flags'].split()), 'IMPORT BUILD FLAGS')
    for key, name in (('exe', 'dh1/PrinceOfPersia2'), ('elf', 'PrinceOfPersia2.elf')):
        require(hashlib.sha256((run / name).read_bytes()).hexdigest() == receipt[key], 'IMPORT BUILD IDENTITY')
    imported = json.loads((run / 'mac-save-import.json').read_text())
    require(imported['source'] == str(reference.resolve().relative_to(ROOT)), 'IMPORT SOURCE')
    require(set(imported['files']) == {'Level 1', 'Level 1.rsrc', 'Level 1.finfo'}, 'IMPORT FORKS')
    for name, digest in imported['files'].items():
        require(hashlib.sha256((run / 'dh1' / name).read_bytes()).hexdigest() == digest, 'IMPORT UNCHANGED ' + name)
    require((run / 'dh1/Level 1').read_bytes() == (reference / 'saved-Level-1.data').read_bytes(), 'IMPORT ORIGINAL DATA')
    require((run / 'dh1/Level 1.rsrc').read_bytes() == (reference / 'saved-Level-1.rsrc').read_bytes(), 'IMPORT ORIGINAL RESOURCE FORK')
    log = (run / 'gdb-out.log').read_text()
    restored(run, log, 'a1200-030')
    require(log.count('CORE REASON GAMEPLAY / MAC SAVE IMPORT COMPLETE\n') == 1, 'IMPORT COMPLETION')
    require('INTEGRATION AUDIT code=1 input=1\n' in log, 'IMPORT CODE/INPUT AUDIT')
    require(re.findall(r'^INTEGRATION STAGE (\d+) ', log, re.M) == ['1', '2', '3', '5', '4'], 'IMPORT STAGE ORDER')
    require(re.findall(r'^INTEGRATION ALERT id=(\d+)$', log, re.M) == ['7002'], 'IMPORT OPEN WITHOUT SAVE')
    require(re.findall(r'^GAMEPLAY FILE save=(\d+) load=(\d+)$', log, re.M) == [('0', '1')], 'IMPORT LOAD WITHOUT SAVE')
    require('GAMEPLAY INPUT actions=6 ' in log and 'menu=01910004\n' in log, 'IMPORT OPEN INPUT')
    native_keys = re.findall(r'^GAMEPLAY KEYS (.*)$', log, re.M)
    original_keys = re.findall(r'^GAMEPLAY_KEYS (.*)$', original, re.M)
    require(len(original_keys) == 25 and native_keys == original_keys, 'IMPORT CONSUMED KEYS')
    for label, prefix in (('first-room', 'gameplay-first-frame-1'), ('loaded-first', 'gameplay-first-frame-2'), ('loaded', 'gameplay-loaded-frame')):
        paired_frame(run, paired, label, prefix)
        native = (run / (label + '-a5.bin')).read_bytes()
        mac = (paired / (prefix + '-a5.bin')).read_bytes()
        require(native[-0x50d2:-0x5094] == mac[-0x50d2:-0x5094], label + ' IMPORT PLAYER RECORD')
    for label, pattern, prefix in (('loaded-first', 'GAMEPLAY FIRST DISPLAY', 'gameplay-first-frame-2'), ('loaded', 'GAMEPLAY DISPLAY', 'gameplay-loaded-frame')):
        fields = re.findall(r'^' + pattern + r' planes=([0-9a-f]+) parity=(\d+)$', log, re.M)
        require(len(fields) == 1, 'IMPORT COPPER RECORD')
        require(decode_copper((run / (label + '-copper.bin')).read_bytes(), int(fields[0][0], 16), int(fields[0][1])) == (paired / (prefix + '.rgb')).read_bytes(), 'IMPORT COPPER')
    print('MAC SAVE IMPORT PASS: fresh Amiga boot, no native save, original-paired loaded frames and unchanged imported file')


def controls(run, reference, paired):
    import contextlib
    import io
    import shutil
    import tempfile
    check(run, reference, paired)
    rejected = 0
    with tempfile.TemporaryDirectory(prefix='import-native-controls-', dir=ROOT / 'tmp') as directory:
        fixture = Path(directory)
        names = ['gdb-out.log', 'build-receipt.json', 'PrinceOfPersia2.elf',
                 'mac-save-import.json', 'logs/debug.uae', 'dh1/Shell.done',
                 'dh1/PrinceOfPersia2', 'dh1/video-transfer.bin', 'dh1/Level 1',
                 'dh1/Level 1.rsrc', 'dh1/Level 1.finfo',
                 'loaded-first-copper.bin', 'loaded-copper.bin']
        names += [label + '-' + suffix + '.bin' for label in
                  ('first-room', 'loaded-first', 'loaded') for suffix in
                  ('screen', 'context', 'a5', 'planes', 'colors')]
        for name in names:
            target = fixture / name; target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(run / name, target)
        with contextlib.redirect_stdout(io.StringIO()):
            check(fixture, reference, paired)

        def reject(name, data):
            nonlocal rejected
            path = fixture / name; good = path.read_bytes()
            assert good != data, 'IMPORT / CONTROL MUST CHANGE EVIDENCE'
            path.write_bytes(data)
            try:
                with contextlib.redirect_stdout(io.StringIO()):
                    check(fixture, reference, paired)
            except (AssertionError, ValueError, RuntimeError):
                rejected += 1
            else:
                raise AssertionError('IMPORT / CORRUPTION ACCEPTED ' + name)
            finally:
                path.write_bytes(good)

        log = (fixture / 'gdb-out.log').read_bytes()
        for before, after in [
                (b'MAC SAVE IMPORT COMPLETE', b'MAC SAVE IMPORT INCOMPLETE'),
                (b'GAMEPLAY FILE save=0 load=1', b'GAMEPLAY FILE save=1 load=1'),
                (b'GAMEPLAY FILE save=0 load=1', b'GAMEPLAY FILE save=0 load=0'),
                (b'INTEGRATION ALERT id=7002', b'INTEGRATION ALERT id=7001'),
                (b'step=23 left=0 right=0 command=128', b'step=23 left=0 right=0 command=0'),
                (b'INTEGRATION STAGE 5 ', b'INTEGRATION STAGE 6 '),
                (b'INTEGRATION AUDIT code=1 input=1', b'INTEGRATION AUDIT code=0 input=1'),
                (b'CORE RESTORE stage=3 error=0', b'CORE RESTORE stage=3 error=1')]:
            reject('gdb-out.log', log.replace(before, after))
        reject('build-receipt.json', (fixture / 'build-receipt.json').read_bytes().replace(b'-DPOP2_GAMEPLAY_IMPORT_PROBE', b'-DPOP2_UNRELATED_PROBE'))
        for name, offset in [
                ('dh1/Level 1', 0), ('dh1/Level 1.finfo', 4),
                ('dh1/PrinceOfPersia2', 128),
                ('loaded-first-a5.bin', 22266 - 0x18fc),
                ('loaded-a5.bin', 22266 - 0x50d2 + 30),
                ('loaded-context.bin', 0x2180),
                ('loaded-screen.bin', 48 * 640 + 65),
                ('loaded-planes.bin', 0), ('loaded-colors.bin', 10),
                ('loaded-copper.bin', 1)]:
            data = bytearray((fixture / name).read_bytes()); data[offset] ^= 1
            reject(name, data)
    print(f'MAC SAVE IMPORT CONTROLS PASS: {rejected} corruptions rejected')
    return rejected


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('run', type=Path); p.add_argument('reference', type=Path); p.add_argument('paired', type=Path)
    p.add_argument('--controls', action='store_true')
    a = p.parse_args(); (controls if a.controls else check)(a.run, a.reference, a.paired)
