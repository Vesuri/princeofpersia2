"""Negative controls for the independent native-bounds checker."""
import copy
from check_c2p_bounds import coverage
r=dict(frame=1,top=0,left=0,nd=1,nc=1,dirty=[(3,17,5,18)],converted=[(3,16,5,48)])
assert coverage(r)==(64,64)
cases=[]
for key,value in [('converted',[]),('converted',[(3,16,5,48)]*2),('converted',[(3,0,5,64)]),('converted',[(3,48,5,80)]),('converted',[(3,17,5,49)]),('dirty',[(5,17,3,18)])]:
    bad=copy.deepcopy(r);bad[key]=value
    bad['nc']=len(bad['converted']);bad['nd']=len(bad['dirty']);cases.append(bad)
for bad in cases:
    try:coverage(bad)
    except AssertionError:pass
    else:raise AssertionError('accepted altered bounds')
print('PASS bounds checker: six missing, overlap, excess, uncovered, alignment and inverted-input controls')
