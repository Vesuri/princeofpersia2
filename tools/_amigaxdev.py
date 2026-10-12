"""Locate and verify the shared Mac helpers before importing them."""
import hashlib
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
PYLIB = Path(os.environ.get('AMIGA_PYLIB') or
             Path.home() / '.local/share/amiga/pylib')
for line in (ROOT / 'manifest/mac-helpers.sha256').read_text().splitlines():
    digest, name = line.split('  ', 1)
    if hashlib.sha256((PYLIB / name).read_bytes()).hexdigest() != digest:
        raise RuntimeError('SHARED HELPER / SHA-256 MISMATCH: ' + name)
sys.path.insert(0, str(PYLIB))
