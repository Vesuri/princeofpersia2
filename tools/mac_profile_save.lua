-- Explicit profiling fixture: call the original SaveGame, then flush its volume.
-- Same ABI/private-stack technique as mac_file_routines.lua. No save bytes are
-- synthesized. The route observer resumes only after restoring caller registers.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local base,active
function M.map(seg,address,guard)
 if seg==2 then guard(seg,address,0x1270,4);base=address end
end
function M.start(name)
 assert(base and not active and name:match('^Profile %d%d$'),'PROFILE SAVE CONTEXT')
 local a5=cpu.state.A5.value&0xffffff
 local stack=(cpu.state.SP.value&0xffffff)-4096
 assert(stack>0x100000 and stack+2048<a5-22266,'PROFILE SAVE STACK')
 active={name=name,stack=stack,phase='save',registers={}}
 for _,key in ipairs({'D0','D1','D2','D3','D4','D5','D6','D7','A0','A1','A2','A3','A4','A5','A6','SP','SR','PC'})do active.registers[key]=cpu.state[key].value end
 local path=name..string.char(0)
 for i=1,#path do mem:writev_u8(stack+256+i-1,path:byte(i))end
 mem:writev_u32(stack,active.registers.PC)
 mem:writev_u32(stack+4,stack+256);mem:writev_u16(stack+8,0)
 active.probe=cpu.debug:bpset(active.registers.PC,string.format('sp==0x%x',stack+4),'logerror "PROFILE_SAVE_RETURN\\n"')
 cpu.state.SP.value=stack;cpu.state.PC.value=base+0x1270
 print(string.format('PROFILE_SAVE_BEGIN name=%s level=%d room=%d',name,mem:readv_u16(a5-0x4fd2),mem:readv_u16(a5-0x4fc2)))
 dbg.execution_state='run'
end
function M.poll()
 if not active or dbg.execution_state~='stop' or cpu.state.PC.value~=active.registers.PC then return false end
 local stack=active.stack
 if active.phase=='save' then
  assert(cpu.state.D0.value&65535==1,'PROFILE SAVE ORIGINAL FAILED')
  local pb=stack+512
  for i=0,127 do mem:writev_u8(pb+i,0)end
  mem:writev_u16(stack+1024,0xa013);mem:writev_u16(stack+1026,0x4e75)
  mem:writev_u32(stack,active.registers.PC)
  cpu.state.SP.value=stack;cpu.state.A0.value=pb;cpu.state.PC.value=stack+1024
  active.phase='flush';dbg.execution_state='run';return 'busy'
 end
 assert(cpu.state.D0.value&65535==0 and mem:readv_u16(stack+528)==0,'PROFILE SAVE FLUSH FAILED')
 cpu.debug:bpclear(active.probe)
 for key,value in pairs(active.registers)do cpu.state[key].value=value end
 print('PROFILE_SAVE_COMPLETE name='..active.name)
 active=nil;return 'complete'
end
return M
