-- CPU-executed GetKeys/GetNextEvent fixture after the pinned opening capture.
-- Keys arrive through normal emulated ADB input. Game instructions are untouched.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local base,checkpoint,probe,output,lastMap;local samples=0;local eventCount=0
local function bytes(a,n)
 local t={};for i=0,n-1 do t[#t+1]=string.char(mem:readv_u8(a+i))end;return table.concat(t)
end
function M.start()
 assert(not base,'INPUT DUPLICATE START')
 base=(cpu.state.SP.value&0xffffff)-16384
 assert(base>0x100000,'INPUT SCRATCH EXTENT')
 local code=base+8192;checkpoint=code+24
 -- PEA keys; GetKeys; reserve Boolean; push mask and EventRecord;
 -- GetNextEvent; discard Boolean. The observer restarts this tiny fixture.
 local program=string.pack('>I2I4I2I2I2I2I2I4I2I2I2',
  0x4879,base+4096,0xa976,0x4267,0x3f3c,0x18,0x4879,base+4128,0xa970,0x548f,0x4e71)
 checkpoint=code+#program-2
 for i=1,#program do mem:writev_u8(code+i-1,program:byte(i))end
 assert(bytes(code,#program)==program,'INPUT FIXTURE BYTES')
 output=assert(io.open(run..'/input-samples.bin','wb'))
 probe=cpu.debug:bpset(checkpoint,'1','logerror "INPUT_STOP\\n"')
 cpu.state.SP.value=base;cpu.state.PC.value=code;dbg.execution_state='run'
 print('INPUT_FIXTURE_STARTED')
end
function M.poll()
 if not base or dbg.execution_state~='stop' or cpu.state.PC.value~=checkpoint then return false end
 assert(cpu.state.SP.value==base,'INPUT STACK CLEANUP')
 local keys=bytes(base+4096,16);local event=bytes(base+4128,16)
 local what=mem:readv_u16(base+4128)
 if keys~=lastMap or what~=0 then
  output:write(string.pack('>I4',mem:readv_u32(0x16a))..keys..event)
  samples=samples+1;lastMap=keys
  if what~=0 then eventCount=eventCount+1 end
  print(string.format('INPUT_SAMPLE sample=%d what=%d message=%08X modifiers=%04X',samples,what,mem:readv_u32(base+4130),mem:readv_u16(base+4142)))
 end
 cpu.state.PC.value=base+8192;dbg.execution_state='run';return true
end
function M.finish()
 assert(base and samples>1 and eventCount>0,'INPUT POSITIVE COMPLETION')
 output:close();cpu.debug:bpclear(probe)
 print(string.format('INPUT_FIXTURE_COMPLETE samples=%d events=%d',samples,eventCount))
 base=nil;manager.machine:exit();dbg.execution_state='run'
end
return M
