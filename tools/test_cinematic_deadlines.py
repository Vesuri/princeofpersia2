"""Controls for the original adaptive deadline interpretation."""
import copy
import unittest
from cinematic_deadlines import feedback


class Feedback(unittest.TestCase):
    def test_ahead_on_time_behind_and_zero_floor(self):
        rows=[dict(progress=i+1,late_ticks=lag,delay=delay)
              for i,(lag,delay) in enumerate([(-2,0),(0,1),(2,1),(3,0),(0,0)])]
        self.assertEqual(feedback(rows)['feedback_transitions_checked'],4)
        self.assertEqual(feedback(rows)['signed_lag_range'],[-2,3])
        for index in range(1,len(rows)):
            bad=copy.deepcopy(rows);bad[index]['delay']+=1
            with self.assertRaises(SystemExit):feedback(bad)
        bad=copy.deepcopy(rows);bad[2]['progress']+=1
        with self.assertRaises(SystemExit):feedback(bad)
        with self.assertRaises(SystemExit):feedback([])


if __name__=='__main__':unittest.main()
