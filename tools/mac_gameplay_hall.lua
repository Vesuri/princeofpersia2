-- Fresh-session Hall read/show/dismiss observer. Original input and dialog run.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger;local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local points={};local phase=nil;local rooms=0
local function dump(name,address,size)
 local f=assert(io.open(run..'/'..name,'wb'))
 for i=0,size-1 do f:write(string.char(mem:readv_u8(address+i)))end
 f:close()
end
function M.map(seg,base,guard,condition)
 if seg~=2 then return end
 for _,site in ipairs({{0x6058,'first-frame'},{0x20f4,'draw'},{0x291e,'shown'},{0x29ca,'return'},{0x5912,'pause'},{0x68b2,'frame-return'}})do
  guard(seg,base,site[1],2);points[base+site[1]]=site[2]
  cpu.debug:bpset(base+site[1],condition,'logerror "HALL_VIEW_STOP\\n"')
 end
end
function M.poll()
 if dbg.execution_state~='stop' then return false end
 local pc=cpu.state.PC.value;local label=points[pc]
 if label=='shown' then
  assert(phase=='drawing','HALL VIEW FADE OWNER')
  dofile('tools/mame_mac_frame.lua').rgb(run..'/hall-view.ppm')
  phase='shown';print('GAMEPLAY_HALL_VIEW_SHOWN')
 elseif label=='first-frame' and cpu.state.D0.value&65535==0xfffe then
  rooms=rooms+1;assert(rooms==1,'HALL VIEW INITIAL ROOM')
 elseif label=='draw' then
  assert(not phase and rooms==1,'HALL VIEW DRAW OWNER')
  local sp=cpu.state.SP.value&0xffffff;local table=mem:readv_u32(sp+4)&0xffffff
  local count=mem:readv_u16(table);assert(count>0 and count<=5,'HALL VIEW TABLE')
  dump('hall-view-table.bin',table,2+count*30)
  phase='drawing'
 elseif label=='return' then
  assert(phase=='shown','HALL VIEW RETURN WITHOUT DRAW')
  phase='returned';print('GAMEPLAY_HALL_VIEW_RETURN')
  dofile('tools/mame_mac_frame.lua').rgb(run..'/hall-return.ppm')
 elseif label=='pause' and phase=='returned' then
  phase='paused';print('GAMEPLAY_HALL_VIEW_PAUSED')
 elseif label=='frame-return' and (phase=='returned' or phase=='paused') then
  phase='resumed';print('GAMEPLAY_HALL_VIEW_RESUMED')
 elseif not label then return false end
 dbg.execution_state='run';return true
end
function M.rooms()return rooms end
function M.phase()return phase end
function M.finish()
 assert(rooms==1 and phase=='resumed','HALL VIEW COMPLETION')
 print('GAMEPLAY_HALL_VIEW_COMPLETE')
end
return M
