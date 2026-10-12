#!/usr/bin/env python3
"""Acceptance gate against the supplied archive, with failure/preservation controls."""
import os
import struct
import subprocess

from extract_original_data import (ARCHIVE, ROOT, apple_double, install,
                                   scratch_directory, unpack, verify_archive, verify_output)


def rejected(action, label):
    try:
        action()
    except ValueError:
        print('CONTROL PASS:', label)
    else:
        raise AssertionError('control accepted: ' + label)


def main():
    first, _ = unpack(ARCHIVE, scratch_directory())
    first_hashes = verify_output(first)
    # The extractor must ignore the caller's timezone when encoding Mac dates.
    old_tz = os.environ.get('TZ')
    os.environ['TZ'] = 'Pacific/Honolulu'
    try:
        second, _ = unpack(ARCHIVE, scratch_directory())
    finally:
        if old_tz is None:
            os.environ.pop('TZ', None)
        else:
            os.environ['TZ'] = old_tz
    assert first_hashes == verify_output(second)
    assert len(first_hashes) == 96
    print('REPEATABILITY PASS: two independent extractions match all 96 pinned hashes')

    work = scratch_directory()
    bad = work / 'bad.SIT'
    original = bytearray(ARCHIVE.read_bytes())
    original[-1] ^= 1
    bad.write_bytes(original)
    rejected(lambda: verify_archive(bad), 'changed archive rejected')
    malformed = work / 'malformed.appledouble'
    malformed.write_bytes(bytes.fromhex('0005160700020000') + bytes(16) +
                          struct.pack('>HIII', 1, 2, 9999, 4))
    rejected(lambda: apple_double(malformed), 'out-of-bounds AppleDouble rejected')

    destination = work / 'installed'
    install(first, destination)
    before = {p: (destination / p).stat().st_mtime_ns for p in first_hashes}
    install(second, destination)
    assert before == {p: (destination / p).stat().st_mtime_ns for p in first_hashes}
    print('PRESERVATION PASS: identical destination left untouched')
    changed = destination / 'Data/Prince2.opt'
    changed.write_bytes(b'changed-option-control')
    (destination / 'save-control').write_bytes(b'preserve this save')
    rejected(lambda: verify_output(destination), 'changed output and extra file rejected')
    backup = ROOT / '.replaced'
    previous = set(backup.iterdir()) if backup.exists() else set()
    install(second, destination)
    verify_output(destination)
    added = set(backup.iterdir()) - previous
    assert len(added) == 1
    retained = added.pop() / destination.name
    assert (retained / 'Data/Prince2.opt').read_bytes() == b'changed-option-control'
    assert (retained / 'save-control').read_bytes() == b'preserve this save'
    print('PRESERVATION PASS: differing tree and extra save retained under .replaced/')

    paths = [str((destination / p).relative_to(ROOT)) for p in first_hashes]
    ignored = subprocess.check_output(['git', 'check-ignore', '--stdin'],
                                     input='\n'.join(paths)+'\n', text=True, cwd=ROOT).splitlines()
    assert set(ignored) == set(paths)
    tracked = subprocess.check_output(['git', 'ls-files', '-z'], cwd=ROOT).decode().split('\0')
    assert 'Makefile' in tracked  # Positive control: we queried the actual repository index.
    assert not any(p.startswith(('tmp/', '.replaced/')) for p in tracked)
    print('TRACKING PASS: every extracted path ignored; no scratch or replacement file tracked')
    print('EXTRACTION GATE PASS')


if __name__ == '__main__':
    main()
