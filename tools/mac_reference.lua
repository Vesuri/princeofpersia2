-- Pinned System 7.5.5 / Macintosh IIci reference. Original instructions execute;
-- the sole game-state intervention supplies an explicit SetRndSeed argument.
io.stdout:setvbuf('no')
local run=assert(os.getenv('POP2_REFERENCE_RUN'),'REFERENCE / RUN REQUIRED')
local meta=dofile(run..'/metadata.lua')
local mac=dofile('tools/mame_mac_input.lua')
local frame=dofile('tools/mame_mac_frame.lua')
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=assert(manager.machine.debugger,'REFERENCE / DEBUGGER REQUIRED')
local control=os.getenv('POP2_REFERENCE_CONTROL')
local mapper=os.getenv('POP2_MAP') and dofile('tools/mac_map.lua') or nil
local finished,armed,checkpoint=false,false,nil
local mapped={}
local function u(a)return mem:readv_u32(a)end
local function ptr(a)return u(a)&0xffffff end
local function fail(reason)
 if finished then return end
 finished=true;print('FAIL REFERENCE / '..tostring(reason))
 manager.machine:exit();dbg.execution_state='run'
end
local app='Prince of Persia 2'
local cond=string.format('b@0x910==0x%x',#app)
for i=1,#app do cond=cond..string.format(' && b@0x%x==0x%x',0x910+i,app:byte(i))end
-- CurApName changes before Finder has relinquished its A5 world.
cond=cond..' && ((d@0x904&0xffffff)-(d@0x908&0xffffff))==0x56fa'
local function guard(seg,base,offset,length)
 local hex=meta.segments[seg].hex
 for i=offset,offset+(length or 4)-1 do
  local expected=tonumber(hex:sub(i*2+1,i*2+2),16)
  if control=='bad-guard' and seg==17 and i==0x6e42 then expected=expected~1 end
  assert(mem:readv_u8(base+i)==expected,'PROBE BYTES')
 end
end
local function at(seg,base,offset,action,length)
 guard(seg,base,offset,length)
 cpu.debug:bpset(base+offset,cond,action..';g')
 print(string.format('PROBE_ARM seg=%d offset=%04X',seg,offset))
end
local function observe()
 if finished then return end
 if not armed and u(0x28)==meta.dispatcher.address then
  assert(u(meta.dispatcher.address)==tonumber(meta.dispatcher.hex:sub(1,8),16)
   and u(meta.dispatcher.address+4)==tonumber(meta.dispatcher.hex:sub(9,16),16),'DISPATCHER BYTES')
  if mapper then mapper.arm(meta,cond)end
  -- Dynamic return probes capture even the first call immediately after LoadSeg.
  -- SP matches each return to its entry, including Gestalt nested by SysEnvirons.
  cpu.debug:bpset(meta.dispatcher.address,cond..' && (w@(d@(sp+2))==0xa1ad || w@(d@(sp+2))==0xa090 || w@(d@(sp+2))==0xa05c)',
   'logerror "SERVICE pc=%08X trap=%04X selector=%08X sp=%08X\\n",d@(sp+2),w@(d@(sp+2)),d0,sp;bpset d@(sp+2)+0x2,1,{logerror "SERVICE_RETURN pc=%08X d0=%08X a0=%08X sp=%08X env=%08X/%08X/%08X/%08X\\n",pc,d0,a0,sp,d@a0,d@(a0+4),d@(a0+8),d@(a0+0xc);g};g')
  cpu.debug:bpset(meta.dispatcher.address,cond,
   'logerror "TRAP pc=%08X trap=%04X d0=%08X a0=%08X a1=%08X a5=%08X sp=%08X\\n",d@(sp+2),w@(d@(sp+2)),d0,a0,a1,a5,sp;g')
  armed=true;print('DISPATCHER_ARMED')
 end
 local a5=ptr(0x904)
 if a5<0x100000 or a5-ptr(0x908)~=22266 then return end
 for seg,info in pairs(meta.segments)do
  local e=a5+info.jt
  if mem:readv_u16(e)==seg and mem:readv_u16(e+2)==0x4ef9 then
   local base=ptr(e+4)-info.offset
   local key=tostring(seg)..':'..tostring(base)
   if not mapped[key]then
    assert(u(base)==tonumber(info.hex:sub(1,8),16),'SEGMENT HEADER')
    mapped[key]=true
    if mapper then mapper.map(seg,base)end
    print(string.format('MAP seg=%d base=%08X size=%d',seg,base,#info.hex/2))
    if seg==17 then
     at(seg,base,0x6e42,'logerror "SEED old=%04X fixed=1234\\n",w@(sp+4);w@(sp+4)=0x1234',42)
     at(seg,base,0x6e68,'logerror "SEED_RESULT value=%08X\\n",d@(a5-0x18fc)')
     at(seg,base,0x4560,'logerror "PACKING_ENTER\\n"')
    elseif seg==2 then
     at(seg,base,0x3ce8,'logerror "PACK_CHECK flags=%04X/%04X\\n",w@(a5-0x5326),w@(a5-0x5324)',38)
     at(seg,base,0x2f8,'logerror "COPY_ROUTE nis=%04X\\n",w@(a6+0x8)',16)
     at(seg,base,0x1a36,'logerror "COPY_ENTER flags=%04X/%04X\\n",w@(a5-0x56d8),w@(a5-0x4ffe)')
    elseif seg==11 then
     at(seg,base,0x574,'logerror "MIDI_BRANCH input=%02X output=%02X\\n",b@(a6+0xc),b@(a6+0xa)',18)
    elseif seg==15 then
     checkpoint=base+0x416
     guard(seg,base,0x416,4)
     cpu.debug:bpset(checkpoint,cond..' && w@(a6+0x8)==0x6978 && w@(a6+0xa)==0x1',
      'logerror "MILESTONE opening-text-1\\n"')
     print('CHECKPOINT_ARMED')
    end
   end
 end
end
end
emu.register_frame_done(function()
 local ok,err=pcall(observe)
 if not ok then fail(err)end
end)
emu.register_periodic(function()
 if finished then return end
 if mapper then
  local ok,handled=pcall(mapper.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if checkpoint and dbg.execution_state=='stop' and cpu.state.PC.value==checkpoint then
  local ok,err=pcall(function()
   assert(mac.frontmost()==app,'CAPTURE APPLICATION')
   assert(mem:readv_u16(0x15a)==0x755,'SYSTEM VERSION')
   assert(manager.machine.options.entries.ramsize:value()=='8M','RAM CONFIGURATION')
   print('MEMORY configured=8M addressing=24-bit')
   assert(mem:readv_u8(0xcb2)==0,'ADDRESSING MODE')
   if mapper then mapper.finish()end
   frame.rgb(run..'/opening.ppm')
   manager.machine.video:snapshot()
   print('REFERENCE_CAPTURED opening-text-1')
  end)
  if not ok then fail(err);return end
  finished=true;manager.machine:exit();dbg.execution_state='run'
 end
end)
mac.run(function()
 local ok,err=pcall(function()
  assert(mac.wait_for('Finder',function()return mac.frontmost()=='Finder'end,7200),'FINDER')
  mac.wait(900)
  -- Fresh System image: Monitors preferences override PRAM. Apply settings
  -- through the original panel each run, then verify the resulting GDevice.
  mac.menu(20,60);mac.wait(120)
  assert(mac.mouse_to(185,318));mac.click(2);mac.wait(120)
  assert(mac.mouse_to(236,222));mac.click();mac.wait(60)
  assert(mac.mouse_to(114,105));mac.click()
  assert(mac.mouse_to(176,120));mac.click();mac.wait(60)
  assert(mac.mouse_to(196,209));mac.mouse_down();mac.wait(20)
  assert(mac.mouse_to(236,209));mac.wait(20);mac.mouse_up();mac.wait(120)
  assert(mac.mouse_to(114,51));mac.click();mac.wait(300)
  local gd=ptr(ptr(0x8a4));local pm=ptr(ptr(gd+22))
  assert(mem:readv_u16(pm+32)==8 and u(pm)==0xfc000a00,'MAIN DISPLAY')
  print('DISPLAY_CONFIRMED 640x480x8 mdc48')
  assert(mac.mouse_to(103,241));mac.click();mac.wait(60)
  assert(mac.mouse_to(598,98));mac.click(2);mac.wait(400)
  assert(mac.mouse_to(103,113));mac.wait(60)
  for i=1,2 do mac.mouse_down();mac.wait(12);mac.mouse_up();mac.wait(12)end
  assert(mac.wait_for('game',function()return mac.frontmost()==app end,1800),'APPLICATION')
  mac.wait(6000);error('MILESTONE NOT REACHED')
 end)
 if not ok then fail(err)end
end)
dbg.execution_state='run'
