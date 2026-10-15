"""A matched rejected entry must remain a failure; missing state must reject."""
from pathlib import Path
import unittest
from unittest.mock import patch
from extract_original_data import ROOT
from inspect_gameplay_rejected_entry import inspect


class RejectedEntry(unittest.TestCase):
    def test_state_match_and_rejection_controls(self):
        native=ROOT/'amiga/.run-CeIpbt'
        reference=ROOT/'ref/mame/runs/gameplay-assisted-level10-clean-1'
        manifest=ROOT/'manifest/gameplay-assisted-level10-clean.sha256'
        report=inspect(native,reference,manifest)
        self.assertFalse(report['route_accepted'])
        self.assertEqual((report['native_frames'],report['original_frames']),(290,292))
        read_text=Path.read_text
        for old,new in (('ROUTE RESULT complete=0','ROUTE RESULT complete=1'),
                        ('ROUTE REJECTED ENTRY','MISSING ENTRY')):
            def changed(path,*args,**kwargs):
                text=read_text(path,*args,**kwargs)
                return text.replace(old,new) if path==native/'gdb-out.log' else text
            with patch.object(Path,'read_text',changed):
                with self.assertRaises(AssertionError):inspect(native,reference,manifest)
        read=Path.read_bytes
        def corrupt(path,*args,**kwargs):
            data=read(path,*args,**kwargs)
            if path==native/'route-rejected-entry-context.bin':
                data=bytearray(data);data[0]^=1;return bytes(data)
            return data
        with patch.object(Path,'read_bytes',corrupt):
            with self.assertRaisesRegex(AssertionError,'FINAL CONTEXT'):
                inspect(native,reference,manifest)


if __name__=='__main__':unittest.main()
