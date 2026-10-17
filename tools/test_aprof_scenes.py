"""Reject altered matched-workload evidence and unsafe measurement observers."""
import copy
import unittest
import tempfile
from pathlib import Path
from unittest.mock import patch
from aprof_scene_report import read
from aprof_scene_report import compare
from aprof_scenes import observer, check


class Controls(unittest.TestCase):
    def test_matched_workload(self):
        a=dict(animation_signature=[],config={'scene':'carpet','cache':'clean'},selector=1,route_start=[12,14],
               timer_requests=2,states=['initial','final'],request_signature=[[12,0,5,15,0x100]],fields=100,build_identity={'exe':'same','elf':'same'},clock_signature=[[0x3152,240,0]])
        compare(a,copy.deepcopy(a))
        changes=dict(animation_signature=[[(0,1,7)]],config={'scene':'carpet','cache':'generated'},selector=6,route_start=[11,14],
                     timer_requests=1,states=['initial','corrupt'],request_signature=[[12,0,6,15,0x100]],fields=110,build_identity={'exe':'changed','elf':'same'},clock_signature=[[0x3152,180,0]])
        for key,value in changes.items():
            with self.subTest(key=key):
                b=copy.deepcopy(a);b[key]=value
                with self.assertRaises(SystemExit):compare(a,b)

    def test_completed_capture_controls(self):
        source=Path(__file__).resolve().parents[1]/'tmp/aprof/throne-clean-c'
        if not source.exists():self.skipTest('local scene evidence not installed')
        names=('config.txt','gdb-out.log','scene-timers.bin','scene-clocks.bin',
               'scene-start-a5.bin','scene-end-a5.bin','build-receipt.json')
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);capture=root/'capture';capture.mkdir()
            for name in names:(capture/name).write_bytes((source/name).read_bytes())
            with patch('aprof_scene_report.OUT',root):
                read('capture')
                changes=(
                    ('gdb-out.log',lambda b:b.replace(b'SCENE RESTORED',b'SCENE MISSING')),
                    ('scene-timers.bin',lambda b:b[:-1]),
                    ('scene-clocks.bin',lambda b:b[:-32]),
                    ('scene-clocks.bin',lambda b:b.replace(bytes.fromhex('00003152'),bytes.fromhex('00003153'),1)),
                    ('scene-end-a5.bin',lambda b:b[:-1]),
                )
                for name,mutate in changes:
                    with self.subTest(name=name):
                        path=capture/name;good=path.read_bytes();bad=mutate(good)
                        self.assertNotEqual(good,bad);path.write_bytes(bad)
                        try:
                            with self.assertRaises(SystemExit):read('capture')
                        finally:path.write_bytes(good)
                read('capture')

    def test_native_completion_controls(self):
        root=Path(__file__).resolve().parents[1]
        evidence=root/'tmp/aprof/throne-clean-c/run-dir.txt'
        if not evidence.exists():self.skipTest('local scene evidence not installed')
        source=root/evidence.read_text().strip()
        machine=dict(cpu_model='68030',mmu_model=None,fastmem_size='8',
                     cpu_frequency='50000000',ntsc='false',cpu_cycle_exact='true')
        with tempfile.TemporaryDirectory() as temp:
            capture=Path(temp);(capture/'dh1').mkdir()
            for name in ('gdb-out.log','build-receipt.json','scene-timers.bin','scene-clocks.bin','dh1/Shell.done'):
                (capture/name).write_bytes((source/name).read_bytes())
            for name in ('dh1/PrinceOfPersia2','PrinceOfPersia2.elf'):
                (capture/name).symlink_to(source/name)
            with patch('aprof_report.machine',return_value=machine):
                check(capture)
                changes=(
                    ('gdb-out.log',lambda b:b.replace(b'CORE REASON SCENE PROFILE / COMPLETE',b'CORE REASON UNKNOWN')),
                    ('gdb-out.log',lambda b:b.replace(b'input=1',b'input=0')),
                    ('scene-clocks.bin',lambda b:b[:-32]),
                    ('build-receipt.json',lambda b:b.replace(b'-DPOP2_AUDIO',b'-DPOP2_VERIFY')),
                    ('dh1/Shell.done',lambda b:b'SHELL PASS'),
                )
                for name,mutate in changes:
                    with self.subTest(name=name):
                        path=capture/name;good=path.read_bytes();bad=mutate(good)
                        self.assertNotEqual(good,bad);path.write_bytes(bad)
                        try:
                            with self.assertRaises(SystemExit):check(capture)
                        finally:path.write_bytes(good)
                for key,value in (('cpu_model','68040'),('mmu_model','68030'),('cpu_cycle_exact','false')):
                    with self.subTest(key=key),patch('aprof_report.machine',return_value={**machine,key:value}):
                        with self.assertRaises(SystemExit):check(capture)
                check(capture)

    def test_observer_never_writes_runtime_or_stops_per_frame(self):
        for selector in (1,6):
            text=observer(selector)
            self.assertNotIn('set g_',text)
            self.assertNotIn('set *',text)
            self.assertNotIn('break *',text)
            self.assertIn('break pop2SceneBoundary',text)
            self.assertEqual(text.count('monitor aprof start'),1)
            self.assertEqual(text.count('monitor aprof stop'),1)
            self.assertIn('SCENE / INCOMPLETE',text)
            self.assertIn('SCENE / RETURN MISMATCH',text)
            self.assertIn('SCENE / DUPLICATE ENTRY',text)


if __name__=='__main__':unittest.main()
