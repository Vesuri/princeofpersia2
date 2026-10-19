-- Original System 7.5.5 StripAddress contract, using the shared fixture seam.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=assert(manager.machine.debugger)
local worker,code,finish,probe
local function run()
 assert(mem:readv_u8(0xcb2)==0,'STRIP / REFERENCE ADDRESS MODE')
 local count=0
 for _,value in ipairs({0,0x123456,0x41c27330,0x40000000,0x7fff8000,0x81234567})do
  for cc=0,31 do
   mem:writev_u16(code,0xa055);mem:writev_u16(code+2,0x4e71)
   dbg:command(string.format('pc=0x%x;d0=0x%x;sr=0x%x',code,value,cc))
   probe=cpu.debug:bpset(code+2,'1','');dbg.execution_state='run';coroutine.yield()
   assert(cpu.state.PC.value==code+2,'STRIP / RETURN');cpu.debug:bpclear(probe)
   local result=cpu.state.D0.value;local flags=cpu.state.SR.value&31
   local low=result&65535;local expected=(cc&16)|(low==0 and 4 or 0)|(low>=32768 and 8 or 0)
   print(string.format('STRIP input=%08X output=%08X cc=%02X->%02X',value,result,cc,flags))
   assert(result==(value&0xffffff) and flags==expected,'STRIP / RESULT')
   count=count+1
  end
 end
 print('STRIP ORIGINAL COMPLETE cases='..count);finish();manager.machine:exit()
end
function M.start(address,done)
 code=address;finish=done;cpu.state.SP.value=code-4096
 worker=coroutine.create(run);local ok,err=coroutine.resume(worker);assert(ok,err)
end
function M.poll()
 if not worker or coroutine.status(worker)=='dead' or dbg.execution_state~='stop' or cpu.state.PC.value~=code+2 then return false end
 local ok,err=coroutine.resume(worker);assert(ok,err);return true
end
return M
