"""Verify the original-instruction BankRLELR differential fixture."""
import hashlib
from extract_original_data import ROOT, pins


def verify_fixture(path):
    recorded = pins(ROOT/'manifest/bank-skip-inputs.sha256')
    for name in ('original.bin', 'replacement.bin', 'encoded.bin', 'cases.json', 'cases.lua', 'fixture.h'):
        file = path.resolve()/name
        key = str(file.relative_to(ROOT))
        assert key in recorded and hashlib.sha256(file.read_bytes()).hexdigest() == recorded[key], key
