"""Compare complete output at all 82 completed opening game frames."""
import re,sys,struct
from pathlib import Path
from aprof_report import check_run,frames
from check_display_scenes import decode_planes,decode_copper

def check(root):
 check_run(root)
 log=(root/'gdb-out.log').read_text()
 matches=re.findall(r'^FRAME_BOUNDARY index=(\d+) game=(\d+) crop=(\d+),(\d+) pending=([01]) pixels=([0-9a-f]+)$',log,re.M)
 record=frames(root)
 assert len(matches)==len(record)==82,'frame extent'
 result=[]
 for i,(n,game,top,left,pending,base) in enumerate(matches,1):
  assert int(n)==i and int(game)==record[i-1]['counter'],'frame order'
  top,left=int(top),int(left);assert 0<=top<=96 and 0<=left<=128,'viewport extent'
  chunky=(root/f'frame-{i}-chunky.bin').read_bytes();assert len(chunky)==307200
  expected=b''.join(chunky[(y+top)*640+left:(y+top)*640+left+512] for y in range(384))
  planes=decode_planes((root/f'frame-{i}-planes.bin').read_bytes());assert planes==expected,('plane mismatch',i)
  rgb=decode_copper((root/f'frame-{i}-copper.bin').read_bytes(),int(base,16),0)
  colors=(root/f'frame-{i}-colors.bin').read_bytes();assert len(colors)==2056
  kid=(root/f'frame-{i}-kid.bin').read_bytes();assert len(kid)==64 and kid[:62].hex()==record[i-1]['kid'],'player record'
  result.append((int(game),top,left,expected,rgb,colors,kid))
 print('PASS complete frame-boundary output:',root,'82 frames;',sum(int(r[4])==0 for r in matches),'without pending publication',flush=True)
 return result

if __name__=='__main__':
 import argparse
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('run',type=Path);parser.add_argument('--compare',type=Path)
 args=parser.parse_args();first=check(args.run)
 if args.compare:
  second=check(args.compare)
  for i,(a,b) in enumerate(zip(first,second),1):assert a==b,('frame differential',i,[j for j,(x,y) in enumerate(zip(a,b)) if x!=y])
  print('PASS all 82 frame boundaries: viewport pixels, copper palette, colour table and complete 64-byte player record match.')
