-- Authored VBL service fixture after the pinned original opening checkpoint.
-- Callback counters and task records are evidence, never original game bytes.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local ending,probe,base,removing
local counts={1,2,0,0xffff}
local function put(address,bytes)
 for i=1,#bytes do mem:writev_u8(address+i-1,bytes:byte(i))end
 for i=1,#bytes do assert(mem:readv_u8(address+i-1)==bytes:byte(i),'TIMING FIXTURE BYTES')end
end
function M.start()
 base=(cpu.state.SP.value&0xffffff)-16384
 assert(base>0x100000 and base+4096<0x800000,'TIMING FIXTURE EXTENT')
 local program=''
 for i,count in ipairs(counts)do
  local task=base+2048+32*i;local data=base+3072+32*i;local callback=base+512+64*i
  put(data,string.rep('\0',32))
  put(task,string.pack('>I4I2I4I2I2',0,1,callback,count,0))
  -- Increment counter, capture Ticks, rearm for one tick until three callbacks.
  put(callback,string.pack('>I2I4I2I4I4I2I2I4I2I2I2I4I2',
   0x5279,data,0x23f9,0x16a,data+4,0x0c79,3,data,0x6408,0x33fc,1,task+10,0x4e75))
  program=program..string.pack('>I2I4I2I2I4',0x207c,task,0xa033,0x33c0,data+2)
 end
 -- Sample clock and task counts twice: installation and observation need not
 -- share a tick. A tick or countdown change retries the complete sample.
 local function branch(op,target)
  local pc=base+#program
  program=program..string.pack('>I2i2',op,target-(pc+2))
 end
 local function snapshot(reg,offset,clock)
  local entry=base+#program
  program=program..string.pack('>I2I4',0x2039+reg*0x200,0x16a)
  for i in ipairs(counts)do
   local task=base+2048+32*i
   program=program..string.pack('>I2I4I4',0x33f9,task+10,task+offset)
  end
  program=program..string.pack('>I2I4I2',0x2039,0x16a,0xb080+reg)
  branch(0x6600,entry)
  for i in ipairs(counts)do
   local task=base+2048+32*i
   program=program..string.pack('>I2I4I2I4',0x3039,task+10,0xb079,task+offset)
   branch(0x6600,entry)
  end
  program=program..string.pack('>I2I4',0x23c0+reg,clock)
 end
 local delay=tonumber(os.getenv('POP2_TIMING_VBL_INSTALL_DELAY') or '0')
 assert(delay==0 or delay==3,'TIMING VBL INSTALL DELAY')
 if delay~=0 then
  program=program..string.pack('>I2I4',0x2c39,0x16a)
  local waiting=base+#program
  program=program..string.pack('>I2I4I2I2I4',0x2039,0x16a,0x9086,0x0c80,delay)
  branch(0x6500,waiting)
 end
 snapshot(6,14,base+1024)
 -- Wait thirty guest ticks, without executing a game frame or changing its timers.
 local wait=base+#program
 program=program..string.pack('>I2I4I2I2I4',0x2039,0x16a,0x9086,0x0c80,30)
 branch(0x6500,wait)
 snapshot(5,24,base+1028)
 ending=base+#program;program=program..string.pack('>I2',0x4e71)
 assert(#program<512,'TIMING VBL PROGRAM/CALLBACK SEPARATION')
 put(base,program)
 probe=cpu.debug:bpset(ending,'1','logerror "TIMING_VBL_STOP\\n"')
 cpu.state.PC.value=base;dbg.execution_state='run'
end
function M.poll()
 if not ending or dbg.execution_state~='stop' or cpu.state.PC.value~=ending then return false end
 if removing then
  for i,count in ipairs(counts)do
   print(string.format('TIMING_VREMOVE initial=%04X error=%04X',count,mem:readv_u16(base+3072+32*i+8)))
  end
  print('TIMING_VBL_COMPLETE removal=1')
  cpu.debug:bpclear(probe);ending=nil;manager.machine:exit();dbg.execution_state='run';return true
 end
 local elapsed=(mem:readv_u32(base+1028)-mem:readv_u32(base+1024))&0xffffffff
 assert(elapsed>=30 and elapsed<120,'TIMING FIXTURE DURATION')
 for i,count in ipairs(counts)do
  local task=base+2048+32*i;local data=base+3072+32*i
  print(string.format('TIMING_VBL initial=%04X calls=%d error=%04X count=%04X link=%08X last=%08X',count,
   mem:readv_u16(data),mem:readv_u16(data+2),mem:readv_u16(task+24),mem:readv_u32(task),mem:readv_u32(data+4)))
  print(string.format('TIMING_VBL_START initial=%04X count=%04X',count,mem:readv_u16(task+14)))
 end
 assert(mem:readv_u16(base+3072+32)>=3,'TIMING VBL POSITIVE CALLBACK')
 print('TIMING_VBL_OBSERVED elapsed='..elapsed)
 cpu.debug:bpclear(probe)
 local program=''
 for i in ipairs(counts)do
  program=program..string.pack('>I2I4I2I2I4',0x207c,base+2048+32*i,0xa034,0x33c0,base+3072+32*i+8)
 end
 local entry=base+128;ending=entry+#program
 put(entry,program..string.pack('>I2',0x4e71));removing=true
 probe=cpu.debug:bpset(ending,'1','logerror "TIMING_REMOVE_STOP\\n"')
 cpu.state.PC.value=entry;dbg.execution_state='run';return true
end
return M
