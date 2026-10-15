"""Reproduce and validate the original contracts used by startup integration."""
import argparse
import os
from pathlib import Path
import shutil
import tempfile
from check_fixed_contracts import check as check_fixed
from check_font_contracts import check as check_font
from check_world_contracts import check as check_world
from check_title_copy_contract import check as check_copy
from check_integration import compress_completed_volume
from extract_original_data import ROOT
from run_reference import run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--references', nargs=4, type=Path, metavar=('FIXED', 'FONT', 'WORLD', 'COPY'))
    args = parser.parse_args()
    os.chdir(ROOT)
    for key in list(os.environ):
        if key.startswith('POP2_'):
            os.environ.pop(key)
    checks = (check_fixed, check_font, check_world, check_copy)
    if args.references:
        references = [p.resolve() for p in args.references]
    else:
        references = []
        settings = ({'POP2_INTEGRATION': '1', 'POP2_FIXED_PROBE': '1'},
                    {'POP2_INTEGRATION': '1'},
                    {'POP2_NEWGWORLD': '1', 'POP2_DISPLAY_ROUTE': '1'},
                    {'POP2_TITLE_COPY': '1', 'POP2_TITLE_COPY_ALL': '1', 'POP2_DISPLAY_ROUTE': '1'})
        for settings_for_run, check in zip(settings, checks):
            os.environ.update(settings_for_run)
            reference = run()
            check(reference)
            compress_completed_volume(reference)
            references.append(reference)
            for key in settings_for_run:
                os.environ.pop(key)
    for check, reference in zip(checks, references):
        check(reference)

    with tempfile.TemporaryDirectory(prefix='integration-contract-controls-', dir=ROOT/'tmp') as directory:
        scratch = Path(directory)
        # Copy only contract inputs, never a reference disk, ROM or emulator state.
        for index, reference in enumerate(references):
            destination = scratch/str(index)
            destination.mkdir()
            for source in reference.glob('integration-*.bin'):
                shutil.copyfile(source, destination/source.name)
            shutil.copyfile(reference/'session.log', destination/'session.log')
        changes = (
            (0, 'session.log', lambda b: b.replace(b'FIXED_FIXTURE_COMPLETE calls=80', b'FIXED_FIXTURE_INCOMPLETE')),
            (0, 'session.log', lambda b: b.replace(b'result=00000000', b'result=00000001', 1)),
            (1, 'session.log', lambda b: b.replace(b'INTEGRATION_COMPLETE calls=', b'INTEGRATION_INCOMPLETE calls=')),
            (1, 'integration-325-before-port.bin', lambda b: b[:68]+bytes([b[68]^1])+b[69:]),
            (1, 'integration-331-after-stack.bin', lambda b: b[1:]),
            (2, 'session.log', lambda b: b.replace(b'INTEGRATION_COMPLETE calls=23', b'INTEGRATION_INCOMPLETE')),
            (2, 'session.log', lambda b: b.replace(b'DISPLAY_ROUTE_CAPTURED first-room', b'DISPLAY_ROUTE_MISSING first-room')),
            (3, 'session.log', lambda b: b.replace(b'INTEGRATION_COMPLETE calls=65', b'INTEGRATION_INCOMPLETE')),
            (3, 'integration-1-after-copy-Destination-pixels.bin', lambda b: bytes([b[0]^1])+b[1:]),
            (2, 'integration-23-world-map.bin', lambda b: b[:5]+bytes([b[5]^1])+b[6:]),
        )
        for index, name, mutate in changes:
            path = scratch/str(index)/name
            good = path.read_bytes()
            bad = mutate(good)
            assert bad != good, 'CONTRACT / INACTIVE REJECTION CONTROL '+name
            path.write_bytes(bad)
            try:
                checks[index](path.parent)
            except (AssertionError, ValueError):
                pass
            else:
                raise AssertionError('CONTRACT / ACCEPTED ALTERED EVIDENCE '+name)
            finally:
                path.write_bytes(good)
        for index, check in enumerate(checks):
            check(scratch/str(index))
    print(f'INTEGRATION CONTRACTS PASS: four original comparisons; {len(changes)} rejected alterations', flush=True)


if __name__ == '__main__':
    main()
