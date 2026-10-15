"""Reject incomplete or altered evidence for the room-two cinematic pair."""
from pathlib import Path
import unittest
from unittest.mock import patch
from check_gameplay_story import check
from extract_original_data import ROOT, pins


class PrincessPair(unittest.TestCase):
    def test_positive_pair_and_corruption_controls(self):
        native=ROOT/'amiga/.run-UvXvud'
        reference=ROOT/'ref/mame/runs/gameplay-princess-room-1'
        manifest=ROOT/'manifest/gameplay-princess-reference.sha256'
        check(native,reference,manifest,5)
        incomplete=pins(manifest)
        del incomplete['nis-3-5-copy-2.indices']
        with patch('check_gameplay_story.pins',return_value=incomplete):
            with self.assertRaisesRegex(AssertionError,'CAPTURE COVERAGE'):
                check(native,reference,manifest,5)
        read=Path.read_bytes
        def changed(path,*args,**kwargs):
            data=read(path,*args,**kwargs)
            if path==native/'nis-3-5-copy-2-screen.bin':
                data=bytearray(data);data[48*640+65]^=1;return bytes(data)
            return data
        with patch.object(Path,'read_bytes',changed):
            with self.assertRaisesRegex(AssertionError,'PIXELS'):
                check(native,reference,manifest,5)
        read_text=Path.read_text
        def missing_return(path,*args,**kwargs):
            data=read_text(path,*args,**kwargs)
            return data.replace('CUTSCENE_RETURN ordinal=3 selector=5','MISSING_RETURN') if path==native/'gdb-out.log' else data
        with patch.object(Path,'read_text',missing_return):
            with self.assertRaisesRegex(AssertionError,'NORMAL RETURN'):
                check(native,reference,manifest,5)


if __name__=='__main__':unittest.main()
