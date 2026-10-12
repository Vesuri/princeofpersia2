#!/usr/bin/env python3
"""Build a fresh, verified local reference volume without overwriting an existing run."""
import argparse
import hashlib
from pathlib import Path
import shutil
import zipfile

from extract_original_data import ROOT, pins, verify_output, DESTINATION
from install_reference_volume import install


def prepare(destination, system):
    destination = destination.absolute()
    if destination.resolve() != destination or not destination.is_relative_to(ROOT/'ref/mame/runs'):
        raise ValueError('REFERENCE / RUN MUST BE UNDER ref/mame/runs WITHOUT SYMLINKS')
    if destination.exists():
        raise ValueError('REFERENCE / RUN ALREADY EXISTS; choose a new run name')
    for name, digest in pins(ROOT/'manifest/reference-inputs.sha256').items():
        with (ROOT/name).open('rb') as stream:
            actual=hashlib.file_digest(stream,'sha256').hexdigest()
        if actual != digest:
            raise ValueError('REFERENCE / INPUT HASH MISMATCH: '+name)
    verify_output(DESTINATION)
    destination.mkdir(parents=True)
    for name in ('cfg','nvram','snap'):
        (destination/name).mkdir()
    volume = destination/'volume.hd'
    with zipfile.ZipFile(ROOT/f'ref/mame/dl/{system}_2GB_drive.zip') as archive:
        expected = f'{system}_2GB_drive.dsk'
        if archive.namelist() != [expected]:
            raise ValueError('REFERENCE / SYSTEM ARCHIVE MEMBERS')
        with archive.open(expected) as source, volume.open('wb') as output:
            shutil.copyfileobj(source, output)
    install(volume)
    (destination/'cfg/maciici.cfg').write_text(
        '<?xml version="1.0"?><mameconfig version="10"><system name="maciici">'
        '<input><keyboard tag=":macadb" enabled="1"/></input></system></mameconfig>\n')
    (destination/'nvram/maciici').mkdir()
    shutil.copyfile(ROOT/'ref/mame/inputs/rtc-iici-256', destination/'nvram/maciici/rtc')
    print('PREPARE PASS:',destination.relative_to(ROOT))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination',type=Path)
    parser.add_argument('--system',choices=['608','755'],default='755')
    args=parser.parse_args()
    prepare(args.destination,args.system)
