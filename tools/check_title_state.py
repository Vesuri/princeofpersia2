"""Validate a labeled, state-paired original title-rendering fixture."""
import hashlib
import shutil
import tempfile
from pathlib import Path
from extract_original_data import ROOT, pins

def check_reference(reference, frame, *, pinned=True):
    log = (reference/'session.log').read_text()
    assert 'FAIL REFERENCE' not in log and '[LUA ERROR]' not in log, 'TITLE STATE / REFERENCE FAILURE'
    assert log.count(f'TITLE_STATE_INPUT frame={frame-1} old=100 cue=101\n') == 1, 'TITLE STATE / PAIRED INPUT'
    assert log.count(f'TITLE_STATE_COMPLETE frame={frame} cue=114\n') == 1, 'TITLE STATE / ORIGINAL RENDERER COMPLETION'
    assert log.count('REFERENCE_CAPTURED opening-text-1\n') == 1, 'TITLE STATE / OPENING CHECKPOINT'
    data = {suffix: (reference/('title-state.'+suffix)).read_bytes() for suffix in ('indices','clut','rgb','ppm')}
    assert len(data['indices']) == 640*480 and len(data['clut']) == 2056 and len(data['rgb']) == 768, 'TITLE STATE / EXTENTS'
    assert data['ppm'] == b'P6\n640 480\n255\n'+b''.join(data['rgb'][i*3:i*3+3] for i in data['indices']), 'TITLE STATE / RGB AND INDEX AGREEMENT'
    if pinned:
        expected=pins(ROOT/'manifest/integration-title-state-reference.sha256')
        assert set(expected)=={'title-state.'+suffix for suffix in data}, 'TITLE STATE / MANIFEST COVERAGE'
        for name, digest in expected.items():
            assert hashlib.sha256((reference/name).read_bytes()).hexdigest() == digest, 'TITLE STATE / PIN '+name
    print('TITLE STATE REFERENCE PASS: original rendering at paired frame and cue; not a natural-route reference')
    return data


def evidence_controls(reference):
    """A valid fixture must reject incomplete or differently paired evidence."""
    check_reference(reference, 16)
    with tempfile.TemporaryDirectory(prefix='title-state-controls-', dir=ROOT/'tmp') as name:
        scratch = Path(name)
        for filename in ('session.log', 'title-state.indices', 'title-state.clut', 'title-state.rgb', 'title-state.ppm'):
            shutil.copyfile(reference/filename, scratch/filename)
        changes = [
            ('session.log', lambda b: b.replace(b'TITLE_STATE_INPUT frame=15 old=100 cue=101', b'TITLE_STATE_INPUT frame=15 old=100 cue=100')),
            ('session.log', lambda b: b.replace(b'TITLE_STATE_COMPLETE frame=16 cue=114\n', b'')),
            ('session.log', lambda b: b.replace(b'TITLE_STATE_COMPLETE frame=16', b'TITLE_STATE_COMPLETE frame=17')),
            ('title-state.indices', lambda b: bytes([b[0]^1])+b[1:]),
            ('title-state.clut', lambda b: b[:12]+bytes([b[12]^1])+b[13:]),
            ('title-state.rgb', lambda b: bytes([b[0]^1])+b[1:]),
        ]
        for filename, change in changes:
            path = scratch/filename
            good = path.read_bytes()
            bad = change(good)
            assert bad != good, 'TITLE STATE / INACTIVE CONTROL'
            path.write_bytes(bad)
            try:
                check_reference(scratch, 16)
            except (AssertionError, ValueError, RuntimeError):
                pass
            else:
                raise AssertionError('TITLE STATE / ACCEPTED ALTERED EVIDENCE '+filename)
            finally:
                path.write_bytes(good)
        check_reference(scratch, 16)
    print(f'TITLE STATE CONTROLS PASS: {len(changes)} alterations rejected; restored positive passed')
    return len(changes)
