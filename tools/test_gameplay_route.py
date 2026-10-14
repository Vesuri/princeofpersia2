"""Reject malformed input schedules and corrupted route comparison evidence."""
from pathlib import Path
import tempfile
import unittest
import hashlib
from unittest.mock import patch

from replay_gameplay_route import compare, inputs, verify_reference, replay, ROOT


class RouteEvidenceTests(unittest.TestCase):
    def test_schedule(self):
        with tempfile.TemporaryDirectory(dir=ROOT / 'tmp') as directory:
            path = Path(directory) / 'inputs.tsv'
            path.write_text('# authored input\n8 left,up\n2 none\n')
            self.assertEqual(inputs(path), [(8, 'left,up'), (2, 'none')])
            for bad in ('', '0 left\n', '601 left\n', '8 unknown\n',
                        'save rooftop\n', '8 left,,up\n'):
                path.write_text(bad)
                with self.subTest(bad=bad), self.assertRaises(ValueError):
                    inputs(path)

    def test_reference_pin_rejection(self):
        with tempfile.TemporaryDirectory(dir=ROOT / 'tmp') as directory:
            reference = Path(directory)
            path = reference / 'evidence.bin'
            path.write_bytes(b'authored fixture')
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            with patch('replay_gameplay_route.pins', return_value={'evidence.bin': digest}):
                verify_reference(reference)
                path.write_bytes(b'changed fixture')
                with self.assertRaisesRegex(ValueError, 'REFERENCE PIN'):
                    verify_reference(reference)

    def test_unreached_endpoint(self):
        with tempfile.TemporaryDirectory(dir=ROOT / 'tmp') as directory:
            run = Path(directory)
            schedule = run / 'inputs.tsv'
            schedule.write_text('8 left\n')
            checkpoints = [dict(level='1', reason='first-frame'),
                           dict(level='1', reason='completed-frames')]
            with patch('replay_gameplay_route.wait', side_effect=checkpoints):
                with self.assertRaisesRegex(ValueError, 'NEXT LEVEL NOT REACHED'):
                    replay(run, schedule)

    def test_multiple_level_replay(self):
        with tempfile.TemporaryDirectory(dir=ROOT / 'tmp') as directory:
            run = Path(directory)
            for level in (2, 3, 4):
                states = [dict(sequence='0', level='1', reason='first-frame', frames='0')]
                for i in range(1, level + 1):
                    states.append(dict(sequence=str(i), level=str(i + 1), reason='next-level', frames=str(i * 8)))
                states.append(dict(sequence=str(level + 1), level=str(level + 1), reason='discovery-finished', frames=str(level * 8)))
                for state in states:
                    state['step'] = state['frames']
                for bad_index in (None, *range(1, level + 2)):
                    replies = [row.copy() for row in states]
                    if bad_index is not None:
                        replies[bad_index]['reason'] = 'completed-frames'
                    with patch('replay_gameplay_route.inputs', side_effect=[[(8, 'left')] for _ in range(level)]), patch('replay_gameplay_route.wait', side_effect=replies), patch('replay_gameplay_route.status', side_effect=states[:level]):
                        if bad_index is None:
                            replay(run, run / 'authored.tsv', through_level=level)
                            self.assertEqual((run / 'route-command.txt').read_text(), f'{level + 1} finish\n')
                        else:
                            with self.subTest(level=level, endpoint=bad_index), self.assertRaises(ValueError):
                                replay(run, run / 'authored.tsv', through_level=level)

    def test_capture_rejections(self):
        with tempfile.TemporaryDirectory(dir=ROOT / 'tmp') as directory:
            reference = Path(directory) / 'reference'
            actual = Path(directory) / 'actual'
            reference.mkdir(); actual.mkdir()
            sizes = {'-context.bin': 0x2200, '.indices': 640 * 480,
                     '.rgb': 768, '-a5.bin': 22266}
            for suffix, size in sizes.items():
                for target in (reference, actual):
                    (target / ('route-0' + suffix)).write_bytes(bytes(size))
            compare(actual, reference, 0)
            # Prove that each claimed state/pixel category is actually checked.
            mutations = [('-context.bin', 0x218a), ('.indices', 48 * 640 + 65),
                         ('.rgb', 13), ('-a5.bin', 22266 - 0x5208),
                         ('-a5.bin', 22266 - 0x50d2 + 30),
                         ('-a5.bin', 22266 - 0x18fc),
                         ('-a5.bin', 22266 - 0x4fd2),
                         ('-a5.bin', 22266 - 0x4fc2),
                         ('-a5.bin', 22266 - 0x349a)]
            for suffix, offset in mutations:
                path = actual / ('route-0' + suffix)
                good = path.read_bytes()
                bad = bytearray(good); bad[offset] ^= 1
                path.write_bytes(bad)
                with self.subTest(suffix=suffix, offset=offset), self.assertRaises(ValueError):
                    compare(actual, reference, 0)
                path.write_bytes(good)
            for suffix in sizes:
                path = actual / ('route-0' + suffix)
                good = path.read_bytes(); path.write_bytes(good[:-1])
                with self.subTest(extent=suffix), self.assertRaises(ValueError):
                    compare(actual, reference, 0)
                path.write_bytes(good)


if __name__ == '__main__':
    unittest.main()
