"""Require completed native input/UI probes, original agreement and restored OS."""
import re,struct
from pathlib import Path
from check_core_result import restored
from check_display_scenes import decode_planes,decode_copper
from report_reference import report

def reference(directory):
 directory=directory.resolve();report(directory)
 log=(directory/'session.log').read_text()
 labels={'packing-declined':0,'packing-game-declined':0,'alert-401-yes':1,'alert-404-yes':1,
         'alert-405-yes':1,'alert-405-no':2,'alert-406-yes':1,'alert-406-no':2}
 for label,value in labels.items():assert log.count(f'UI_ROUTINE_COMPLETE {label} result={value}\n')==1,'UI / ORIGINAL DIALOG COMPLETION'
 assert log.count('UI_ROUTINE_COMPLETE about\n')==1,'UI / ORIGINAL ABOUT COMPLETION'
 accepted=re.findall(r'^UI_ROUTINE_COMPLETE copy-accepted flags=([0-9A-F]+)$',log,re.M)
 assert len(accepted)==1,'UI / ORIGINAL PROTECTION COMPLETION'
 rows=re.findall(r'^UI_MENU_RESULT index=(\d+) value=([0-9A-F]+)$',log,re.M)
 assert [int(i) for i,_ in rows]==list(range(15)),'UI / ORIGINAL MENU COMPLETION'
 assert log.count('UI_MENU_COMPLETE calls=19\n')==1,'UI / ORIGINAL MENU CALLS'
 return [int(value,16) for _,value in rows],int(accepted[0],16)

def input_reference(directory):
 directory=directory.resolve();report(directory)
 log=(directory/'session.log').read_text();raw=(directory/'input-samples.bin').read_bytes()
 marker=re.findall(r'^INPUT_FIXTURE_COMPLETE samples=(\d+) events=(\d+)$',log,re.M)
 assert len(marker)==1 and len(raw)==int(marker[0][0])*36 and int(marker[0][1])==9,'INPUT / COMPLETION'
 messages=[];maps=set();previous=0
 for at in range(0,len(raw),36):
  tick=int.from_bytes(raw[at:at+4],'big');assert tick>=previous;previous=tick
  keys=raw[at+4:at+20];maps.add(keys)
  what,message,when,v,h,modifiers=struct.unpack_from('>HIIhhH',raw,at+20)
  assert when<=tick,'INPUT / EVENT TIME'
  if what:assert what==3;messages.append((message,modifiers))
 assert len(messages)==9,'INPUT / ORIGINAL EVENTS'
 return messages,maps

def check_input(run,reference_run):
 messages,maps=input_reference(reference_run)
 log=(run/'gdb-out.log').read_text();restored(run,log)
 assert "INPUT RESTORE vector=1\n" in log,"INPUT / CIA VECTOR RESTORE"
 assert 'INPUT PROBE stage=9 ' in log,'INPUT / VBI COMPLETION'
 rows=re.findall(r'^INPUT EVENT message=([0-9a-f]+) modifiers=([0-9a-f]+) when=(\d+) keys=([0-9a-f]{32})$',log,re.M)
 assert len(rows)==9,'INPUT / NATIVE EVENT COUNT'
 assert [(int(m,16),int(f,16)) for m,f,_,_ in rows]==messages,'INPUT / NATIVE MESSAGES'
 assert all(bytes.fromhex(keys) in maps for _,_,_,keys in rows),'INPUT / NATIVE KEYMAP'
 ticks=[int(t) for _,_,t,_ in rows];assert all(a<b for a,b in zip(ticks,ticks[1:])),'INPUT / VBI TIMESTAMPS'
 assert 'CORE EXCEPTION vector=0 pc=00000000\n' in log
 print('INPUT NATIVE PASS: VBI KeyMap and events match nine original ADB keys')

def check(run,reference_run):
 expected,accepted=reference(reference_run)
 log=(run/'gdb-out.log').read_text();restored(run,log)
 assert "INPUT RESTORE vector=1\n" in log,"INPUT / CIA VECTOR RESTORE"
 assert 'CORE REASON UI / FIXTURE COMPLETE\n' in log,'UI / NATIVE COMPLETION'
 assert 'CORE EXCEPTION vector=0 pc=00000000\n' in log,'UI / NATIVE EXCEPTION'
 assert (run/'ui-keymaps.bin').read_bytes()==bytes(32),'UI / DIRECT AND CACHED GETKEYS'
 original=(reference_run/'traps.txt').read_text().splitlines()[:144]
 native=re.findall(r'^CODE \d+\+[0-9A-F]+ trap=[0-9A-F]+$',log,re.M)[:144]
 assert native==[line.split(' selector=')[0] for line in original],'UI / ORIGINAL STARTUP PREFIX'
 # Explicit D3/D8 and D9 departures: fixed display, compression unavailable.
 assert expected[8]==0x01930004,'UI / ORIGINAL FULL SCREEN POSITIVE CONTROL'
 extra=expected[10:];assert extra[3:]==[0,0],"UI / ORIGINAL UNBOUND SHORTCUTS"
 extra[3:]=[0x01930001,0x01900001]
 expected=expected[:10];expected[8]=0;expected[9]&=~0x90
 expected += [1,accepted&~0x90,0,0,0,0,1,1,1,1,2,1,2]+extra
 window=re.search(r'^UI WINDOW port=([0-9a-f]+) planes=([0-9a-f]+) parity=(\d+) dma=([0-9a-f]+)$',log,re.M)
 assert window and int(window[1],16) and not int(window[4],16)&0x20,'UI / WINDOW OWNERSHIP AND POINTER'
 expected += [int(window[1],16),0,int(window[1],16)]
 assert (run/'ui-window-planes.bin').read_bytes()==bytes([255])*196608,'UI / WINDOW CHROME'
 assert decode_copper((run/'ui-window-copper.bin').read_bytes(),int(window[2],16),int(window[3]))[-3:]==bytes(3),'UI / WINDOW BACKGROUND'
 rows=re.findall(r'^UI RESULT index=(\d+) value=([0-9a-f]+)$',log,re.M)
 assert [int(i) for i,_ in rows]==list(range(31)),'UI / RESULT COVERAGE'
 assert [int(v,16) for _,v in rows]==expected,'UI / PAIRED COMMAND AND DIALOG RESULTS'
 overlays=re.findall(r'^UI OVERLAY id=(\d+) kind=(\d+) planes=([0-9a-f]+) parity=(\d+) dma=([0-9a-f]+)$',log,re.M)
 assert [(int(i),int(k)) for i,k,_,_,_ in overlays]==list(enumerate((600,401,404,405,405,406,406),1)),'UI / OVERLAY COVERAGE'
 for number,kind,base,parity,dma in overlays:
  assert not int(dma,16)&0x20,'UI / SPRITE DMA ENABLED'
  logical=(run/f'ui-{number}-logical.bin').read_bytes();assert len(logical)==307200,'UI / LOGICAL EXTENT'
  expected_pixels=b''.join(logical[y*640:y*640+512] for y in range(384))
  assert set(expected_pixels)=={0,255},'UI / OWNED TEXT COLOURS'
  # All glyphs are within the owned text area. No Macintosh bar/frame/cursor.
  assert all(value==255 for y in range(384) for x,value in enumerate(expected_pixels[y*512:(y+1)*512]) if y<40 or y>=352 or x<32 or x>=496),'UI / CHROME OUTSIDE TEXT AREA'
  assert decode_planes((run/f'ui-{number}-planes.bin').read_bytes())==expected_pixels,'UI / AGA PIXELS'
  palette=decode_copper((run/f'ui-{number}-copper.bin').read_bytes(),int(base,16),int(parity))
  assert palette==b'\xff\xff\xff'+bytes(765),'UI / OWNED PALETTE'
 print('UI NATIVE PASS: 144 original startup calls, menu actions, seven prompts, bypasses, exact AGA output, no sprite pointer')

def startup(run,reference_run):
 reference(reference_run)
 log=(run/'gdb-out.log').read_text();restored(run,log)
 assert 'INPUT RESTORE vector=1\n' in log,'INPUT / CIA VECTOR RESTORE'
 assert 'CORE STOP trap=aaa0 segment=17 offset=0820 count=144\n' in log,'UI / STARTUP STOP'
 assert 'CORE REASON UI / STARTUP CHECKPOINT\n' in log,'UI / STARTUP REASON'
 assert 'CORE EXCEPTION vector=0 pc=00000000\n' in log
 native=re.findall(r'^CODE \d+\+[0-9A-F]+ trap=[0-9A-F]+$',log,re.M)
 original=[line.split(' selector=')[0] for line in (reference_run/'traps.txt').read_text().splitlines()[:144]]
 assert native==original,'UI / ORIGINAL STARTUP PREFIX'
 print('UI STARTUP PASS: 144 original calls; explicit Palette2CTab checkpoint; restored OS')

if __name__=='__main__':
 import argparse
 p=argparse.ArgumentParser();p.add_argument('run',type=Path);p.add_argument('reference',type=Path);p.add_argument('--input',action='store_true');p.add_argument('--startup',action='store_true');a=p.parse_args()
 (startup if a.startup else check_input if a.input else check)(a.run,a.reference)
