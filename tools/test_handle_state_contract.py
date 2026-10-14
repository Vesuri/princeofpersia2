"""Reject missing or altered memory-manager contract evidence."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from check_handle_state_contract import ROOT, check


class HandleContractTests(unittest.TestCase):
    def test_original_and_live(self):
        check(ROOT/'ref/mame/runs/gameplay-hunlock-contract-1')
        check(ROOT/'ref/mame/runs/gameplay-assisted-handles-1',live=True)
        check(ROOT/'ref/mame/runs/gameplay-assisted-ending-handles-1',ending=True)

    def test_reject_evidence_changes(self):
        source=ROOT/'ref/mame/runs/gameplay-hunlock-contract-1/session.log'
        text=source.read_text()
        changes=[text.replace('HANDLE_STATE_FIXTURE_COMPLETE','INCOMPLETE'),
                 text.replace('memerr=FF8F','memerr=0000'),
                 text.replace('stage=2','stage=1'),
                 text.replace('d0=FFFFFF8F','d0=00000000')]
        with tempfile.TemporaryDirectory(dir=ROOT/'tmp',prefix='handle-contract-') as directory:
            run=Path(directory)
            for changed in changes:
                self.assertNotEqual(changed,text)
                (run/'session.log').write_text(changed)
                with self.assertRaises(AssertionError):check(run)
                with patch('check_handle_state_contract.pins',return_value={}):
                    with self.assertRaises(AssertionError):check(run)


if __name__=='__main__':unittest.main()
