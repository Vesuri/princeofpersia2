local freed=os.getenv('POP2_FREED_PROBE') and dofile('tools/mac_freed_handles.lua') or nil
local heapFixture=dofile('tools/mac_heap.lua')
local rectangles=dofile('tools/mac_rectangles.lua')
local saveRect=os.getenv('POP2_SAVE_RECT_FIXTURE') and dofile('tools/mac_save_rect.lua') or nil
local wrappers=os.getenv('POP2_WRAPPER_FIXTURE') and dofile('tools/mac_wrapper_ccr.lua') or nil
-- Read-only original-call contracts for native startup integration.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local meta,pending,bp,thePort,callCondition;local seen={};local serial=0
local function ptr(a)return mem:readv_u32(a)&0xffffff end
local function dump(label,address,size)
 if address<0x1000 or address+size>=0x800000 then return end
 local f=assert(io.open(run..'/integration-'..serial..'-'..label..'.bin','wb'))
 local b={};for i=0,size-1 do b[#b+1]=string.char(mem:readv_u8(address+i))end
 f:write(table.concat(b));f:close()
 print(string.format('INTEGRATION_DUMP id=%d label=%s address=%08X size=%d',serial,label,address,size))
end
local function bitmap(address)
 if mem:readv_u16(address+4)&0xc000==0xc000 then return ptr(ptr(address))end
 return address
end
local function capture(phase,p)
 local sp=cpu.state.SP.value+(phase=='before' and 8 or 0)
 dump(phase..'-stack',sp,64)
 if os.getenv('POP2_TITLE_COPY') then
  if phase=='before' then
   p.copySource=bitmap(ptr(sp+18));p.copyDestination=bitmap(ptr(sp+14))
   dump('copy-from',ptr(sp+10),8);dump('copy-to',ptr(sp+6),8)
  end
  for _,side in ipairs({'Source','Destination'})do
   local map=p['copy'..side];dump(phase..'-copy-'..side..'-map',map,50)
   local pixels=ptr(map);if mem:readv_u16(map+14)==2 then pixels=ptr(pixels)end
   local height=mem:readv_u16(map+10)-mem:readv_u16(map+6)
   dump(phase..'-copy-'..side..'-pixels',pixels,(mem:readv_u16(map+4)&0x3fff)*height)
  end
 end
 if os.getenv('POP2_NEWGWORLD') then
  if phase=='before' then
   p.worldOutput=ptr(sp+18);p.worldBounds=ptr(sp+12)
   dump('world-bounds',p.worldBounds,8)
  else
   local port=ptr(p.worldOutput)
   assert(port~=0,'WORLD ALLOCATION FAILED')
   dump('world-port',port,108);dump('world-map',ptr(ptr(port+2)),50)
  end
 end
 if p.trap==0xa86e and phase=='before' then thePort=ptr(sp)end
 if thePort and ptr(thePort)~=0 then
  local port=ptr(thePort);dump(phase..'-port',port,108)
  dump(phase..'-pixmap',ptr(ptr(port+2)),50)
  dump(phase..'-vis',ptr(ptr(port+24)),10)
  dump(phase..'-clip',ptr(ptr(port+28)),10)
 end
 local registers={}
 for _,prefix in ipairs({'D','A'})do for i=0,7 do
  local name=prefix..i;registers[#registers+1]=string.format('%s=%08X',name,cpu.state[name].value)
 end end
 print('INTEGRATION_REGS '..phase..' id='..serial..' '..table.concat(registers,' '))
 if phase=='before' then
  p.pointers={cpu.state.A0.value&0xffffff,cpu.state.A1.value&0xffffff}
  for i=0,14 do p.pointers[#p.pointers+1]=ptr(sp+2*i)end
 end
 for i,a in ipairs(p.pointers)do
  dump(phase..'-pointer-'..i,a,64)
  if a>=0x1000 and a<0x800000 then dump(phase..'-indirect-'..i,ptr(a),64)end
 end
end
function M.arm(metadata,condition)
 meta=metadata;callCondition=condition
 if os.getenv('POP2_TITLE_COPY') then
  bp=cpu.debug:bpset(meta.dispatcher.address,condition..' && (w@(d@(sp+2))==0xa8ec || w@(d@(sp+2))==0xa86e)','logerror "INTEGRATION_STOP\\n"')
  return
 end
 if os.getenv('POP2_NEWGWORLD') then
  bp=cpu.debug:bpset(meta.dispatcher.address,condition..' && w@(d@(sp+2))==0xab1d && (d0&0xffff)==0','logerror "INTEGRATION_STOP\\n"')
  return
 end
 if os.getenv("POP2_INTEGRATION_CLEANUP") then
  bp=cpu.debug:bpset(meta.dispatcher.address,condition.." && w@(d@(sp+2))==0xa023",'logerror "INTEGRATION_STOP\\n"')
  return
 end
 bp=cpu.debug:bpset(meta.dispatcher.address,condition..' && w@(d@(sp+2))!=0xa1ad && w@(d@(sp+2))!=0xa090 && w@(d@(sp+2))!=0xa05c && (d@(sp+2)&0xffffff)>=0x100000 && (d@(sp+2)&0xffffff)<0x800000','logerror "INTEGRATION_STOP\\n"')
end
function M.opening()
 assert(not pending,'INTEGRATION PENDING AT OPENING')
 if not os.getenv("POP2_INTEGRATION_CLEANUP") and not os.getenv("POP2_NEWGWORLD") and not os.getenv("POP2_TITLE_COPY") then cpu.debug:bpclear(bp);bp=nil end
end
local nilProbe,nilPC,nilStage
local equalBase,equalIndex
local equalRects={{0,0,10,20},{0,0,10,20},{0,0,0,0},{10,20,0,0},{-32768,-1,32767,0},{0,1,10,20}}
local function nextEqual()
 local a=(equalIndex-1)//#equalRects+1;local b=(equalIndex-1)%#equalRects+1
 for n,r in ipairs({equalRects[a],equalRects[b]})do
  for i,v in ipairs(r)do mem:writev_u16(equalBase+256+(n-1)*8+(i-1)*2,v&65535)end
 end
 local sp=equalBase+1024
 mem:writev_u32(sp,equalBase+256);mem:writev_u32(sp+4,equalBase+264);mem:writev_u16(sp+8,0x7f5a)
 cpu.state.SP.value=sp;cpu.state.PC.value=equalBase;dbg.execution_state='run'
end
function M.startNil()
 assert(not pending,'INTEGRATION PENDING AT FIXTURE')
 local code=(cpu.state.SP.value&0xffffff)-8192
 assert(code>0x100000,'INTEGRATION FIXTURE EXTENT')
 if freed then freed.start(code,M.finish);return end
 if saveRect then saveRect.start(code,M.finish);return end
 if wrappers then wrappers.start(code,M.finish);return end
 if os.getenv('POP2_HEAP_PROBE') then heapFixture.start(code,M.finish);return end
 if os.getenv('POP2_RECTANGLE_PROBE') then rectangles.start(code,M.finish);return end
 if os.getenv('POP2_EQUAL_RECT_PROBE') then
  equalBase=code;equalIndex=1
  mem:writev_u16(code,0xa8a6);mem:writev_u16(code+2,0x4e71)
  cpu.debug:bpset(code+2,'1','logerror "EQUAL_RECT_STOP\\n"');nextEqual();return
 end
 -- NewHandle(16), DisposeHandle(valid), then DisposeHandle(nil).
 local program=string.pack('>I2I2I2I2I2I2I2',0x7010,0xa122,0xa023,0x4e71,0x91c8,0xa023,0x4e71)
 if os.getenv('POP2_HANDLE_COPY_PROBE') then
  program=string.pack('>I2I2I2I2I2I2I2I2I2I2',0x7010,0xa122,0x4e71,0x2448,0x70c0,0xa06a,0xa9e1,0x4e71,0xa025,0x4e71)
 elseif os.getenv('POP2_HANDLE_STATE_PROBE') then
  -- Valid unlock, retain its master address, dispose, then unlock and purge
  -- the freed master without allocating anything between the calls.
  program=string.pack('>I2I2I2I2I2I2I2I2I2I2I2',0x7010,0xa122,0xa02a,0x4e71,0x2448,0xa023,0x204a,0xa02a,0x4e71,0xa049,0x4e71)
 end
 for i=1,#program do mem:writev_u8(code+i-1,program:byte(i))end
 for i=1,#program do assert(mem:readv_u8(code+i-1)==program:byte(i),'INTEGRATION FIXTURE BYTES')end
 nilPC=code+(os.getenv('POP2_HANDLE_COPY_PROBE') and 4 or 6);nilStage=1
 nilProbe=cpu.debug:bpset(nilPC,'1','logerror "INTEGRATION_NIL_STOP\\n"')
 cpu.state.PC.value=code;dbg.execution_state='run'
end
local fixedCases,fixedIndex,fixedPC,fixedSP,fixedProbe
local function nextFixed()
 local c=fixedCases[fixedIndex]
 local code=fixedSP-8192
 -- Authored caller: allocate Fixed result, push two Fixed arguments, trap.
 local program=string.pack('>I2I2I4I2I4I2I2',0x598f,0x2f3c,c[2],0x2f3c,c[3],c[1],0x4e71)
 for i=1,#program do mem:writev_u8(code+i-1,program:byte(i))end
 for i=1,#program do assert(mem:readv_u8(code+i-1)==program:byte(i),'FIXED FIXTURE BYTES')end
 fixedPC=code+#program-2
 fixedProbe=cpu.debug:bpset(fixedPC,'1','logerror "INTEGRATION_FIXED_STOP\\n"')
 cpu.state.SP.value=fixedSP;cpu.state.PC.value=code;dbg.execution_state='run'
end
function M.startFixed()
 assert(not pending,'INTEGRATION PENDING AT FIXED FIXTURE')
 if bp then cpu.debug:bpclear(bp);bp=nil end
 fixedSP=cpu.state.SP.value&0xffffff;fixedCases={};fixedIndex=1
 local pairs={{0,0},{0x10000,0},{0xffff0000,0},{0x10000,0x10000},
  {0xffff0000,0x10000},{0x10000,0xffff0000},{0xffff0000,0xffff0000},
  {1,0x8000},{0xffffffff,0x8000},{1,0xffff8000},{0xffffffff,0xffff8000},
  {1,0x20000},{0xffffffff,0x20000},{1,0xfffe0000},{0xffffffff,0xfffe0000},
  {0x7fffffff,0x7fffffff},{0x80000000,0x7fffffff},{0x80000000,0x80000000},
  {0x7fffffff,1},{0x80000000,1},{0x80000000,0xffffffff},
  {0x12345678,0x30000},{0xedcba988,0x30000},{0x12345678,0xfffd0000},
  {0x10000,0x30000},{0xffff0000,0x30000},{0x10001,0x18000},{0xfffeffff,0x18000},
  {2,0x30000},{0xfffffffe,0x30000},{2,0xfffd0000},{0xfffffffe,0xfffd0000},
  {3,0x20000},{0xfffffffd,0x20000},{5,0x20000},{0xfffffffb,0x20000},
  {7,0x20000},{0xfffffff9,0x20000},{0x7fffffff,0x30000},{0x80000000,0x30000}}
 for _,trap in ipairs({0xa868,0xa84d})do for _,p in ipairs(pairs)do fixedCases[#fixedCases+1]={trap,p[1],p[2]}end end
 nextFixed()
end
function M.poll()
 if freed and freed.poll()then return true end
 if saveRect and saveRect.poll()then return true end
 if wrappers and wrappers.poll()then return true end
 if heapFixture.poll()then return true end
 if rectangles.poll()then return true end
 if not meta or dbg.execution_state~='stop' then return false end
 local pc=cpu.state.PC.value
 if fixedPC and pc==fixedPC then
  local c=fixedCases[fixedIndex]
  assert(cpu.state.SP.value&0xffffff==fixedSP-4,'FIXED RESULT STACK')
  print(string.format('FIXED_RESULT trap=%04X a=%08X b=%08X result=%08X',c[1],c[2],c[3],mem:readv_u32(fixedSP-4)))
  cpu.debug:bpclear(fixedProbe);fixedIndex=fixedIndex+1
  if fixedIndex>#fixedCases then
   print('FIXED_FIXTURE_COMPLETE calls='..#fixedCases);fixedPC=nil;M.finish();manager.machine:exit()
  else nextFixed()end
  return true
 end
 if equalBase and pc==equalBase+2 then
  local sp=cpu.state.SP.value&0xffffff
  assert(sp==equalBase+1032,'EQUAL RECT STACK')
  local a=(equalIndex-1)//#equalRects+1;local b=(equalIndex-1)%#equalRects+1
  local expected=true;for i=1,4 do if equalRects[a][i]~=equalRects[b][i]then expected=false end end
  assert(mem:readv_u8(sp)==(expected and 1 or 0) and mem:readv_u8(sp+1)==0x5a,'EQUAL RECT BOOLEAN SLOT')
  for n,r in ipairs({equalRects[a],equalRects[b]})do for i,v in ipairs(r)do assert(mem:readv_u16(equalBase+256+(n-1)*8+(i-1)*2)==v&65535,'EQUAL RECT INPUT PRESERVED')end end
  print(string.format('EQUAL_RECT_RESULT a=%d b=%d result=%d pad=%02X',a,b,mem:readv_u8(sp),mem:readv_u8(sp+1)))
  equalIndex=equalIndex+1
  if equalIndex>36 then print('EQUAL_RECT_COMPLETE calls=36');M.finish();manager.machine:exit();dbg.execution_state='run'else nextEqual()end
  return true
 end
 if nilPC and pc==nilPC and os.getenv('POP2_HANDLE_COPY_PROBE') then
  local h=cpu.state.A0.value&0xffffff
  print(string.format('HANDLE_COPY_FIXTURE stage=%d d0=%08X memerr=%04X a0=%08X master=%08X sr=%04X',nilStage,cpu.state.D0.value,mem:readv_u16(0x220),h,mem:readv_u32(h),cpu.state.SR.value))
  if nilStage==1 then
   assert(h~=0 and mem:readv_u16(0x220)==0,'COPY FIXTURE ALLOCATION')
   for i=0,15 do mem:writev_u8(ptr(h)+i,(i*37+11)%256)end
  elseif nilStage==2 then
   cpu.state.SR.value=(cpu.state.SR.value&0xffe0)|31
   mem:writev_u16(0x220,0x1234)
  elseif nilStage==3 then
   local source=cpu.state.A2.value&0xffffff
   assert(source~=h,'COPY FIXTURE DISTINCT HANDLE')
   print(string.format('HANDLE_COPY_SOURCE handle=%08X master=%08X',source,mem:readv_u32(source)))
   for i=0,15 do assert(mem:readv_u8(ptr(source)+i)==(i*37+11)%256 and mem:readv_u8(ptr(h)+i)==(i*37+11)%256,'COPY FIXTURE CONTENTS')end
  else assert(cpu.state.D0.value==16,'COPY FIXTURE SIZE')end
  cpu.debug:bpclear(nilProbe)
  if nilStage<4 then
   nilPC=nilPC+(nilStage==1 and 8 or nilStage==2 and 2 or 4);nilStage=nilStage+1
   nilProbe=cpu.debug:bpset(nilPC,'1','logerror "INTEGRATION_HANDLE_COPY_STOP\\n"')
  else print('HANDLE_COPY_FIXTURE_COMPLETE');M.finish();manager.machine:exit()end
  dbg.execution_state='run';return true
 end
 if nilPC and pc==nilPC and os.getenv('POP2_HANDLE_STATE_PROBE') then
  local h=cpu.state.A0.value&0xffffff
  print(string.format('HANDLE_STATE_FIXTURE stage=%d d0=%08X memerr=%04X a0=%08X master=%08X sr=%04X',nilStage,cpu.state.D0.value,mem:readv_u16(0x220),h,mem:readv_u32(h),cpu.state.SR.value))
  cpu.debug:bpclear(nilProbe)
  if nilStage<3 then
   nilPC=nilPC+(nilStage==1 and 10 or 4);nilStage=nilStage+1
   nilProbe=cpu.debug:bpset(nilPC,'1','logerror "INTEGRATION_HANDLE_STATE_STOP\\n"')
  else print('HANDLE_STATE_FIXTURE_COMPLETE');M.finish();manager.machine:exit()end
  dbg.execution_state='run';return true
 end
 if nilPC and pc==nilPC then
  print(string.format('DISPOSE_FIXTURE stage=%d d0=%08X memerr=%04X a0=%08X',nilStage,cpu.state.D0.value,mem:readv_u16(0x220),cpu.state.A0.value))
  cpu.debug:bpclear(nilProbe)
  if nilStage==1 then
   assert(cpu.state.D0.value&65535==0 and mem:readv_u16(0x220)==0,'DISPOSE VALID CONTROL')
   nilPC=nilPC+6;nilStage=2
   nilProbe=cpu.debug:bpset(nilPC,'1','logerror "INTEGRATION_NIL_STOP\\n"')
  else
   print('DISPOSE_FIXTURE_COMPLETE');M.finish();manager.machine:exit()
  end
  dbg.execution_state='run';return true
 end
 if pending and pc==pending.returnPC then
  cpu.debug:bpclear(pending.probe);capture('after',pending);pending=nil
  if os.getenv('POP2_TITLE_COPY') and not os.getenv('POP2_TITLE_COPY_ALL') then cpu.debug:bpclear(bp);bp=nil end
  dbg.execution_state='run';return true
 end
 if not bp or pc~=meta.dispatcher.address then return false end
 local caller=ptr(cpu.state.SP.value+2);local a5=cpu.state.A5.value&0xffffff
 if os.getenv('POP2_TITLE_COPY') and mem:readv_u16(caller)==0xa86e then
  thePort=ptr(cpu.state.SP.value+8);dbg.execution_state='run';return true
 end
 local segment,offset
 for seg,info in pairs(meta.segments)do
  local e=a5+info.jt
  if mem:readv_u16(e)==seg and mem:readv_u16(e+2)==0x4ef9 then
   local base=ptr(e+4)-info.offset
   if caller>=base and caller<base+#info.hex/2 then segment=seg;offset=caller-base;break end
  end
 end
 if not segment then dbg.execution_state='run';return true end
 if os.getenv('POP2_TITLE_COPY') then
  local sp=cpu.state.SP.value+8;local selected=false
  for _,offset in ipairs({14,18})do
   local map=bitmap(ptr(sp+offset))
   if mem:readv_u16(map+10)-mem:readv_u16(map+6)==384 and mem:readv_u16(map+12)-mem:readv_u16(map+8)==510 then selected=true end
  end
  if not selected then dbg.execution_state='run';return true end
 end
 if serial==0 then
 local drive=ptr(0x30a);local count=0
 while drive~=0 do
  assert(count<16,'INTEGRATION DRIVE QUEUE CYCLE')
  dump('drive-'..count,drive-4,32)
  local ref=mem:readv_u16(drive+8);local unit=(65535-ref)*4+ptr(0x11c)
  dump('unit-'..count,unit,4)
  local dce=ptr(ptr(unit));dump('dce-'..count,dce,48)
  local driver=ptr(dce);if mem:readv_u16(dce+4)&0x40~=0 then driver=ptr(driver)end
  dump('driver-'..count,driver,64)
  drive=ptr(drive);count=count+1
 end
 end
 local trap=mem:readv_u16(caller)
 print(string.format('TRAP pc=%08X trap=%04X d0=%08X a0=%08X a1=%08X a5=%08X sp=%08X',caller,trap,cpu.state.D0.value,cpu.state.A0.value,cpu.state.A1.value,cpu.state.A5.value,cpu.state.SP.value))
 local key=segment..':'..offset
 if seen[key] and not os.getenv("POP2_NEWGWORLD") and not os.getenv("POP2_TITLE_COPY_ALL") and not (os.getenv("POP2_INTEGRATION_CLEANUP") and trap==0xa023) then dbg.execution_state='run';return true end
 assert(not pending,'INTEGRATION NESTED CALL')
 seen[key]=true;serial=serial+1
 print(string.format('INTEGRATION_CALL id=%d segment=%d offset=%04X trap=%04X',serial,segment,offset,trap))
 local p={returnPC=caller+2,trap=trap};capture('before',p)
 p.probe=cpu.debug:bpset(p.returnPC,'1','logerror "INTEGRATION_STOP\\n"');pending=p
 dbg.execution_state='run';return true
end
function M.finish()
 assert(not pending and serial>0,'INTEGRATION INCOMPLETE')
 print(string.format('INTEGRATION_COMPLETE calls=%d',serial))
end
return M
