if os.getenv('POP2_GAMEPLAY')=='route' then return dofile('tools/mac_gameplay_route.lua')end
-- Original gameplay lifecycle observer. Diagnostic only. The save/load route
-- supplies the D6-authorized protection-accepted condition, as in the UI fixture.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local points={};local rooms=0;local starts=0;local commands=0
local route=os.getenv("POP2_GAMEPLAY");local phase;local reply;local files=0;local loadedFrames=0;local flushing;local flushed=false
local function dump(name,address,size)
 local f=assert(io.open(run..'/'..name..'.bin','wb'))
 for i=0,size-1 do f:write(string.char(mem:readv_u8(address+i)))end
 f:close()
end
function M.map(seg,base,guard,condition)
 if seg~=2 then return end
 local sites={{0x5aea,'restart-enter'},{0x6058,'first-frame'},{0x3554,'menu-command'}}
 if route=='save-load' then
  for _,point in ipairs({{0xe36,'save-picker'},{0xe5c,'load-picker'},{0xe5e,'picker-return'},
                        {0x1270,'save-enter'},{0x143e,'save-return'},{0x1038,'load-enter'},{0x11e2,'load-return'},{0x68b2,'frame-return'},{0x1a36,'protection-enter'},{0x5622,'keyboard-read'}})do sites[#sites+1]=point end
 end
 for _,point in ipairs(sites)do
  guard(seg,base,point[1],4)
  points[base+point[1]]=point[2]
  cpu.debug:bpset(base+point[1],condition,'logerror "GAMEPLAY_STOP\\n"')
 end
end
function M.poll()
 if dbg.execution_state~='stop' then return false end
 if flushing and cpu.state.PC.value==flushing.PC then
  assert(cpu.state.D0.value&65535==0 and mem:readv_u16(flushing.pb+16)==0,'GAMEPLAY FLUSH VOLUME')
  for name,value in pairs(flushing)do if name~='pb' then cpu.state[name].value=value end end
  flushing=nil;flushed=true;loadedFrames=1
  print('GAMEPLAY_VOLUME_FLUSHED')
  dbg.execution_state='run';return true
 end
 local label=points[cpu.state.PC.value]
 if not label then return false end
 local a5=cpu.state.A5.value&0xffffff
 if label=='keyboard-read' then
  local keymap=(cpu.state.A6.value&0xffffff)-16
  print(string.format('GAMEPLAY_KEYS step=%d left=%d right=%d command=%d',mem:readv_u16(a5-0x349a),mem:readv_u8(keymap+15)&8,mem:readv_u8(keymap+15)&16,mem:readv_u8(keymap+6)&128))
 elseif label=='protection-enter' then
  assert(route=='save-load' and phase=='load-return','GAMEPLAY PROTECTION CONTEXT')
  print(string.format('GAMEPLAY_PROTECTION_ENTER accepted=%d',mem:readv_u16(a5-0x56d8)))
  mem:writev_u16(a5-0x56d8,1) -- Owner decision D6; original accepted branch runs.
  print('GAMEPLAY_PROTECTION_ACCEPTED decision=D6')
 elseif label=='menu-command' then
  commands=commands+1
  print(string.format('GAMEPLAY_MENU ordinal=%d command=%08X',commands,mem:readv_u32(cpu.state.SP.value+4)))
 elseif label=='restart-enter' then
  starts=starts+1
  print(string.format('GAMEPLAY_RESTART_ENTER ordinal=%d requested=%d',starts,mem:readv_u16(cpu.state.SP.value+4)))
 elseif label=='save-picker' or label=='load-picker' then
  phase=label;reply=mem:readv_u32(cpu.state.SP.value+2)&0xffffff
  print('GAMEPLAY_FILE_PICKER '..label)
 elseif label=='picker-return' then
  assert(reply,'GAMEPLAY PICKER RETURN OWNER')
  files=files+1;dump('gameplay-picker-'..files,reply,76)
  print(string.format('GAMEPLAY_FILE_REPLY ordinal=%d good=%d volume=%04X',files,mem:readv_u8(reply),mem:readv_u16(reply+6)))
  reply=nil
 elseif label=='save-enter' or label=='load-enter' then
  dump('gameplay-'..label..'-a5',a5-22266,22266)
  dump('gameplay-'..label..'-context',mem:readv_u32(a5-0x5014)&0xffffff,0x2200)
  print('GAMEPLAY_FILE_ENTER '..label)
  print(string.format('GAMEPLAY_FILE_STEP label=%s step=%d',label,mem:readv_u16(a5-0x349a)))
 elseif label=='save-return' or label=='load-return' then
  assert(cpu.state.D0.value&65535==1,'GAMEPLAY FILE FAILED '..label)
  phase=label
  dump('gameplay-'..label..'-a5',a5-22266,22266)
  dump('gameplay-'..label..'-context',mem:readv_u32(a5-0x5014)&0xffffff,0x2200)
  print('GAMEPLAY_FILE_COMPLETE '..label)
 elseif label=='frame-return' and phase=='load-return' and loadedFrames==0 then
  dump('gameplay-loaded-frame-a5',a5-22266,22266)
  dump('gameplay-loaded-frame-context',mem:readv_u32(a5-0x5014)&0xffffff,0x2200)
  local frame=dofile('tools/mame_mac_frame.lua')
  frame.indexed(run..'/gameplay-loaded-frame.indices');frame.palette(run..'/gameplay-loaded-frame.rgb')
  print(string.format('GAMEPLAY_LOADED_FRAME result=%04X',cpu.state.D0.value&65535))
  -- Finish the original filesystem operation through a harness-authored
  -- PBFlushVolSync caller, as in mac_file_routines.lua. Preserve all registers
  -- and use a bounded private slice below the original stack.
  local stack=(cpu.state.SP.value&0xffffff)-4096
  assert(stack>0x100000 and stack+2048<a5-22266,'GAMEPLAY FLUSH STACK')
  flushing={pb=stack+512}
  for _,name in ipairs({'D0','D1','D2','D3','D4','D5','D6','D7','A0','A1','A2','A3','A4','A5','A6','SP','SR','PC'})do flushing[name]=cpu.state[name].value end
  for i=0,127 do mem:writev_u8(flushing.pb+i,0)end
  mem:writev_u16(stack+1024,0xa013);mem:writev_u16(stack+1026,0x4e75)
  mem:writev_u32(stack,flushing.PC);cpu.state.SP.value=stack
  cpu.state.A0.value=flushing.pb;cpu.state.PC.value=stack+1024

 elseif label=='first-frame' and cpu.state.D0.value&65535==0xfffe then
  rooms=rooms+1
  local context=mem:readv_u32(a5-0x5014)&0xffffff
  assert(context>=0x100000 and context+0x2200<0x800000,'GAMEPLAY CONTEXT')
  local prefix='gameplay-first-frame-'..rooms
  dump(prefix..'-a5',a5-22266,22266);dump(prefix..'-context',context,0x2200)
  local frame=dofile('tools/mame_mac_frame.lua')
  frame.indexed(run..'/'..prefix..'.indices');frame.palette(run..'/'..prefix..'.rgb')
  print(string.format('GAMEPLAY_FIRST_FRAME ordinal=%d starts=%d level=%d room=%d seed=%08X',rooms,starts,mem:readv_u16(a5-0x4fd2),mem:readv_u16(a5-0x4fc2),mem:readv_u32(a5-0x18fc)))
 end
 dbg.execution_state='run';return true
end
function M.rooms()return rooms end
function M.phase()return phase end
function M.loadedFrames()return loadedFrames end
function M.finish()
 if route=='restart' then
  assert(rooms==2 and starts==1 and commands==1,'GAMEPLAY RESTART COMPLETION')
  print('GAMEPLAY_REFERENCE_COMPLETE route=restart first_frames=2')
 else
  assert(route=='save-load' and rooms==2 and loadedFrames==1 and flushed and files==2 and phase=='load-return','GAMEPLAY SAVE/LOAD COMPLETION')
  print('GAMEPLAY_REFERENCE_COMPLETE route=save-load loaded_frames=1')
 end
end
return M
