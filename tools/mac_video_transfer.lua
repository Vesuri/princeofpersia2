-- CPU-executed SetEntries sweep, adapted from Alone in the Dark's observer.
-- Original game code is untouched; this isolated fixture exits after capture.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local base,returnPC,probe,batch,inputs,outputs
local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local function nextBatch()
 for i=0,255 do
  local red,green,blue
  if batch<256 then red=i*256+batch;green=red;blue=red
  else red=i*257;green=(255-i)*257;blue=((i*71)%256)*257 end
  local at=base+4096+i*8
  mem:writev_u16(at,i);mem:writev_u16(at+2,red);mem:writev_u16(at+4,green);mem:writev_u16(at+6,blue)
 end
 cpu.state.SP.value=base-8
 mem:writev_u32(base-8,base+4096);mem:writev_u16(base-4,255);mem:writev_u16(base-2,0)
 cpu.state.PC.value=base+8192;dbg.execution_state='run'
end
function M.start()
 assert(not base,'VIDEO SWEEP DUPLICATE')
 base=(cpu.state.SP.value&0xffffff)-12288;returnPC=base+8194;batch=0
 assert(base>0x100000,'VIDEO SCRATCH STACK')
 mem:writev_u16(base+8192,0xaa3f);mem:writev_u16(returnPC,0x4e71)
 assert(mem:readv_u32(base+8192)==0xaa3f4e71,'VIDEO FIXTURE BYTES')
 inputs=assert(io.open(run..'/video-input.bin','wb'));outputs=assert(io.open(run..'/video-output.bin','wb'))
 probe=cpu.debug:bpset(returnPC,'1','logerror "VIDEO_STOP\\n"')
 nextBatch()
end
function M.poll()
 if not base or dbg.execution_state~='stop' or cpu.state.PC.value~=returnPC then return false end
 assert(cpu.state.SP.value==base,'VIDEO STACK CLEANUP')
 local palette=assert(manager.machine.palettes[':nbc:mdc48'])
 for i=0,255 do
  local at=base+4096+i*8
  inputs:write(string.pack('>I2I2I2I2',mem:readv_u16(at),mem:readv_u16(at+2),mem:readv_u16(at+4),mem:readv_u16(at+6)))
  outputs:write(string.pack('>I4',palette:pen_color(i)))
 end
 print(string.format('VIDEO_BATCH batch=%d sp=%08X',batch,cpu.state.SP.value))
 if batch==256 then
  inputs:close();outputs:close();cpu.debug:bpclear(probe)
  print('VIDEO_TRANSFER_COMPLETE values=65536 mixed=256 calls=257 stack=8')
  base=nil;manager.machine:exit();dbg.execution_state='run'
 else batch=batch+1;nextBatch()end
 return true
end
return M
