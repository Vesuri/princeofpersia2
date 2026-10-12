-- Explicit original-routine fixture, separate from normal reference execution.
-- Only caller arguments and a private stack slice are supplied by the harness.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local stage,base,stack,returnPC,probe,initial,output,world
local restart=os.getenv('POP2_RESTART_FROM')
local function saveName()
 local name=':Data:Fixture Save'..string.char(0)
 for i=1,#name do mem:writev_u8(output+i-1,name:byte(i))end
end
local function bytes(a,n)
 local out={};for i=0,n-1 do out[#out+1]=string.char(mem:readv_u8(a+i))end
 return table.concat(out)
end
local function invoke(offset,args)
 local sp=stack-#args-4
 mem:writev_u32(sp,returnPC)
 for i=1,#args do mem:writev_u8(sp+3+i,args:byte(i))end
 cpu.state.SP.value=sp;cpu.state.PC.value=base+offset
 probe=cpu.debug:bpset(returnPC,string.format('sp==0x%x',sp+4),'logerror "FILE_ROUTINE_RETURN\\n"')
 dbg.execution_state='run'
end
function M.start(meta,checkpoint)
 assert(not stage,'FILE ROUTINE DUPLICATE')
 local a5=cpu.state.A5.value&0xffffff;world=a5
 local info=assert(meta.segments[2]);local e=a5+info.jt
 assert(mem:readv_u16(e)==2 and mem:readv_u16(e+2)==0x4ef9,'FILE ROUTINE CODE2 LOADED')
 base=(mem:readv_u32(e+4)&0xffffff)-info.offset
 for _,off in ipairs({0x275c,0x29de,0x1270,0x1038})do
  for i=off,off+3 do assert(mem:readv_u8(base+i)==tonumber(info.hex:sub(i*2+1,i*2+2),16),'FILE ROUTINE GUARD')end
 end
 -- A bounded fixture below the stopped original stack; no original return is
 -- resumed after it. The process is terminated after the fixture completes.
 stack=(cpu.state.SP.value&0xffffff)-4096
 assert(stack>0x100000 and stack<a5-22266,'FILE ROUTINE STACK')
 output=stack+256;returnPC=checkpoint;stage='read'
 print('FILE_ROUTINE_BEGIN HallOfFame')
 invoke(0x275c,string.pack('>I2I4',0,output))
end
function M.poll()
 if not stage or dbg.execution_state~='stop' or cpu.state.PC.value~=returnPC then return false end
 cpu.debug:bpclear(probe)
 local result=cpu.state.D0.value&0xffffff
 if stage=='read' then
  assert(result~=0,'FILE ROUTINE INITIAL HALL')
  local count=mem:readv_u16(result)
  assert(count==0,'FILE ROUTINE FRESH HALL')
  initial=bytes(result,32)
  if restart then
   local f=assert(io.open(os.getenv('POP2_REFERENCE_RUN')..'/persisted/Prince.hof.data','rb'))
   local expected=f:read('a');f:close()
   assert(initial==expected,'FILE ROUTINE HALL RESTART BYTES')
   print('FILE_ROUTINE_COMPLETE HallOfFame restart-read')
   saveName();stage='resume';invoke(0x1038,string.pack('>I4I2',output,0))
  else
   stage='write';invoke(0x29de,string.pack('>I4',result))
  end
 elseif stage=='write' then
  assert(result==1,'FILE ROUTINE WRITE HALL')
  stage='readback';invoke(0x275c,string.pack('>I2I4',0,output))
 elseif stage=='readback' then
  assert(result~=0 and bytes(result,32)==initial,'FILE ROUTINE HALL READBACK')
  print('FILE_ROUTINE_COMPLETE HallOfFame write-readback')
  local level=mem:readv_u32(world-0x5014)&0xffffff
  print(string.format('FILE_SAVE_STATE level=%08X rebirth=%08X',level,mem:readv_u32(world-0x5018)))
  assert(level~=0,'FILE SAVE REQUIRES INITIALIZED LEVEL')
  saveName()
  stage='save';invoke(0x1270,string.pack('>I4I2',output,0))
 elseif stage=='save' then
  assert(result==1,'FILE ROUTINE SAVE')
  print('FILE_ROUTINE_COMPLETE SaveGame write')
  stage='resume';invoke(0x1038,string.pack('>I4I2',output,0))
 elseif stage=='resume' then
  assert(result==1,'FILE ROUTINE RESUME')
  print(restart and 'FILE_ROUTINE_COMPLETE ResumeGame restart-read' or 'FILE_ROUTINE_COMPLETE ResumeGame readback')
  -- Flush the original filesystem before the emulator closes the image.
  -- This is a harness-authored PBFlushVolSync caller, not patched game code.
  local pb=output+1024
  for i=0,127 do mem:writev_u8(pb+i,0)end
  mem:writev_u16(output+512,0xa013);mem:writev_u16(output+514,0x4e75)
  cpu.state.A0.value=pb;stage='flush';invoke(output+512-base,'')
 else
  assert(result==0 and mem:readv_u16(output+1040)==0,'FILE ROUTINE FLUSH VOLUME')
  print('FILE_ROUTINE_COMPLETE volume-flushed')
  manager.machine:exit();dbg.execution_state='run';stage=nil
 end
 return true
end
return M
