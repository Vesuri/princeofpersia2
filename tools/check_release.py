"""Independently extract and audit the installer candidate with Lhasa."""
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path
from package_release import INSTALLER_SOURCE

ROOT = Path(__file__).resolve().parents[1]
MEMBERS = {
    'PrinceOfPersia2', 'PrinceOfPersia2.slave', 'Pop2InstallData',
    'PrinceOfPersia2.inf', 'Install', 'Install.info', 'ReadMe',
    'ReadMe.info', 'LICENSE.LGPL.txt', 'video-transfer.bin',
}
MEMBERS.update('InstallerSource/' + name for name in INSTALLER_SOURCE)
MEMBERS.add('InstallerSource/Build.txt')


def check():
    version = (ROOT / 'VERSION').read_text().strip()
    archive = ROOT / 'dist' / ('PrinceOfPersia2-' + version + '.lha')
    report = json.loads(archive.with_suffix('.json').read_text())
    data = archive.read_bytes()
    assert report['sha256'] == hashlib.sha256(data).hexdigest()
    assert report['bytes'] == len(data)
    assert report['version'] == version and report['date'] == '19.10.2026'
    assert set(report['members']) == MEMBERS
    with tempfile.TemporaryDirectory(prefix='release-audit-', dir=ROOT / 'tmp') as temp:
        base = Path(temp)
        subprocess.run(['lha', 'xq', str(archive)], cwd=base, check=True)
        folder = base / 'Prince of Persia 2 Install'
        expected = {'Prince of Persia 2 Install/' + n for n in MEMBERS}
        expected.add('Prince of Persia 2 Install.info')
        assert {str(p.relative_to(base)) for p in base.rglob('*') if p.is_file()} == expected
        for name, digest in report['members'].items():
            assert hashlib.sha256((folder / name).read_bytes()).hexdigest() == digest, name
        for name in ['PrinceOfPersia2', 'Pop2InstallData']:
            assert (folder / name).read_bytes()[:4] == b'\0\0\x03\xf3', name
        tag = (version + ' (19.10.2026)').encode()
        for name in ['PrinceOfPersia2', 'PrinceOfPersia2.slave', 'Pop2InstallData', 'Install', 'ReadMe']:
            assert tag in (folder / name).read_bytes(), name
        slave = (folder / 'PrinceOfPersia2.slave').read_bytes()
        assert b'WHDLOADS' in slave
        for name in ['Install.info', 'ReadMe.info', 'PrinceOfPersia2.inf']:
            assert (folder / name).read_bytes()[:2] == b'\xe3\x10', name
        assert (base / 'Prince of Persia 2 Install.info').read_bytes()[:2] == b'\xe3\x10'
    print('RELEASE ARCHIVE PASS: independent extraction, exact members, hashes, version and formats')


if __name__ == '__main__':
    check()
