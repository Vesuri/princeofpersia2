"""Reject missing and inconsistent eligible-update deadline evidence."""
import unittest
from carpet_deadlines import validate


class Deadlines(unittest.TestCase):
    def test_rearm_and_late_poll(self):
        rows=[(10,10,3,0,1,16,6,1,1,0),(18,16,6,1,0,21,3,2,1,1)]
        result=validate(rows)
        self.assertEqual([r['late_ticks'] for r in result[0]],[0,2])
        for row,column,value in [(0,5,13),(0,1,11),(0,4,0),(1,1,15),(1,2,5),(1,3,0),(1,7,0)]:
            bad=[list(r) for r in rows];bad[row][column]=value
            with self.subTest(row=row,column=column),self.assertRaises(SystemExit):validate(bad)
        with self.assertRaises(SystemExit):validate([])



class OriginalControls(unittest.TestCase):
    def test_completed_reference(self):
        import tempfile
        from pathlib import Path
        from carpet_deadlines import original
        source=Path(__file__).resolve().parents[1]/'ref/mame/runs/deadlines-carpet-original-b/session.log'
        if not source.exists():self.skipTest('local original capture not installed')
        good=source.read_text()
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory);log=path/'session.log';log.write_text(good)
            self.assertEqual([len(a) for a in original(path)],[119,81,90])
            for old,new in [('GAMEPLAY_DISCOVERY_COMPLETE','MISSING_COMPLETE'),
                            ('selector=1 site=0454','selector=1 site=0455'),('selector=1 site=0392','selector=2 site=0392')]:
                self.assertIn(old,good);log.write_text(good.replace(old,new,1))
                with self.subTest(old=old),self.assertRaises(SystemExit):original(path)
            log.write_text(good);original(path)

class NativeControls(unittest.TestCase):
    def test_completed_native(self):
        import tempfile
        from pathlib import Path
        from carpet_deadlines import native
        source=Path(__file__).resolve().parents[1]/'tmp/aprof/deadlines-carpet-native-c'
        if not (source/'scene-animation.bin').exists():self.skipTest('local native capture not installed')
        good=(source/'scene-animation.bin').read_bytes();text=(source/'gdb-out.log').read_text()
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory);data=path/'scene-animation.bin';log=path/'gdb-out.log'
            data.write_bytes(good);log.write_text(text)
            self.assertEqual([len(a) for a in native(path)],[119,81,90])
            mutations=[good[:-40]]
            for offset in (4,20,28,32):
                changed=bytearray(good);changed[offset+3]^=1;mutations.append(changed)
            for altered in mutations:
                self.assertNotEqual(good,altered);data.write_bytes(altered)
                with self.assertRaises(SystemExit):native(path)
            data.write_bytes(good);log.write_text(text.replace('SCENE RESTORED','MISSING RESTORATION'))
            with self.assertRaises(SystemExit):native(path)
            log.write_text(text);native(path)

    def test_method_table_restoration(self):
        import tempfile
        from pathlib import Path
        from unittest.mock import patch
        from aprof_scene_report import read
        source=Path(__file__).resolve().parents[1]/'tmp/aprof/deadlines-carpet-native-c'
        if not (source/'scene-animation.bin').exists():self.skipTest('local native capture not installed')
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);capture=root/'capture';capture.mkdir()
            for name in ('config.txt','gdb-out.log','build-receipt.json','scene-animation.bin',
                         'scene-start-a5.bin','scene-end-a5.bin'):
                (capture/name).write_bytes((source/name).read_bytes())
            with patch('aprof_scene_report.OUT',root):
                read('capture')
                path=capture/'scene-end-a5.bin';good=path.read_bytes();bad=bytearray(good)
                bad[-0x226c]^=1;path.write_bytes(bad)
                with self.assertRaises(SystemExit):read('capture')
                path.write_bytes(good);read('capture')


if __name__=='__main__':unittest.main()
