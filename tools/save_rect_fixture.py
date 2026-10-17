"""Verify the independently pinned original-loop fixture inputs."""
import hashlib
from extract_original_data import ROOT,pins


def verify_fixture(path):
    recorded=pins(ROOT/'manifest/save-rect-inputs.sha256')
    for name in ('original.bin','cases.json','cases.lua','fixture.h'):
        p=path.resolve()/name;key=str(p.relative_to(ROOT))
        if key not in recorded or hashlib.sha256(p.read_bytes()).hexdigest()!=recorded[key]:
            raise ValueError('SAVE RECT / FIXTURE INPUT PIN '+key)
