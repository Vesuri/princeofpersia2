-- Pinned System 7.5.5 / Macintosh IIci reference. Original instructions execute;
-- the baseline supplies an explicit SetRndSeed argument. Optional profiles
-- add labeled service/routine fixtures after the verified opening checkpoint.
io.stdout:setvbuf('no')
local run=assert(os.getenv('POP2_REFERENCE_RUN'),'REFERENCE / RUN REQUIRED')
local meta=dofile(run..'/metadata.lua')
local mac=dofile('tools/mame_mac_input.lua')
local frame=dofile('tools/mame_mac_frame.lua')
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=assert(manager.machine.debugger,'REFERENCE / DEBUGGER REQUIRED')
local control=os.getenv('POP2_REFERENCE_CONTROL')
local mapper=os.getenv('POP2_MAP') and dofile('tools/mac_map.lua') or nil
local resourceCalls=os.getenv('POP2_RESOURCES') and dofile('tools/mac_resource_calls.lua') or nil
local files=os.getenv('POP2_FILES') and dofile('tools/mac_files.lua') or nil
local fileRoutines=os.getenv('POP2_FILE_ROUTINES') and dofile('tools/mac_file_routines.lua') or nil
local uiRoutines=os.getenv('POP2_UI_ROUTINES') and dofile('tools/mac_ui_routines.lua') or nil
local inputProbe=os.getenv('POP2_INPUT_PROBE') and dofile('tools/mac_input_probe.lua') or nil
local audio=os.getenv('POP2_AUDIO') and dofile('tools/mac_audio.lua') or nil
local integration=(os.getenv('POP2_INTEGRATION') or os.getenv('POP2_NEWGWORLD') or os.getenv('POP2_TITLE_COPY')) and dofile('tools/mac_integration.lua') or nil
local timing=os.getenv('POP2_TIMING_TM') and dofile('tools/mac_time_manager.lua') or os.getenv('POP2_TIMING_VBL') and dofile('tools/mac_timing.lua') or nil
local gameplay=os.getenv('POP2_GAMEPLAY') and dofile('tools/mac_gameplay.lua') or nil
local inputStage=false
local ui=os.getenv('POP2_UI') and dofile('tools/mac_ui.lua') or nil
local display=os.getenv('POP2_DISPLAY') and dofile('tools/mac_display.lua') or nil
local finished,armed,checkpoint=false,false,nil
local checkpointProbe,trapTraceProbe
local videoTransfer=os.getenv('POP2_VIDEO_TRANSFER') and dofile('tools/mac_video_transfer.lua') or nil
local displayRoute=os.getenv("POP2_DISPLAY_ROUTE")
local routeStage,routeProbes,routePoints=nil,{},{}
-- A separate renderer fixture pairs the observed native frame/cue inputs.
-- It never supplies evidence for natural startup or gameplay.
local titleState=tonumber(os.getenv('POP2_TITLE_STATE'))
local titleStateApplied=false
if titleState then assert(titleState>=4 and titleState<=128 and titleState%1==0 and displayRoute and not audio,'TITLE STATE CONFIGURATION')end
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
  if resourceCalls then resourceCalls.arm(meta,cond)end
  if files then files.arm(meta,cond)end
  if mapper then mapper.arm(meta,cond)end
  if display then display.arm(meta,cond)end
  if integration then integration.arm(meta,cond)end
  if ui then ui.arm(meta,cond)end
  -- Dynamic return probes capture even the first call immediately after LoadSeg.
  -- SP matches each return to its entry, including Gestalt nested by SysEnvirons.
  cpu.debug:bpset(meta.dispatcher.address,cond..' && (w@(d@(sp+2))==0xa1ad || w@(d@(sp+2))==0xa090 || w@(d@(sp+2))==0xa05c)',
   'logerror "SERVICE pc=%08X trap=%04X selector=%08X sp=%08X\\n",d@(sp+2),w@(d@(sp+2)),d0,sp;bpset d@(sp+2)+0x2,1,{logerror "SERVICE_RETURN pc=%08X d0=%08X a0=%08X sp=%08X env=%08X/%08X/%08X/%08X\\n",pc,d0,a0,sp,d@a0,d@(a0+4),d@(a0+8),d@(a0+0xc);g};g')
  trapTraceProbe=cpu.debug:bpset(meta.dispatcher.address,cond,
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
    if gameplay then gameplay.map(seg,base,guard,cond)end
    if audio then audio.map(seg,base,guard,meta)end
    print(string.format('MAP seg=%d base=%08X size=%d',seg,base,#info.hex/2))
    -- Timing observations use guest ticks and original routine boundaries.
    -- They neither alter deadlines nor inject game-state changes.
    if os.getenv('POP2_TIMING_TRACE') then
     if seg==2 then
      at(seg,base,0x68b2,'logerror "TIMING_FRAME ticks=%08X result=%04X room=%04X state=%04X\\n",d@0x16a,d0&0xffff,w@(a5-0x4fc2),w@(a5-0x4fcc)',2)
     elseif seg==3 then
      at(seg,base,0x3330,'logerror "TIMING_DRAW_BEGIN ticks=%08X frame=%04X world=%02X\\n",d@0x16a,w@(a5-0x349a),b@(a5-0x521e)')
      at(seg,base,0x338c,'logerror "TIMING_DRAW_END ticks=%08X frame=%04X world=%02X\\n",d@0x16a,w@(a5-0x349a),b@(a5-0x521e)',2)
     elseif seg==17 then
      at(seg,base,0x3172,'logerror "TIMING_SET ticks=%08X timer=%04X delay=%04X\\n",d@0x16a,w@(a6+8),w@(a6+0xc)')
      at(seg,base,0x31a2,'logerror "TIMING_WAIT ticks=%08X timer=%04X interrupted=%04X\\n",d@0x16a,w@(a6+8),d3&0xffff')
      at(seg,base,0x31da,'logerror "TIMING_WAIT_NOPOLL ticks=%08X timer=%04X\\n",d@0x16a,w@(a6+8)')
     end
    end
    if os.getenv('POP2_ROUTE_TRACE') then
     if seg==15 then
      at(seg,base,0x15e,'logerror "ROUTE_NIS_RETURN kind=%04X result=%04X ticks=%08X\\n",w@(a6+8),d4&0xffff,d@0x16a')
     elseif seg==2 then
      at(seg,base,0x3be,'logerror "ROUTE_PLAYNIS_RETURN kind=%04X result=%04X ticks=%08X\\n",w@(a6+8),d4&0xffff,d@0x16a')
     end
    end
    if os.getenv('POP2_HANDLE_TRACE') and seg==3 then
     at(seg,base,0x5f9a,'logerror "PURGE_BEFORE handle=%08X body=%08X ticks=%08X\\n",a0,d@a0,d@0x16a')
     at(seg,base,0x5f9c,'logerror "PURGE_AFTER handle=%08X d0=%08X memerr=%04X ticks=%08X\\n",a0,d0,w@0x220,d@0x16a')
    end
    if os.getenv('POP2_TIMER_TRACE') and seg==22 then
     at(seg,base,0x6b90,'logerror "TIMER_INNER_BEFORE task=%08X d0=%08X type=%04X count=%08X wake=%08X ticks=%08X\\n",a0,d0,w@(a0+4),d@(a0+10),d@(a0+14),d@0x16a')
     at(seg,base,0x6b92,'logerror "TIMER_INNER_AFTER task=%08X d0=%08X type=%04X count=%08X wake=%08X ticks=%08X\\n",a0,d0,w@(a0+4),d@(a0+10),d@(a0+14),d@0x16a')
     at(seg,base,0x7028,'logerror "TIMER_OUTER_BEFORE task=%08X d0=%08X type=%04X count=%08X wake=%08X ticks=%08X\\n",a0,d0,w@(a0+4),d@(a0+10),d@(a0+14),d@0x16a')
     at(seg,base,0x702a,'logerror "TIMER_OUTER_AFTER task=%08X d0=%08X type=%04X count=%08X wake=%08X ticks=%08X\\n",a0,d0,w@(a0+4),d@(a0+10),d@(a0+14),d@0x16a')
     at(seg,base,0x7026,'logerror "MIDI_REPRIME interval=%08X measured=%08X next=%08X ticks=%08X cue=%04X\\n",d@(a3+0x1e),d4,d5,d@0x16a,w@(a5-0x1f6e)')
    end
    if os.getenv('POP2_TITLE_TRACE') and seg==15 then
     at(seg,base,0x6ff2,'logerror "TITLE_CALL phase=%04X cue=%04X ticks=%08X\\n",w@(a6+8),w@(a5-0x1f6e),d@0x16a')
     at(seg,base,0x70ca,'logerror "TITLE_LOGO cue=%04X ticks=%08X\\n",w@(a5-0x1f6e),d@0x16a')
     at(seg,base,0x7164,'logerror "TITLE_RETURN phase=%04X cue=%04X result=%04X ticks=%08X\\n",w@(a6+8),w@(a5-0x1f6e),d3&0xffff,d@0x16a')
    end
    if displayRoute and (seg==15 or seg==2) then
     local off=seg==15 and 0x6fec or 0x6056
     if seg==15 then
      guard(seg,base,0x7060,4)
      routePoints[base+0x7060]='title-start'
      routeProbes[base+0x7060]=cpu.debug:bpset(base+0x7060,cond,'logerror "DISPLAY_ROUTE_STOP\\n"')
     end
     guard(seg,base,off,4)
     routePoints[base+off]=seg==15 and 'title' or 'first-room'
     routeProbes[base+off]=cpu.debug:bpset(base+off,seg==2 and cond..' && (d4&0xffff)==0xfffe' or cond..' && w@(sp+4)==0x20 && w@(a5-0x1f6e)==0x72','logerror "DISPLAY_ROUTE_STOP\\n"')
     if seg==15 and titleState then
      cpu.debug:bpclear(routeProbes[base+off]);routePoints[base+off]='title-state'
      routeProbes[base+off]=cpu.debug:bpset(base+off,cond..string.format(' && (w@(sp+4)==0x%X || w@(sp+4)==0x%X)',titleState-1,titleState),'logerror "TITLE_STATE_STOP\\n"')
     end
    end
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
     checkpointProbe=cpu.debug:bpset(checkpoint,cond..' && w@(a6+0x8)==0x6978 && w@(a6+0xa)==0x1',
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
 if gameplay then
  local ok,handled=pcall(gameplay.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if timing then
  local ok,handled=pcall(timing.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if audio then
  local ok,handled=pcall(audio.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if videoTransfer then
  local ok,handled=pcall(videoTransfer.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if uiRoutines then
  local ok,handled=pcall(uiRoutines.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if inputProbe then
  local ok,handled=pcall(inputProbe.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if ui then
  local ok,handled=pcall(ui.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if integration then
  local ok,handled=pcall(integration.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if display then
  local ok,handled=pcall(display.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if fileRoutines then
  local ok,handled=pcall(fileRoutines.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if resourceCalls then
  local ok,handled=pcall(resourceCalls.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if files then
  local ok,handled=pcall(files.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if mapper then
  local ok,handled=pcall(mapper.poll)
  if not ok then fail(handled);return end
  if handled then return end
 end
 if displayRoute and dbg.execution_state=='stop' and routePoints[cpu.state.PC.value] then
  local pc=cpu.state.PC.value;local label=routePoints[pc]
  local ok,err=pcall(function()
   if label=='title-state' then
    local phase=mem:readv_u16(cpu.state.SP.value+4);local address=(cpu.state.A5.value&0xffffff)-0x1f6e
    if phase==titleState-1 then
     assert(not titleStateApplied and mem:readv_u16(address)==100,'TITLE STATE INITIAL CUE')
     print(string.format('TITLE_STATE_INPUT frame=%d old=100 cue=101',phase))
     mem:writev_u16(address,101);titleStateApplied=true;dbg.execution_state='run';return
    end
    assert(titleStateApplied and phase==titleState and mem:readv_u16(address)==114,'TITLE STATE LOGO RETURN')
   end
   frame.rgb(run..'/'..label..'.ppm');frame.indexed(run..'/'..label..'.indices');frame.palette(run..'/'..label..'.rgb');frame.logical(run..'/'..label..'.clut')
   if label=='first-room' then
    local a5=cpu.state.A5.value&0xffffff
    local f=assert(io.open(run..'/first-room-a5.bin','wb'))
    for i=-22266,-1 do f:write(string.char(mem:readv_u8(a5+i)))end
    f:close()
    local context=ptr(a5-0x5014)
    assert(context>=0x100000 and context+0x2200<0x800000,'FIRST ROOM CONTEXT')
    f=assert(io.open(run..'/first-room-context.bin','wb'))
    for i=0,0x21ff do f:write(string.char(mem:readv_u8(context+i)))end
    f:close()
    print(string.format('FIRST_ROOM_STATE a5=%08X context=%08X',a5,context))
   end
   if label=='title-state' then
    print(string.format('TITLE_STATE_COMPLETE frame=%d cue=114',titleState))
    cpu.debug:bpclear(routeProbes[pc]);routePoints[pc]=nil
    finished=true;manager.machine:exit();dbg.execution_state='run';return
   end
   print('DISPLAY_ROUTE_CAPTURED '..label)
   cpu.debug:bpclear(routeProbes[pc]);routePoints[pc]=nil;routeStage=label
   if label=='first-room' and not audio and not gameplay then
    if integration then integration.finish()end
    if display then display.finish()end
    finished=true;manager.machine:exit()
   end
   dbg.execution_state='run'
  end)
  if not ok then fail(err)end
  return
 end
 if checkpoint and dbg.execution_state=='stop' and cpu.state.PC.value==checkpoint then
  local ok,err=pcall(function()
   assert(mac.frontmost()==app,'CAPTURE APPLICATION')
   assert(mem:readv_u16(0x15a)==0x755,'SYSTEM VERSION')
   assert(manager.machine.options.entries.ramsize:value()=='8M','RAM CONFIGURATION')
   print('MEMORY configured=8M addressing=24-bit')
   assert(mem:readv_u8(0xcb2)==0,'ADDRESSING MODE')
   if mapper then mapper.finish()end
   if audio then print('AUDIO_PHASE opening-checkpoint');if not displayRoute then audio.finish()end end
   if display and not displayRoute then display.finish()end
   if ui then ui.finish()end
   if files then files.finish()end
   if resourceCalls then resourceCalls.finish()end
   frame.rgb(run..'/opening.ppm')
   manager.machine.video:snapshot()
   print('REFERENCE_CAPTURED opening-text-1')
   if gameplay and trapTraceProbe then cpu.debug:bpclear(trapTraceProbe);trapTraceProbe=nil end
   if integration and os.getenv('POP2_FIXED_PROBE') then
    cpu.debug:bpclear(checkpointProbe);checkpoint=nil;integration.startFixed();return
   end
   if integration and os.getenv('POP2_NULL_DISPOSE') then
    cpu.debug:bpclear(checkpointProbe);checkpoint=nil;integration.startNil();return
   end
   if integration then integration.opening();if not displayRoute then integration.finish()end end
  end)
  if not ok then fail(err);return end
  if os.getenv('POP2_NULL_DISPOSE') or os.getenv('POP2_FIXED_PROBE') then return end
  if timing then
   cpu.debug:bpclear(checkpointProbe);checkpoint=nil
   local ok,err=pcall(timing.start);if not ok then fail(err)end
   return
  end
  if os.getenv('POP2_AUDIO')=='fixture' then
   cpu.debug:bpclear(checkpointProbe);audio.startFixture(checkpoint);checkpoint=nil;return
  end
  if uiRoutines then
   cpu.debug:bpclear(checkpointProbe)
   local ok,err=pcall(uiRoutines.start,meta,checkpoint);checkpoint=nil
   if not ok then fail(err)end
   return
  end
  if inputProbe then
   cpu.debug:bpclear(checkpointProbe);checkpoint=nil;inputStage=true
   local ok,err=pcall(inputProbe.start)
   if not ok then fail(err)end
   return
  end
  if videoTransfer then
   cpu.debug:bpclear(checkpointProbe);checkpoint=nil
   local ok,err=pcall(videoTransfer.start)
   if not ok then fail(err)end
   return
  end
  if displayRoute then
   cpu.debug:bpclear(checkpointProbe);checkpoint=nil;routeStage='opening'
   dbg.execution_state='run';return
  end
  if fileRoutines then
   cpu.debug:bpclear(checkpointProbe)
   local ok,err=pcall(fileRoutines.start,meta,checkpoint)
   checkpoint=nil
   if not ok then fail(err)end
   return
  end
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
  if os.getenv('POP2_GAMEPLAY')=='load-import' then
   -- Adding the save changes Finder's first icon. Select the application by
   -- its initial instead of opening the save document at that icon position.
   mac.press('p');mac.wait(60);mac.press('o',{mac.CMD})
  else
   assert(mac.mouse_to(103,113));mac.wait(60)
   for i=1,2 do mac.mouse_down();mac.wait(12);mac.mouse_up();mac.wait(12)end
  end
  assert(mac.wait_for('game',function()return mac.frontmost()==app end,1800),'APPLICATION')
  -- The paired CopyBits survey keeps the hardware cursor outside WIND 400.
  -- Use ordinary ADB input so no QuickDraw state or pixels are patched.
  if display then assert(mac.mouse_to(620,460),'DISPLAY CURSOR OUTSIDE CLIENT')end
  if uiRoutines then
   local steps={{'packing',352,223},{'packing-game',352,223},{'about'},
    {'alert-401-yes'},{'alert-404-yes'},{'alert-405-yes'},
    {'alert-405-no',355,225},{'alert-406-yes'},{'alert-406-no',352,223}}
   for _,step in ipairs(steps)do
    assert(mac.wait_for(step[1],function()return uiRoutines.phase()==step[1]end,18000),'UI ROUTINE START '..step[1])
    mac.wait(60)
    if step[2]then assert(mac.mouse_to(step[2],step[3]),'UI BUTTON');mac.click(1)else mac.press('Return')end
   end
   mac.wait(3000);error('UI ROUTINE NOT COMPLETE')
  end
  if inputProbe then
   assert(mac.wait_for('input-fixture',function()return inputStage end,18000),'INPUT START')
   mac.wait(20)
   for _,key in ipairs({'a','Left Arrow','Right Arrow','Up Arrow','Down Arrow','Space','Return','Esc'})do mac.press(key)end
   mac.press('s',{mac.CMD});mac.wait(20);inputProbe.finish();return
  end
  if displayRoute then
   assert(mac.wait_for('opening',function()return routeStage=='opening'end,18000),'DISPLAY OPENING')
   if os.getenv('POP2_AUDIO')~='cutscene' and not os.getenv('POP2_NATURAL_OPENING') then mac.press('Esc')end
   assert(mac.wait_for('title-start',function()return routeStage=='title-start'end,18000),'DISPLAY TITLE START')
   if os.getenv('POP2_AUDIO')=='cutscene' then
    audio.finish();print('AUDIO_CUTSCENE_COMPLETE');finished=true;manager.machine:exit();return
   end
   assert(mac.wait_for('title',function()return routeStage=='title'end,18000),'DISPLAY TITLE')
   if audio then print('AUDIO_PHASE title');mac.wait(120)end
   if not os.getenv('POP2_DEMO_ROUTE') and not os.getenv('POP2_NATURAL_TITLE') then mac.press('Space')end
   if gameplay then
    assert(mac.wait_for('gameplay-first-frame',function()return gameplay.rooms()>=1 end,18000),'GAMEPLAY FIRST FRAME')
    if os.getenv('POP2_GAMEPLAY')=='route' then
     if os.getenv('POP2_ROUTE_HALL')=='1' then
      assert(mac.wait_for('hall-ready',function()return gameplay.phase()=='hall-ready'end,216000),'GAMEPLAY HALL READY')
      print('GAMEPLAY_HALL_TYPING');mac.wait(12);mac.type('Codex');mac.press('Return');print('GAMEPLAY_HALL_TYPED')
      assert(mac.wait_for('hall-confirm',function()return gameplay.phase()=='hall-confirm'end,3600),'GAMEPLAY HALL CONFIRM')
      mac.wait(60);mac.press('Return')
     end
     assert(mac.wait_for('route-complete',gameplay.complete,216000),'GAMEPLAY ROUTE INCOMPLETE')
     return
    elseif os.getenv('POP2_GAMEPLAY')=='hall-view' then
     mac.wait(60);mac.press('h',{mac.CMD})
     assert(mac.wait_for('hall-view',function()return gameplay.phase()=='shown'end,3600),'HALL VIEW NOT SHOWN')
     mac.wait(30);mac.press('Esc')
     assert(mac.wait_for('hall-returned',function()return gameplay.phase()=='resumed' or gameplay.phase()=='paused'end,3600),'HALL VIEW NOT RETURNED')
     if gameplay.phase()=='paused' then mac.press('Space')end
     assert(mac.wait_for('hall-resumed',function()return gameplay.phase()=='resumed'end,3600),'HALL VIEW NOT DISMISSED')
    elseif os.getenv('POP2_GAMEPLAY')=='restart' then
     mac.wait(60);mac.press('r',{mac.CMD})
    else
     assert(os.getenv('POP2_GAMEPLAY')=='save-load' or os.getenv('POP2_GAMEPLAY')=='load-import','GAMEPLAY ROUTE')
     if os.getenv('POP2_GAMEPLAY')=='load-import' then
      assert(mac.wait_for('import-input',function()return gameplay.step()>=22 end,3600),'GAMEPLAY IMPORT INPUT')
     else
     mac.wait(60);mac.press('s',{mac.CMD})
     assert(mac.wait_for('save-picker',function()return gameplay.phase()=='save-picker'end,3600),'GAMEPLAY SAVE PICKER')
     mac.wait(120);dofile('tools/mame_mac_frame.lua').rgb(run..'/save-picker.ppm');mac.press('Return')
     assert(mac.wait_for('save-complete',function()return gameplay.phase()=='save-return'end,3600),'GAMEPLAY SAVE COMPLETE')
     mac.wait(30);mac.key_down('Left Arrow');mac.wait(20);mac.key_up('Left Arrow');mac.wait(30)
     end
     mac.press('o',{mac.CMD})
     assert(mac.wait_for('load-picker',function()return gameplay.phase()=='load-picker'end,3600),'GAMEPLAY LOAD PICKER')
     mac.wait(120);dofile('tools/mame_mac_frame.lua').rgb(run..'/load-picker.ppm');mac.press('Down Arrow');mac.press('Return')
     assert(mac.wait_for('load-complete',function()return gameplay.phase()=='load-return'end,3600),'GAMEPLAY LOAD COMPLETE')
    end
    if os.getenv('POP2_GAMEPLAY')=='hall-view' then
     assert(gameplay.phase()=='resumed','HALL VIEW RESUME')
    elseif os.getenv('POP2_GAMEPLAY')=='restart' then
     assert(mac.wait_for('gameplay-second-frame',function()return gameplay.rooms()==2 end,3600),'GAMEPLAY SECOND FRAME')
    else
     assert(mac.wait_for('gameplay-loaded-frame',function()return gameplay.loadedFrames()==1 end,3600),'GAMEPLAY LOADED FRAME')
    end
    gameplay.finish()
    if audio then audio.finish();print('AUDIO_ROUTE_COMPLETE')end
    finished=true;manager.machine:exit();return
   end
   if audio then
    assert(mac.wait_for('first-room',function()return routeStage=='first-room'end,18000),'AUDIO GAMEPLAY')
    print('AUDIO_PHASE gameplay')
    mac.wait(120);mac.key_down('Right Arrow')
    if os.getenv('POP2_DEMO_ROUTE') then
     mac.wait(120);mac.key_up('Right Arrow');mac.wait(120)
     audio.finish();print('AUDIO_DEMO_COMPLETE');finished=true;manager.machine:exit();return
    end
    assert(mac.wait_for('death',audio.dead,3600),'AUDIO DEATH NOT REACHED')
    mac.key_up('Right Arrow');mac.wait(240)
    audio.finish();print('AUDIO_ROUTE_COMPLETE');finished=true;manager.machine:exit();return
   end
   mac.wait(18000);error('DISPLAY FIRST ROOM NOT REACHED')
  else
   mac.wait(display and 18000 or 6000);error('MILESTONE NOT REACHED')
  end
 end)
 if not ok then fail(err)end
end)
dbg.execution_state='run'
