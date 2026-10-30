"""Independently extract and audit the installer candidate with Lhasa."""
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MEMBERS = {
    'PrinceOfPersia2', 'PrinceOfPersia2.slave', 'Pop2InstallData',
    'PrinceOfPersia2.inf', 'Install', 'Install.info', 'ReadMe',
    'ReadMe.info', 'LICENSE.LGPL.txt',
}
DATE = '31.10.2026'


def check():
    version = (ROOT / 'VERSION').read_text().strip()
    archive = ROOT / 'dist' / ('PrinceOfPersia2-' + version + '.lha')
    report = json.loads(archive.with_suffix('.json').read_text())
    data = archive.read_bytes()
    assert report['sha256'] == hashlib.sha256(data).hexdigest()
    assert report['bytes'] == len(data)
    assert report['version'] == version and report['date'] == DATE
    assert set(report['members']) == MEMBERS
    with tempfile.TemporaryDirectory(prefix='release-audit-', dir=ROOT / 'tmp') as temp:
        base = Path(temp)
        subprocess.run(['lha', 'xq', str(archive)], cwd=base, check=True)
        folder = base / 'PoP2 Install'
        expected = {'PoP2 Install/' + n for n in MEMBERS}
        expected.add('PoP2 Install.info')
        assert {str(p.relative_to(base)) for p in base.rglob('*') if p.is_file()} == expected
        for name, digest in report['members'].items():
            assert hashlib.sha256((folder / name).read_bytes()).hexdigest() == digest, name
        for name in ['PrinceOfPersia2', 'Pop2InstallData']:
            assert (folder / name).read_bytes()[:4] == b'\0\0\x03\xf3', name
        from display_inputs import header
        transfer=(ROOT/'build/display/video-transfer.bin').read_bytes()
        header(transfer)  # Verify the measured input before auditing its linked bytes.
        executable=(folder/'PrinceOfPersia2').read_bytes()
        assert transfer in executable, 'RELEASE / EMBEDDED VIDEO TRANSFER'
        assert b'video-transfer.bin' not in executable, 'RELEASE / EXTERNAL VIDEO TRANSFER'
        tag = (version + ' (' + DATE + ')').encode()
        for name in ['PrinceOfPersia2', 'PrinceOfPersia2.slave', 'Pop2InstallData', 'Install', 'ReadMe']:
            assert tag in (folder / name).read_bytes(), name
        slave = (folder / 'PrinceOfPersia2.slave').read_bytes()
        assert b'WHDLOADS' in slave
        for name in ['Install.info', 'ReadMe.info', 'PrinceOfPersia2.inf']:
            assert (folder / name).read_bytes()[:2] == b'\xe3\x10', name
        assert (base / 'PoP2 Install.info').read_bytes()[:2] == b'\xe3\x10'
    print('RELEASE ARCHIVE PASS: independent extraction, exact members, hashes, version and formats')


if __name__ == '__main__':
    check()
