-- Execute the actual authored wrapper bodies on the reference Macintosh CPU.
-- The callbacks deliberately damage all registers/CCR. Observe the real RTS
-- continuation without executing a flag-changing capture instruction.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local base,meta,index,finish,stack,saved,breakpoint
local function nextCase()
 local which=index//64;local pattern=(index//32)%2;local cc=index%32
 local seed=pattern==0 and 0x11220000 or 0x89ab0000
 for i=0,7 do cpu.state['D'..i].value=seed+i*0x101 end
 for i=0,6 do cpu.state['A'..i].value=seed+(i+8)*0x101 end
 for i=0,63 do mem:writev_u8(stack+i,i~0xa5)end
 mem:writev_u32(base+meta.symbols.wrapperFixtureHook,0xffffffff)
 mem:writev_u32(base+meta.symbols.wrapperFixtureArgument,0xffffffff)
 cpu.state.SP.value=stack
 cpu.state.SR.value=(cpu.state.SR.value&0xffe0)|cc
 cpu.state.PC.value=base+meta.symbols[meta.wrappers[which+1]]
 dbg.execution_state='run'
end
function M.start(address,done)
 base=address;finish=done;index=0;stack=base+6144
 saved={}
 for _,name in ipairs({'D0','D1','D2','D3','D4','D5','D6','D7','A0','A1','A2','A3','A4','A5','A6','SP','SR','PC'})do saved[name]=cpu.state[name].value end
 local directory=assert(os.getenv('POP2_WRAPPER_FIXTURE'))
 meta=dofile(directory..'/metadata.lua')
 local f=assert(io.open(directory..'/image.bin','rb'));local bytes=f:read('*a');f:close()
 assert(#bytes==meta.size and #bytes<4096,'WRAPPER FIXTURE IMAGE EXTENT')
 for i=1,#bytes do mem:writev_u8(base+i-1,bytes:byte(i))end
 for _,offset in ipairs(meta.relocations)do
  assert(offset>=0 and offset+4<=#bytes,'WRAPPER FIXTURE RELOCATION')
  mem:writev_u32(base+offset,mem:readv_u32(base+offset)+base)
 end
 breakpoint=cpu.debug:bpset(base+meta.symbols.wrapperFixtureDone,'1','logerror "WRAPPER_FIXTURE_STOP\\n"')
 nextCase()
end
function M.poll()
 if not base or dbg.execution_state~='stop' or cpu.state.PC.value~=base+meta.symbols.wrapperFixtureDone then return false end
 local which=index//64;local pattern=(index//32)%2;local row={which,pattern,index%32}
 for i=0,7 do row[#row+1]=cpu.state['D'..i].value&0xffffffff end
 for i=0,6 do row[#row+1]=cpu.state['A'..i].value&0xffffffff end
 row[#row+1]=cpu.state.SR.value&31
 row[#row+1]=(cpu.state.SP.value-stack)&0xffffffff
 local argument=mem:readv_u32(base+meta.symbols.wrapperFixtureArgument)
 if which==0 or which==8 then argument=(argument-stack)&0xffffffff
 elseif which~=5 and which~=6 and which~=7 then argument=0 end
 row[#row+1]=argument;row[#row+1]=mem:readv_u32(base+meta.symbols.wrapperFixtureHook)
 local intact=1;for i=0,63 do if mem:readv_u8(stack+i)~=(i~0xa5)then intact=0 end end
 row[#row+1]=intact
 print(string.format('WRAPPER_ROW '..string.rep('%08X,',22)..'%08X',table.unpack(row)))
 index=index+1
 if index==640 then
  print('WRAPPER_FIXTURE_COMPLETE cases=640');finish()
  cpu.debug:bpclear(breakpoint)
  for name,value in pairs(saved)do cpu.state[name].value=value end
  for name,value in pairs(saved)do assert(cpu.state[name].value==value,'WRAPPER FIXTURE RESTORE '..name)end
  print('WRAPPER_FIXTURE_RESTORED');base=nil
  manager.machine:exit();dbg.execution_state='run'
 else nextCase()end
 return true
end
return M
