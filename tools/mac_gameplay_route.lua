-- Interactive route discovery using the shared Macintosh input helper. Only
-- original completed-frame boundaries stop the CPU. Input records and captures
-- remain private; reaching the next level is the positive completion signal.
local M={}
local mac=dofile('tools/mame_mac_input.lua')
local frame=dofile('tools/mame_mac_frame.lua')
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local points={};local rooms=0;local frames=0;local remaining=0;local sequence=0
local waiting=false;local complete=false;local currentNIS={}
local checkpoints={};local heldKeys='none';local lastLevel=0
local discovery=os.getenv('POP2_ROUTE_DISCOVERY')=='1'
local names={left='Left Arrow',right='Right Arrow',up='Up Arrow',down='Down Arrow',shift='Shift',control='Control',space='Space',enter='Return',escape='Esc',command=mac.CMD,r='r'}
local function release()for _,name in pairs(names)do mac.key_up(name)end end
local function dump(path,address,size)
 local f=assert(io.open(run..'/'..path,'wb'))
 for i=0,size-1 do f:write(string.char(mem:readv_u8(address+i)))end
 f:close()
end
local function capture(reason)
 local a5=cpu.state.A5.value&0xffffff
 local prefix='route-'..sequence
 dump(prefix..'-a5.bin',a5-22266,22266)
 dump(prefix..'-context.bin',mem:readv_u32(a5-0x5014)&0xffffff,0x2200)
 frame.indexed(run..'/'..prefix..'.indices');frame.palette(run..'/'..prefix..'.rgb')
 local status=string.format('sequence=%d frames=%d step=%d level=%d room=%d player4=%d player6=%d reason=%s prefix=%s\n',sequence,frames,mem:readv_u16(a5-0x349a),mem:readv_u16(a5-0x4fd2),mem:readv_u16(a5-0x4fc2),mem:readv_u16(a5-0x50ce),mem:readv_u16(a5-0x50cc),reason,prefix)
 local f=assert(io.open(run..'/route-status.tmp','w'));f:write(status);f:close()
 assert(os.rename(run..'/route-status.tmp',run..'/route-status.txt'))
 print('GAMEPLAY_ROUTE_CHECKPOINT '..status)
end
function M.map(seg,base,guard,condition)
 if seg~=2 then return end
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
 local finishId=text:match('^(%d+) finish%s*$')
 if finishId then
  finishId=tonumber(finishId)
  if finishId<=sequence then return end
  assert(discovery and finishId==sequence+1 and #currentNIS==0,'GAMEPLAY DISCOVERY FINISH CONTEXT')
  sequence=finishId;release();capture('discovery-finished');complete=true;waiting=false
  local log=assert(io.open(run..'/route-actions.txt','a'));log:write(text);log:close()
  print('GAMEPLAY_DISCOVERY_COMPLETE');manager.machine:exit();dbg.execution_state='run';return
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
 if dbg.execution_state~='stop' then return false end
 if waiting then await_command();return true end
 local label=points[cpu.state.PC.value];if not label then return false end
 local a5=cpu.state.A5.value&0xffffff
 if label=='keys' then
  local pointer=(cpu.state.A6.value&0xffffff)-16;local bytes={}
  for i=0,15 do bytes[#bytes+1]=string.format('%02X',mem:readv_u8(pointer+i))end
  print(string.format('GAMEPLAY_ROUTE_KEYS frame=%d step=%d map=%s',frames,mem:readv_u16(a5-0x349a),table.concat(bytes)))
 elseif label=='protection' then
  mem:writev_u16(a5-0x56d8,1) -- Owner-authorized D6 accepted branch.
  print('GAMEPLAY_ROUTE_PROTECTION_ACCEPTED decision=D6')
 elseif label=='nis-enter' then
  local selector=mem:readv_u16(cpu.state.SP.value+4)
  currentNIS[#currentNIS+1]=selector;print('GAMEPLAY_ROUTE_NIS_ENTER selector='..selector)
 elseif label=='nis-return' then
  assert(#currentNIS>0,'GAMEPLAY ROUTE NIS RETURN OWNER')
  print('GAMEPLAY_ROUTE_NIS_RETURN selector='..table.remove(currentNIS))
 elseif label=='first' and cpu.state.D0.value&65535==0xfffe then
  rooms=rooms+1
  local level=mem:readv_u16(a5-0x4fd2)
  if level==2 and not discovery then
   release();capture('next-level');complete=true
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
function M.complete()return complete end
return M
