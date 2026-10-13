"""Pair the authored effect-replacement callback fixture on Macintosh and A1200-030."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct


def original_rows(log):
    if log.count('AUDIO_CALLBACK_ORDER_COMPLETE calls=6\n') != 1 or 'FAIL REFERENCE' in log:
        raise ValueError('AUDIO ORDER / ORIGINAL COMPLETION')
    rows = [tuple(int(v, 16 if i == 3 else 10) for i, v in enumerate(row))
            for row in re.findall(r'^AUDIO_CALLBACK_ORDER index=(\d+) id=(\d+) callbacks=(\d+) result=([0-9A-F]{8})$', log, re.M)]
    expected = [(1, 5, 0, 0), (2, 5, 0, 0), (3, 7, 1, 0),
                (4, 7, 1, 0), (5, 7, 1, 1), (6, 7, 1, 0)]
    if rows != expected:
        raise ValueError('AUDIO ORDER / ORIGINAL CALLBACK AND QUERY RESULTS')
    return rows


def check(run, reference):
    from check_core_result import restored
    from extract_original_data import ROOT, pins
    original = original_rows((reference / 'session.log').read_text())
    driver = (reference / 'audio-driver.bin').read_bytes()
    if hashlib.sha256(driver).hexdigest() != pins(ROOT / 'manifest/audio-inputs.sha256')['driver-expanded.bin']:
        raise ValueError('AUDIO ORDER / ORIGINAL DRIVER IDENTITY')
    receipt = json.loads((run / 'build-receipt.json').read_text())
    required = {'-DPOP2_AUDIO', '-DPOP2_AUDIO_PROBE', '-DPOP2_AUDIO_ORDER_PROBE', '-DPOP2_VERIFY'}
    if not required.issubset(receipt['flags'].split()):
        raise ValueError('AUDIO ORDER / FIXTURE BUILD FLAGS')
    for key, name in (('exe', 'dh1/PrinceOfPersia2'), ('elf', 'PrinceOfPersia2.elf')):
        if hashlib.sha256((run / name).read_bytes()).hexdigest() != receipt[key]:
            raise ValueError('AUDIO ORDER / BUILD IDENTITY')
    log = (run / 'gdb-out.log').read_text()
    restored(run, log, 'a1200-030')
    if log.count('CORE REASON AUDIO / CALLBACK ORDER COMPLETE\n') != 1:
        raise ValueError('AUDIO ORDER / NATIVE COMPLETION')
    if 'CORE EXCEPTION vector=0 pc=00000000\n' not in log:
        raise ValueError('AUDIO ORDER / NATIVE EXCEPTION')
    if 'AUDIO REPLAY calls=7 done=7 callbacks=1 abi=7 dma=2\n' not in log:
        raise ValueError('AUDIO ORDER / NATIVE COVERAGE')
    adk = re.search(r'AUDIO ADK saved=(\d+) actual=(\d+)', log)
    if not adk or adk[1] != adk[2] or 'sound_output=exact' not in (run / 'logs/debug.uae').read_text():
        raise ValueError('AUDIO ORDER / PAULA STATE')
    data = (run / 'audio-events.bin').read_bytes()
    if len(data) != 7 * 24:
        raise ValueError('AUDIO ORDER / NATIVE ROW EXTENT')
    rows = list(struct.iter_unpack('>6I', data))
    if any(row[5] != i + 1 for i, row in enumerate(rows)):
        raise ValueError('AUDIO ORDER / REGISTER PRESERVATION')
    # Native row zero initializes the driver; the Macintosh fixture starts
    # with the original game's initialized driver. Compare the six shared calls.
    if [row[0] for row in rows[1:]] != [row[3] for row in original]:
        raise ValueError('AUDIO ORDER / QUERY RESULTS')
    if rows[2][1:5] != (3, 5, 0, 0) or any(row[1:5] != (7, 7, 1, 1) for row in rows[3:]):
        raise ValueError('AUDIO ORDER / CALLBACK BEFORE REPLACEMENT')
    print('AUDIO CALLBACK ORDER PASS: original-paired replacement, queries, callback and register preservation')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('reference', type=Path)
    args = parser.parse_args()
    check(args.run, args.reference)
