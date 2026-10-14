"""Check pinned replay boundaries and reject malformed consumed-input records."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from gameplay_route_inputs import REFERENCES, ROOT, generate, metadata


class RouteInputTests(unittest.TestCase):
    def test_pinned_boundaries(self):
        for level, count, captures in ((1, 1002, 96), (2, 1362, 115), (3, 2770, 181)):
            rows, points = metadata(REFERENCES[level], level)
            self.assertEqual((len(rows), len(points)), (count, captures))
            self.assertEqual(sum(int(row[1]) == 1001 for row in rows), min(level, 2))
            self.assertEqual([row[1] for row in points if row[4]], list(range(1, level + 2)))
            header = generate(REFERENCES[level], level)
            self.assertIn(f'#if POP2_GAMEPLAY_ROUTE_LEVEL != {level}', header)

    def test_bad_records(self):
        original = (REFERENCES[2] / 'session.log').read_text()
        key = next(line for line in original.splitlines() if line.startswith('GAMEPLAY_ROUTE_KEYS frame=1001 '))
        first_key = next(line for line in original.splitlines() if line.startswith('GAMEPLAY_ROUTE_KEYS '))
        changes = [original.replace(key + '\n', '', 1),
                   original.replace('frame=1360 step=1360 map=', 'frame=1359 step=1359 map='),
                   original.replace('sequence=114 frames=1360', 'sequence=115 frames=1360'),
                   original.replace('reason=next-level prefix=route-114', 'reason=state-load prefix=route-114'),
                   original.replace(first_key, first_key.split('map=')[0] + 'map=01' + '00' * 15, 1)]
        with tempfile.TemporaryDirectory(prefix='route-input-controls-', dir=ROOT / 'tmp') as directory:
            candidate = Path(directory)
            # Schema controls bypass hashes deliberately; production generation
            # still verifies the complete manifest before parsing any input.
            with patch('gameplay_route_inputs.verify_reference'):
                (candidate / 'session.log').write_text(original)
                generate(candidate, 2)
                for changed in changes:
                    self.assertNotEqual(changed, original)
                    (candidate / 'session.log').write_text(changed)
                    with self.assertRaises(AssertionError):
                        generate(candidate, 2)


if __name__ == '__main__':
    unittest.main()
