"""Reject malformed or unlogged assisted state changes before native replay."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from gameplay_assisted_inputs import ROOT, REFERENCE, generate, metadata


class AssistedInputs(unittest.TestCase):
    def test_pinned_ending_and_completion_controls(self):
        reference=ROOT/'ref/mame/runs/gameplay-assisted-ending-10'
        manifest=ROOT/'manifest/gameplay-assisted-ending.sha256'
        keys,points,writes,labels=metadata(reference,manifest)
        self.assertEqual((len(keys),len(points),len(writes)),(387,81,41))
        self.assertEqual(points[-1][:4],(385,14,7,387))
        self.assertIn('ASSISTED ENDING COMPLETE',generate(reference,manifest))
        original=(reference/'session.log').read_text()
        changes=[original.replace('GAMEPLAY_VICTORY result=15','GAMEPLAY_VICTORY result=14'),
                 original.replace('GAMEPLAY_ROUTE_NIS_RETURN selector=11','MISSING_ENDING_RETURN'),
                 original.replace('reason=ending-return','reason=completed-frames'),
                 original.replace('reason=completed-frames','reason=state-load',1)]
        with tempfile.TemporaryDirectory(dir=ROOT/'tmp',prefix='ending-controls-') as directory:
            candidate=Path(directory)
            for source in reference.glob('route-*'):
                if source.suffix in ('.bin','.rgb','.indices'):(candidate/source.name).symlink_to(source)
            with patch('gameplay_assisted_inputs.pins',return_value={}):
                (candidate/'session.log').write_text(original);metadata(candidate)
                for changed in changes:
                    (candidate/'session.log').write_text(changed)
                    with self.assertRaises(AssertionError):metadata(candidate)

    def test_pinned_survey(self):
        keys,points,writes,labels=metadata()
        self.assertEqual({p[1] for p in points},set(range(1,15)))
        self.assertEqual(points[-1][:3],(390,14,1))
        self.assertEqual(len(writes),65)
        self.assertTrue(keys and len(points)==len(labels))
        self.assertIn('ASSISTED SURVEY COMPLETE',generate())

    def test_reject_corruption(self):
        original=(REFERENCE/'session.log').read_text()
        first=next(line for line in original.splitlines() if line.startswith('GAMEPLAY_ASSIST '))
        changes=[original.replace('GAMEPLAY_DISCOVERY_COMPLETE','INCOMPLETE'),
                 original.replace(first+'\n','',1),
                 original.replace(first,first.replace('a5minus=4FAA','a5minus=0002'),1),
                 original.replace(first,first.replace('before=0001','before=FFFF'),1),
                 original.replace('reason=assist-before','reason=state-load',1)]
        with tempfile.TemporaryDirectory(dir=ROOT/'tmp',prefix='assisted-controls-') as directory:
            candidate=Path(directory)
            for source in REFERENCE.glob('route-*'):
                if source.suffix in ('.bin','.rgb','.indices'):
                    (candidate/source.name).symlink_to(source)
            # Production checks pins first. Isolate semantic rejection here.
            with patch('gameplay_assisted_inputs.pins',return_value={}):
                (candidate/'session.log').write_text(original);metadata(candidate)
                for changed in changes:
                    self.assertNotEqual(changed,original)
                    (candidate/'session.log').write_text(changed)
                    with self.assertRaises(AssertionError):metadata(candidate)


if __name__=='__main__':unittest.main()
