-- Original gameplay lifecycle observer. Diagnostic only; no game-state writes.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local points={};local rooms=0;local starts=0;local commands=0
local function dump(name,address,size)
 local f=assert(io.open(run..'/'..name..'.bin','wb'))
 for i=0,size-1 do f:write(string.char(mem:readv_u8(address+i)))end
 f:close()
end
function M.map(seg,base,guard,condition)
 if seg~=2 then return end
 for _,point in ipairs({{0x5aea,'restart-enter'},{0x6058,'first-frame'},{0x3554,'menu-command'}})do
  guard(seg,base,point[1],4)
  points[base+point[1]]=point[2]
  cpu.debug:bpset(base+point[1],condition,'logerror "GAMEPLAY_STOP\\n"')
 end
end
function M.poll()
 if dbg.execution_state~='stop' then return false end
 local label=points[cpu.state.PC.value]
 if not label then return false end
 local a5=cpu.state.A5.value&0xffffff
 if label=='menu-command' then
  commands=commands+1
  print(string.format('GAMEPLAY_MENU ordinal=%d command=%08X',commands,mem:readv_u32(cpu.state.SP.value+4)))
 elseif label=='restart-enter' then
  starts=starts+1
  print(string.format('GAMEPLAY_RESTART_ENTER ordinal=%d requested=%d',starts,mem:readv_u16(cpu.state.SP.value+4)))
 elseif cpu.state.D0.value&65535==0xfffe then
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
function M.finish()
 assert(rooms==2 and starts==1 and commands==1,'GAMEPLAY RESTART COMPLETION')
 print('GAMEPLAY_REFERENCE_COMPLETE route=restart first_frames=2')
end
return M
