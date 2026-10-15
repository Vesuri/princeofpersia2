-- Interactive route discovery using the shared Macintosh input helper. Only
-- original completed-frame boundaries stop the CPU. Input records and captures
-- remain private; reaching the next level is the positive completion signal.
local M={}
local mac=dofile('tools/mame_mac_input.lua')
local frame=dofile('tools/mame_mac_frame.lua')
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local bases={}
local cutscenes=os.getenv('POP2_CUTSCENE_OBSERVER')=='1' and dofile('tools/mac_cutscene_observer.lua') or nil
local profileSave=os.getenv('POP2_PROFILE_SAVES')=='1' and dofile('tools/mac_profile_save.lua') or nil
local copyProbe=nil;local copySerial=0;local worldSerial=0
local bitmapProbe=nil;local bitmapSerial=0
local offsetProbe=nil;local offsetSerial=0
local paintProbe=nil;local paintSerial=0
local audioObserver=os.getenv('POP2_ROUTE_AUDIO_TRACE') and dofile('tools/mac_audio.lua') or nil
local audioMetadata=audioObserver and dofile(run..'/metadata.lua') or nil
local points={};local rooms=0;local frames=0;local remaining=0;local sequence=0
local waiting=false;local complete=false;local currentNIS={}
local checkpoints={};local heldKeys='none';local lastLevel=0
local discovery=os.getenv('POP2_ROUTE_DISCOVERY')=='1'
local assisted=os.getenv('POP2_ROUTE_ASSIST')=='1'
local directKeys=os.getenv('POP2_ROUTE_DIRECT_KEYS')=='1'
local ending=os.getenv('POP2_ROUTE_ENDING')=='1'
local hall=os.getenv('POP2_ROUTE_HALL')=='1'
local phase=nil;local hallTable=nil;local hallWritten=false;local flushing=nil;local hallReadyProbe=nil
assert(not hall or ending,'HALL ROUTE REQUIRES ENDING')
local victory=false
assert(not ending or assisted,'ENDING ROUTE REQUIRES ASSISTANCE')
assert(not directKeys or assisted,'DIRECT KEYS REQUIRE ASSISTANCE')
local keyCodes={left=123,right=124,up=126,down=125,shift=56,control=59,space=49,enter=36,escape=53,command=55,r=15}
assert(not assisted or discovery,'GAMEPLAY ASSIST REQUIRES DISCOVERY')
local names={left='Left Arrow',right='Right Arrow',up='Up Arrow',down='Down Arrow',shift='Shift',control='Control',space='Space',enter='Return',escape='Esc',command=mac.CMD,r='r'}
local function release()for _,name in pairs(names)do mac.key_up(name)end end
local function dump(path,address,size)
 local f=assert(io.open(run..'/'..path,'wb'))
 for i=0,size-1 do f:write(string.char(mem:readv_u8(address+i)))end
 f:close()
end
local function capture(reason)
 local a5=cpu.state.A5.value&0xffffff
 local prefix='route-'..sequence..(reason=='assist-before' and '-before' or '')
 dump(prefix..'-a5.bin',a5-22266,22266)
 dump(prefix..'-context.bin',mem:readv_u32(a5-0x5014)&0xffffff,0x2200)
 frame.indexed(run..'/'..prefix..'.indices');frame.palette(run..'/'..prefix..'.rgb')
 local status=string.format('sequence=%d frames=%d step=%d level=%d room=%d player4=%d player6=%d reason=%s prefix=%s\n',sequence,frames,mem:readv_u16(a5-0x349a),mem:readv_u16(a5-0x4fd2),mem:readv_u16(a5-0x4fc2),mem:readv_u16(a5-0x50ce),mem:readv_u16(a5-0x50cc),reason,prefix)
 local f=assert(io.open(run..'/route-status.tmp','w'));f:write(status);f:close()
 assert(os.rename(run..'/route-status.tmp',run..'/route-status.txt'))
 print('GAMEPLAY_ROUTE_CHECKPOINT '..status)
end
function M.map(seg,base,guard,condition)
 bases[seg]=base
 if cutscenes then cutscenes.map(seg,base,guard,condition)end
 if profileSave then profileSave.map(seg,base,guard)end
 if seg==15 and os.getenv('POP2_ROUTE_RESOURCE_WRITE_TRACE')=='1' then
  for _,offset in ipairs({0x38c0,0x38c2,0x38ce,0x38d0,0x3900,0x3902,0x3906,0x3908})do
   guard(seg,base,offset,4);points[base+offset]='resource-write-'..string.format('%04X',offset)
   cpu.debug:bpset(base+offset,condition,'logerror "GAMEPLAY_RESOURCE_WRITE_STOP\\n"')
  end
 end
 if seg==15 and os.getenv('POP2_ROUTE_PAINT_TRACE')=='1' then
  for _,point in ipairs({{0x3cac,'paint-before'},{0x3cae,'paint-after'}})do
   guard(seg,base,point[1],4);points[base+point[1]]=point[2]
   cpu.debug:bpset(base+point[1],condition,'logerror "GAMEPLAY_PAINT_STOP\\n"')
  end
 end
 if audioObserver then audioObserver.map(seg,base,guard,audioMetadata)end
 if (seg==17 or seg==15) and os.getenv('POP2_ROUTE_RANDOM_TRACE') then
  for _,offset in ipairs(seg==17 and {0x1c4e,0x6e1e} or {0x32a2})do
   guard(seg,base,offset,4);points[base+offset]='random'
   cpu.debug:bpset(base+offset,condition,'logerror \"GAMEPLAY_RANDOM_STOP\\n\"')
  end
 end
 if seg==15 and os.getenv('POP2_ROUTE_RESOURCE_TRACE')=='1' then
  for _,offset in ipairs({0x2736,0x27cc,0x2806,0x289c,0x28d6,0x296a,0x29aa,0x29c2,0x29da})do
   guard(seg,base,offset,4);points[base+offset]='handle-before';points[base+offset+2]='handle-after'
   cpu.debug:bpset(base+offset,condition,'logerror "GAMEPLAY_HANDLE_STOP\\n"')
   cpu.debug:bpset(base+offset+2,condition,'logerror "GAMEPLAY_HANDLE_STOP\\n"')
  end
 end
 if seg==3 and os.getenv('POP2_ROUTE_CURSOR_TRACE')=='1' then
  for _,point in ipairs({{0x5690,'cursor-return'},{0x57de,'cursor-purgeable'}})do
   guard(seg,base,point[1],4);points[base+point[1]]=point[2]
   cpu.debug:bpset(base+point[1],condition,'logerror "GAMEPLAY_CURSOR_STOP\\n"')
  end
 end
 if seg==17 and (os.getenv('POP2_ROUTE_COPY_TRACE')=='1' or os.getenv('POP2_ROUTE_WORLD_TRACE')=='1') then
  guard(seg,base,0x4e1a,4);points[base+0x4e1a]='world-before';points[base+0x4e1c]='world-after'
  cpu.debug:bpset(base+0x4e1a,condition,'logerror "GAMEPLAY_WORLD_STOP\\n"')
  cpu.debug:bpset(base+0x4e1c,condition,'logerror "GAMEPLAY_WORLD_STOP\\n"')
 end
 if seg==17 and os.getenv('POP2_ROUTE_COPY_TRACE')=='1' then
  guard(seg,base,0x508a,4);points[base+0x508a]='copy-before';points[base+0x508c]='copy-after'
  cpu.debug:bpset(base+0x508a,condition,'logerror "GAMEPLAY_COPY_STOP\\n"')
  cpu.debug:bpset(base+0x508c,condition,'logerror "GAMEPLAY_COPY_STOP\\n"')
 end
 if seg==17 and os.getenv('POP2_ROUTE_BITMAP_TRACE')=='1' then
  for _,point in ipairs({{0x5e3a,'bitmap-before'},{0x5e3c,'bitmap-after'},{0x5e68,'offset-before'},{0x5e6a,'offset-after'}})do
   guard(seg,base,point[1],4);points[base+point[1]]=point[2]
   cpu.debug:bpset(base+point[1],condition,'logerror "GAMEPLAY_BITMAP_STOP\\n"')
  end
 end
 if seg==17 and hall then
  guard(seg,base,0x6d42,4);points[base+0x6d42]='hall-confirm'
  cpu.debug:bpset(base+0x6d42,condition,'logerror "GAMEPLAY_HALL_STOP\\n"')
 end
 if seg~=2 then return end
 if ending then
  for _,point in ipairs({{0x45b0,'victory'},{0x45b8,'ending-return'}})do
   guard(seg,base,point[1],4);points[base+point[1]]=point[2]
   cpu.debug:bpset(base+point[1],condition,'logerror "GAMEPLAY_ENDING_STOP\\n"')
  end
 end
 if hall then
  for _,point in ipairs({{0x2220,'hall-name'},{0x231a,'hall-ready'},{0x1f86,'hall-name-return'},{0x2032,'hall-written'},{0x45c2,'hall-return'}})do
   guard(seg,base,point[1],4);points[base+point[1]]=point[2]
   local probe=cpu.debug:bpset(base+point[1],condition,'logerror "GAMEPLAY_HALL_STOP\\n"')
   if point[2]=='hall-ready' then hallReadyProbe=probe end
  end
 end
 for _,point in ipairs({{0x6058,'first'},{0x68b2,'frame'},{0x5622,'keys'},{0x0298,'nis-enter'},{0x03c6,'nis-return'},{0x1a36,'protection'}})do
  guard(seg,base,point[1],4);points[base+point[1]]=point[2]
  cpu.debug:bpset(base+point[1],condition,'logerror "GAMEPLAY_ROUTE_STOP\\n"')
 end
end
local function readbytes(address,size)
 local bytes={}
 for i=0,size-1 do bytes[#bytes+1]=string.char(mem:readv_u8(address+i))end
 return table.concat(bytes)
end
local function signature()
 local a5=cpu.state.A5.value&0xffffff
 local parts={readbytes(a5-22266,22266),readbytes(mem:readv_u32(a5-0x5014)&0xffffff,0x2200)}
 for _,name in ipairs({'D0','D1','D2','D3','D4','D5','D6','D7','A0','A1','A2','A3','A4','A5','A6','SP','SR','PC'})do
  parts[#parts+1]=string.pack('>I4',cpu.state[name].value&0xffffffff)
 end
 return table.concat(parts)
end
local function apply_keys(keys)
 release()
 if keys~='none' then for key in keys:gmatch('[^,]+')do mac.key_down(assert(names[key],'GAMEPLAY ROUTE UNKNOWN KEY'))end end
 heldKeys=keys
end
local function command()
 local f=io.open(run..'/route-command.txt','r');if not f then return end
 local text=f:read('a');f:close()
 local saveId,saveLevel=text:match('^(%d+) profile%-save (%d+)%s*$')
 if saveId then
  saveId=tonumber(saveId);saveLevel=tonumber(saveLevel)
  if saveId<=sequence then return end
  assert(profileSave and assisted and saveId==sequence+1 and #currentNIS==0,'PROFILE SAVE COMMAND')
  assert(saveLevel==mem:readv_u16((cpu.state.A5.value&0xffffff)-0x4fd2),'PROFILE SAVE LEVEL')
  sequence=saveId;waiting=false;release()
  profileSave.start(string.format('Profile %02d',saveLevel));return
 end
 local finishId=text:match('^(%d+) finish%s*$')
 if finishId then
  finishId=tonumber(finishId)
  if finishId<=sequence then return end
  assert(discovery and finishId==sequence+1 and #currentNIS==0,'GAMEPLAY DISCOVERY FINISH CONTEXT')
  sequence=finishId;release();capture('discovery-finished');complete=true;waiting=false
  local log=assert(io.open(run..'/route-actions.txt','a'));log:write(text);log:close()
  if audioObserver then audioObserver.finish()end
  if cutscenes then cutscenes.finish()end
  print('GAMEPLAY_DISCOVERY_COMPLETE');manager.machine:exit();dbg.execution_state='run';return
 end
 local assistId,kind,value=text:match('^(%d+) assist ([%a]+) (%d+)%s*$')
 if assistId then
  assistId=tonumber(assistId);value=tonumber(value)
  if assistId<=sequence then return end
  assert(assisted and assistId==sequence+1 and #currentNIS==0,'GAMEPLAY ASSIST CONTEXT')
  local a5=cpu.state.A5.value&0xffffff
  local writes={}
  if kind=='health' then
   assert(value>=1 and value<=12,'GAMEPLAY ASSIST HEALTH RANGE')
   assert(mem:readv_u16(a5-0x50b4)>0,'GAMEPLAY ASSIST CANNOT REVIVE DEAD ACTOR')
   writes={{0x50b4,value},{0x50b2,value}}
  elseif kind=='shadow' then
   assert(value==1 and mem:readv_u16(a5-0x4fd2)==14,'GAMEPLAY ASSIST SHADOW CONTEXT')
   assert(mem:readv_u16(a5-0x50b4)>0,'GAMEPLAY ASSIST CANNOT REVIVE DEAD ACTOR')
   -- DERIVED from the original transformation and MEASURED in ending-4:
   -- actor kind 1, shadow display attribute 48, ordinary shadow at health 2; final-level flame requires more than 2.
   -- Deliberately omit the abandoned-body actor; this cheat does not test re-entry.
   writes={{0x50ca,1},{0x509c,48},{0x50b4,2},{0x50b2,2}}
  elseif kind=='level' then
   assert(value>=1 and value<=14,'GAMEPLAY ASSIST LEVEL RANGE')
   -- Request the original session restart/level-load path (CODE 2+6124
   -- and +7148). This is an assisted entry, never a natural exit.
   writes={{0x4faa,value},{0x4f92,value},{0x4fac,1}}
  elseif kind=='room' then
   assert(value>=1 and value<=32,'GAMEPLAY ASSIST ROOM RANGE')
   assert(mem:readv_u16(a5-0x50b4)>0,'GAMEPLAY ASSIST CANNOT MOVE DEAD ACTOR')
   -- Use the original redraw branch at CODE 2+675A. ReadKeyboard clears
   -- the separate requested-room word before this frame boundary.
   writes={{0x50bc,value},{0x4fc2,value},{0x5244,1}}
  elseif kind=='y' then
   assert(value==106 or value==226 or value==346,'GAMEPLAY ASSIST FLOOR Y RANGE')
   assert(mem:readv_u16(a5-0x50b4)>0,'GAMEPLAY ASSIST CANNOT MOVE DEAD ACTOR')
   -- Actor y and logical row, using the original 120-unit floor spacing.
   writes={{0x50cc,value},{0x50c4,(value-106)//120}}
  elseif kind=='x' then
   assert(value>=230 and value<=720,'GAMEPLAY ASSIST X RANGE')
   assert(mem:readv_u16(a5-0x50b4)>0,'GAMEPLAY ASSIST CANNOT MOVE DEAD ACTOR')
   writes={{0x50ce,value}}
  else error('GAMEPLAY ASSIST UNKNOWN OPERATION')end
  sequence=assistId
  capture('assist-before')
  for _,write in ipairs(writes)do
   local before=mem:readv_u16(a5-write[1])
   mem:writev_u16(a5-write[1],write[2])
   assert(mem:readv_u16(a5-write[1])==write[2],'GAMEPLAY ASSIST WRITE FAILED')
   print(string.format('GAMEPLAY_ASSIST sequence=%d kind=%s a5minus=%04X before=%04X after=%04X',sequence,kind,write[1],before,write[2]))
  end
  local log=assert(io.open(run..'/route-actions.txt','a'));log:write(text);log:close()
  -- Separate after capture so the intervention never overwrites its evidence.
  capture('assisted')
  return
 end
 local stateId,operation,slot=text:match('^(%d+) (save) ([%w%-]+)%s*$')
 if not stateId then stateId,operation,slot=text:match('^(%d+) (load) ([%w%-]+)%s*$')end
 if stateId then
  stateId=tonumber(stateId)
  if stateId<=sequence then return end
  assert(stateId==sequence+1 and #slot<=48 and #currentNIS==0,'GAMEPLAY STATE COMMAND CONTEXT')
  local path=run..'/states/maciici/'..slot..'.sta'
  if operation=='save' then
   local existing=io.open(path,'rb');if existing then existing:close();error('GAMEPLAY STATE ALREADY EXISTS')end
   local saved={signature=signature(),frames=frames,rooms=rooms,keys=heldKeys,lastLevel=lastLevel}
   dbg:command('statesave '..slot)
   local state=assert(io.open(path,'rb'),'GAMEPLAY STATE FILE MISSING')
   assert(state:seek('end')>1000,'GAMEPLAY STATE FILE EXTENT');state:close()
   assert(signature()==saved.signature,'GAMEPLAY STATE SAVE MUTATED CHECKPOINT')
   checkpoints[slot]=saved
  else
   local saved=assert(checkpoints[slot],'GAMEPLAY STATE UNKNOWN CHECKPOINT')
   dbg:command('stateload '..slot)
   assert(signature()==saved.signature,'GAMEPLAY STATE RESTORE MISMATCH')
   frames=saved.frames;rooms=saved.rooms;lastLevel=saved.lastLevel;remaining=0;apply_keys(saved.keys)
  end
  sequence=stateId
  local log=assert(io.open(run..'/route-actions.txt','a'));log:write(text);log:close()
  print('GAMEPLAY_STATE_'..operation:upper()..' slot='..slot)
  capture('state-'..operation)
  return
 end
 local id,count,keys=text:match('^(%d+) (%d+) ([%w,%-]+)%s*$')
 assert(id,'GAMEPLAY ROUTE COMMAND FORMAT')
 id=tonumber(id);count=tonumber(count)
 if id<=sequence then return end
 assert(id==sequence+1 and count>=1 and count<=600,'GAMEPLAY ROUTE COMMAND ORDER/EXTENT')
 apply_keys(keys)
 sequence=id;remaining=count;waiting=false
 local log=assert(io.open(run..'/route-actions.txt','a'));log:write(text);if text:sub(-1)~='\n' then log:write('\n')end;log:close()
 print(string.format('GAMEPLAY_ROUTE_ACTION sequence=%d frames=%d keys=%s',id,count,keys))
 dbg.execution_state='run'
end
local function await_command()
 local deadline=os.time()+900
 while waiting do
  command()
  if waiting then
   assert(os.time()<deadline,'GAMEPLAY ROUTE INPUT DEADLINE')
   -- Headless debugger automatically resumes after callbacks return. Keep the
   -- exact completed instruction boundary until a host command is supplied.
   os.execute('/bin/sleep 1')
  end
 end
end
function M.poll()
 if cutscenes and cutscenes.poll()then return true end
 if profileSave then
  local result=profileSave.poll()
  if result=='complete' then capture('profile-saved');waiting=true;await_command();return true end
  if result then return true end
 end
 if audioObserver and audioObserver.poll() then return true end
 if dbg.execution_state~='stop' then return false end
 if flushing and cpu.state.PC.value==flushing.PC then
  assert(cpu.state.D0.value&65535==0 and mem:readv_u16(flushing.pb+16)==0,'HALL VOLUME FLUSH')
  for name,value in pairs(flushing)do if name~='pb' then cpu.state[name].value=value end end
  flushing=nil;complete=true;print('GAMEPLAY_HALL_COMPLETE');manager.machine:exit();dbg.execution_state='run';return true
 end
 if waiting then await_command();return true end
 local label=points[cpu.state.PC.value];if not label then return false end
 local a5=cpu.state.A5.value&0xffffff
 if label=='victory' then
  assert(cpu.state.D4.value&65535==15 and #currentNIS==0,'ENDING VICTORY RESULT')
  victory=true;release();capture('victory')
  print('GAMEPLAY_VICTORY result=15')
 elseif label=='ending-return' then
  assert(victory and #currentNIS==0,'ENDING RETURN WITHOUT VICTORY')
  capture('ending-return');print('GAMEPLAY_ENDING_COMPLETE')
  if not hall then complete=true;manager.machine:exit();dbg.execution_state='run';return true end
 elseif label=='hall-name' then
  assert(victory and not hallTable,'HALL NAME OWNER')
  local sp=cpu.state.SP.value&0xffffff
  hallTable=mem:readv_u32(sp+4)&0xffffff
  local count=mem:readv_u16(hallTable);local index=mem:readv_u16(sp+8)
  assert(count>=1 and count<=5 and index<count,'HALL TABLE EXTENT')
  dump('hall-before.bin',hallTable,2+count*30)
  print(string.format('GAMEPLAY_HALL_NAME count=%d index=%d',count,index));phase='hall-name'
 elseif label=='hall-ready' then
  assert(hallTable and hallReadyProbe,'HALL READY OWNER')
  cpu.debug:bpclear(hallReadyProbe);hallReadyProbe=nil
  frame.rgb(run..'/hall-ready.ppm');phase='hall-ready';print('GAMEPLAY_HALL_READY')
 elseif label=='hall-name-return' then
  assert(hallTable and cpu.state.D0.value&65535==1,'HALL NAME ACCEPTED')
  dump('hall-after.bin',hallTable,2+mem:readv_u16(hallTable)*30)
  print('GAMEPLAY_HALL_NAME_ACCEPTED');phase='hall-name-return'
 elseif label=='hall-confirm' then
  assert(hallTable and mem:readv_u16(cpu.state.SP.value+4)==406,'HALL CONFIRMATION OWNER')
  phase='hall-confirm';print('GAMEPLAY_HALL_CONFIRM')
 elseif label=='hall-written' then
  assert(hallTable and cpu.state.D0.value&65535==1,'HALL WRITE RESULT')
  hallWritten=true;print('GAMEPLAY_HALL_WRITTEN')
 elseif label=='hall-return' then
  assert(hallWritten,'HALL RETURN WITHOUT WRITE')
  print('GAMEPLAY_HALL_RETURN')
  -- Existing save/load fixture's explicit FlushVol caller, after session return.
  local stack=(cpu.state.SP.value&0xffffff)-2048
  assert(stack>0x100000 and stack+1152<a5-22266,'HALL FLUSH STACK')
  flushing={pb=stack+512}
  for _,name in ipairs({'D0','D1','D2','D3','D4','D5','D6','D7','A0','A1','A2','A3','A4','A5','A6','SP','SR','PC'})do flushing[name]=cpu.state[name].value end
  for i=0,127 do mem:writev_u8(flushing.pb+i,0)end
  mem:writev_u16(stack+1024,0xa013);mem:writev_u16(stack+1026,0x4e75)
  mem:writev_u32(stack,flushing.PC);cpu.state.SP.value=stack
  cpu.state.A0.value=flushing.pb;cpu.state.PC.value=stack+1024
 elseif label:match('^resource%-write%-') then
  local sp=cpu.state.SP.value&0xffffff
  local args={};for i=0,15 do args[#args+1]=string.format('%02X',mem:readv_u8(sp+i))end
  print(string.format('GAMEPLAY_RESOURCE_WRITE phase=%s current=%04X reserr=%04X memerr=%04X d0=%08X sp=%08X args=%s',label,mem:readv_u16(0xa5a),mem:readv_u16(0xa60),mem:readv_u16(0x220),cpu.state.D0.value,sp,table.concat(args)))
 elseif label=='cursor-return' or label=='cursor-purgeable' then
  local h=label=='cursor-return' and mem:readv_u32(cpu.state.SP.value)&0xffffff or cpu.state.A3.value&0xffffff
  assert(h~=0,'CURSOR TRACE NULL HANDLE')
  print(string.format('GAMEPLAY_CURSOR phase=%s handle=%08X master=%08X memerr=%04X',label,h,mem:readv_u32(h),mem:readv_u16(0x220)))
  if label=='cursor-return' then assert(mem:readv_u32(h)&0xffffff~=0,'CURSOR TRACE EMPTY RESULT')end
 elseif label=='paint-before' then
  paintSerial=paintSerial+1;assert(paintSerial<=8,'PAINT TRACE LIMIT')
  local port=mem:readv_u32(a5-0x187e)&0xffffff
  local map=mem:readv_u32(mem:readv_u32(port+2)&0xffffff)&0xffffff
  local pattern=mem:readv_u32(mem:readv_u32(port+58)&0xffffff)&0xffffff
  local data=mem:readv_u32(map)
  if mem:readv_u16(map+14)==2 then data=mem:readv_u32(data&0xffffff)end
  local size=(mem:readv_u16(map+4)&0x3fff)*(mem:readv_i16(map+10)-mem:readv_i16(map+6))
  assert(size>0 and size<=1048576,'PAINT TRACE SIZE')
  paintProbe={prefix='paint-'..paintSerial,data=data,size=size}
  dump(paintProbe.prefix..'-port.bin',port,108);dump(paintProbe.prefix..'-map.bin',map,50)
  dump(paintProbe.prefix..'-pattern.bin',pattern,28)
  for _,r in ipairs({{24,'vis'},{28,'clip'}})do
   local region=mem:readv_u32(mem:readv_u32(port+r[1])&0xffffff)&0xffffff
   local bytes=mem:readv_u16(region);assert(bytes>=10 and bytes<=32766,'PAINT REGION EXTENT')
   dump(paintProbe.prefix..'-'..r[2]..'.bin',region,bytes)
  end
  local patData=mem:readv_u32(pattern+6)&0xffffff
  if patData~=0 then dump(paintProbe.prefix..'-pattern-data.bin',mem:readv_u32(patData)&0xffffff,8)end
  dump(paintProbe.prefix..'-rect.bin',mem:readv_u32(cpu.state.SP.value)&0xffffff,8)
 elseif label=='paint-after' then
  assert(paintProbe,'PAINT TRACE PAIR')
  print(string.format('GAMEPLAY_PAINT_RETURN id=%d',paintSerial));paintProbe=nil
 elseif label=='offset-before' then
  offsetSerial=offsetSerial+1;assert(offsetSerial<=2048,'OFFSET TRACE LIMIT')
  local sp=cpu.state.SP.value&0xffffff
  offsetProbe={prefix='offset-'..offsetSerial,handle=mem:readv_u32(sp+4)&0xffffff}
  dump(offsetProbe.prefix..'-delta.bin',sp,4)
  local data=mem:readv_u32(offsetProbe.handle)&0xffffff;local size=mem:readv_u16(data)
  assert(size>=10 and size<=32766,'OFFSET TRACE INPUT')
  dump(offsetProbe.prefix..'-before.bin',data,size)
 elseif label=='offset-after' then
  assert(offsetProbe,'OFFSET TRACE PAIR')
  local data=mem:readv_u32(offsetProbe.handle)&0xffffff;local size=mem:readv_u16(data)
  assert(size>=10 and size<=32766,'OFFSET TRACE OUTPUT')
  dump(offsetProbe.prefix..'-after.bin',data,size)
  print(string.format('GAMEPLAY_OFFSET_RETURN id=%d size=%d',offsetSerial,size));offsetProbe=nil
 elseif label=='bitmap-before' then
  bitmapSerial=bitmapSerial+1;assert(bitmapSerial<=2048,'BITMAP TRACE LIMIT')
  local sp=cpu.state.SP.value&0xffffff;local map=mem:readv_u32(sp)&0xffffff
  local stride=mem:readv_u16(map+4);local height=mem:readv_i16(map+10)-mem:readv_i16(map+6)
  assert(stride>0 and stride<8192 and height>0 and stride*height<=1048576,'BITMAP TRACE EXTENT')
  bitmapProbe={prefix='bitmap-'..bitmapSerial,handle=mem:readv_u32(sp+4)&0xffffff}
  dump(bitmapProbe.prefix..'-map.bin',map,14)
  dump(bitmapProbe.prefix..'-pixels.bin',mem:readv_u32(map)&0xffffff,stride*height)
 elseif label=='bitmap-after' then
  assert(bitmapProbe and mem:readv_u16(cpu.state.SP.value)==0,'BITMAP TRACE RESULT')
  local data=mem:readv_u32(bitmapProbe.handle)&0xffffff;local size=mem:readv_u16(data)
  assert(size>=10 and size<=32766,'BITMAP TRACE REGION')
  dump(bitmapProbe.prefix..'-region.bin',data,size)
  print(string.format('GAMEPLAY_BITMAP_RETURN id=%d size=%d',bitmapSerial,size));bitmapProbe=nil
 elseif label=='world-before' then
  worldSerial=worldSerial+1
  dump('world-'..worldSerial..'-bounds.bin',mem:readv_u32(cpu.state.SP.value+12)&0xffffff,8)
 elseif label=='world-after' then
  assert(mem:readv_u16(cpu.state.SP.value)==0,'WORLD TRACE ALLOCATION FAILED')
  local port=mem:readv_u32((cpu.state.A6.value&0xffffff)-4)&0xffffff
  local pm=mem:readv_u32(mem:readv_u32(port+2)&0xffffff)&0xffffff
  dump('world-'..worldSerial..'-map.bin',pm,50)
  print('GAMEPLAY_WORLD_RETURN id='..worldSerial)
 elseif label=='copy-before' then
  local sp=cpu.state.SP.value&0xffffff
  local source=mem:readv_u32(sp+18)&0xffffff;local destination=mem:readv_u32(sp+14)&0xffffff
  if source~=destination then dbg.execution_state='run';return true end
  copySerial=copySerial+1;local prefix='copy-'..copySerial
  local stride=mem:readv_u16(source+4)&0x3fff
  local height=mem:readv_i16(source+10)-mem:readv_i16(source+6)
  local pixels=mem:readv_u32(source)&0xffffff
  if mem:readv_u16(source+14)==2 then pixels=mem:readv_u32(pixels)&0xffffff end
  assert(stride>0 and height>0 and stride*height<=1024*1024,'COPY TRACE EXTENT')
  dump(prefix..'-map.bin',source,50);dump(prefix..'-args.bin',sp,22)
  dump(prefix..'-from.bin',mem:readv_u32(sp+10)&0xffffff,8)
  dump(prefix..'-to.bin',mem:readv_u32(sp+6)&0xffffff,8)
  dump(prefix..'-before.bin',pixels,stride*height)
  copyProbe={prefix=prefix,pixels=pixels,size=stride*height}
 elseif label=='copy-after' then
  if not copyProbe then dbg.execution_state='run';return true end
  dump(copyProbe.prefix..'-after.bin',copyProbe.pixels,copyProbe.size)
  print('GAMEPLAY_COPY_RETURN prefix='..copyProbe.prefix);copyProbe=nil
 elseif label=='handle-before' or label=='handle-after' then
  local h=cpu.state.A0.value&0xffffff
  print(string.format('GAMEPLAY_HANDLE phase=%s offset=%04X a0=%08X master=%08X memerr=%04X owners=%08X/%08X/%08X',label,cpu.state.PC.value-bases[15],h,mem:readv_u32(h),mem:readv_u16(0x220),mem:readv_u32(a5-0x2454),mem:readv_u32(a5-0x2450),mem:readv_u32(a5-0x244c)))
 elseif label=='random' then
  local step=mem:readv_u16(a5-0x349a)
  if (step>=523 and step<=551) or (step>=819 and step<=865) or (step>=1490 and step<=1545) then
   local caller=mem:readv_u32(cpu.state.A6.value+4)&0xffffff
   print(string.format('ROUTE_RANDOM step=%d seed=%08X result=%04X site=%04X caller=%08X',step,mem:readv_u32(a5-0x18fc),mem:readv_u16(cpu.state.SP.value),cpu.state.PC.value-(cpu.state.PC.value>=bases[17] and cpu.state.PC.value<bases[17]+28830 and bases[17] or bases[15]),caller))
  end
 elseif label=='keys' then
  local pointer=(cpu.state.A6.value&0xffffff)-16;local bytes={}
  if directKeys then
   for i=0,15 do mem:writev_u8(pointer+i,0)end
   if heldKeys~='none' then for key in heldKeys:gmatch('[^,]+')do
    local code=assert(keyCodes[key],'DIRECT KEY CODE');local byte=pointer+code//8
    mem:writev_u8(byte,mem:readv_u8(byte)|(1<<(code%8)))
   end end
   print('GAMEPLAY_DIRECT_KEYS frame='..frames..' keys='..heldKeys)
  end
  for i=0,15 do bytes[#bytes+1]=string.format('%02X',mem:readv_u8(pointer+i))end
  print(string.format('GAMEPLAY_ROUTE_KEYS frame=%d step=%d map=%s',frames,mem:readv_u16(a5-0x349a),table.concat(bytes)))
 elseif label=='protection' then
  mem:writev_u16(a5-0x56d8,1) -- Owner-authorized D6 accepted branch.
  print('GAMEPLAY_ROUTE_PROTECTION_ACCEPTED decision=D6')
 elseif label=='nis-enter' then
  local selector=mem:readv_u16(cpu.state.SP.value+4)
  currentNIS[#currentNIS+1]=selector;print('GAMEPLAY_ROUTE_NIS_ENTER selector='..selector)
  if cutscenes then cutscenes.enter(selector)end
 elseif label=='nis-return' then
  assert(#currentNIS>0,'GAMEPLAY ROUTE NIS RETURN OWNER')
  print('GAMEPLAY_ROUTE_NIS_RETURN selector='..table.remove(currentNIS))
 elseif label=='first' and cpu.state.D0.value&65535==0xfffe then
  rooms=rooms+1
  local level=mem:readv_u16(a5-0x4fd2)
  if level==2 and not discovery then
   release();capture('next-level');complete=true
   if audioObserver then audioObserver.finish()end
   print('GAMEPLAY_ROUTE_COMPLETE level=1 next=2');manager.machine:exit();dbg.execution_state='run';return true
  end
  assert(level==1 or (discovery and level>=1 and level<=14),'GAMEPLAY ROUTE UNEXPECTED LEVEL '..level)
  local changed=lastLevel~=0 and level~=lastLevel;lastLevel=level
  if changed then release();remaining=0 end
  capture(changed and 'next-level' or 'first-frame');waiting=true;await_command();return true
 elseif label=='frame' then
  frames=frames+1
  if cpu.state.D0.value&65535~=0xfffe then
   print(string.format('GAMEPLAY_ROUTE_FRAME_RESULT frame=%d result=%04X',frames,cpu.state.D0.value&65535))
   release();remaining=0
  elseif remaining>0 then
   remaining=remaining-1
   if remaining==0 then capture('completed-frames');waiting=true;await_command();return true end
  end
 end
 dbg.execution_state='run';return true
end
function M.rooms()return rooms end
function M.phase()return phase end
function M.complete()return complete end
return M
