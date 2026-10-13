-- Authored Time Manager fixture after the original opening checkpoint.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local base,ending,probe
local cases={{40,0},{-40000,0},{80,1},{20,100},{200,0,true},{0,0},{20,0,false,true}}
local function put(address,bytes)
 for i=1,#bytes do mem:writev_u8(address+i-1,bytes:byte(i))end
 for i=1,#bytes do assert(mem:readv_u8(address+i-1)==bytes:byte(i),'TIME FIXTURE BYTES')end
end
local function words(...)
 local out='';for _,word in ipairs({...})do out=out..string.pack('>I2',word)end;return out
end
local function long(value)return string.pack('>I4',value&0xffffffff)end
function M.start()
 base=(cpu.state.SP.value&0xffffff)-16384
 assert(base>0x100000 and base+4096<0x800000,'TIME FIXTURE EXTENT')
 local callback=base+1024
 -- Count, record TickCount and active flag, then optionally rearm twice.
 local body=words(0x5269,24,0x2379)..long(0x16a)..words(36,0x3369,4,52,
  0x0c69,1,24,0x6606,0x2369,36,48,0x4a69,60,0x670e,
  0x0c69,3,24,0x6406,0x2049,0x7014,0xa05a,0x4e75)
 -- Both conditional exits land on the single final RTS.
 -- The branch after TST skips CMP, BHS and the three priming instructions.
 put(callback,body)
 local program=''
 for i,c in ipairs(cases)do
  local task=base+2048+64*(i-1)
  put(task,string.rep('\0',64));mem:writev_u32(task+6,callback)
  if c[4]then mem:writev_u16(task+60,20)end
  program=program..words(0x207c)..long(task)..words(0xa058,0x33c0)..long(task+26)
  program=program..words(0x23f9)..long(0x16a)..long(task+32)
  program=program..words(0x203c)..long(c[1])..words(0x207c)..long(task)..words(0xa05a,0x33c0)..long(task+28)
  if c[2]~=0 then
   program=program..words(0x203c)..long(c[2])..words(0x207c)..long(task)..words(0xa05a,0x33c0)..long(task+56)
  end
  if c[3]then program=program..words(0x207c)..long(task)..words(0xa059,0x33c0)..long(task+30)end
  program=program..words(0x2c39)..long(task+32)..words(0x2039)..long(0x16a)..words(0x9086,0x0c80)..long(20)..words(0x65f0)
  program=program..words(0x23f9)..long(task+10)..long(task+40)..words(0x33f9)..long(task+4)..long(task+44)
  program=program..words(0x207c)..long(task)..words(0xa059,0x33c0)..long(task+58)
 end
 assert(#program<1024,'TIME FIXTURE CODE EXTENT')
 ending=base+#program;put(base,program..words(0x4e71))
 probe=cpu.debug:bpset(ending,'1','logerror "TIMING_TM_STOP\\n"')
 cpu.state.PC.value=base;dbg.execution_state='run'
end
function M.poll()
 if not ending or dbg.execution_state~='stop' or cpu.state.PC.value~=ending then return false end
 for i,c in ipairs(cases)do
  local task=base+2048+64*(i-1);local start=mem:readv_u32(task+32)
  print(string.format('TIMING_TM case=%d delay=%d second=%d calls=%d install=%04X prime=%04X cancel=%04X remove=%04X count=%08X flags=%04X callback_flags=%04X first=%d last=%d',
   i,c[1],c[2],mem:readv_u16(task+24),mem:readv_u16(task+26),mem:readv_u16(task+28),mem:readv_u16(task+30),mem:readv_u16(task+58),
   mem:readv_u32(task+40),mem:readv_u16(task+44),mem:readv_u16(task+52),
   (mem:readv_u32(task+48)-start)&0xffffffff,(mem:readv_u32(task+36)-start)&0xffffffff))
 end
 local out=assert(io.open(os.getenv('POP2_REFERENCE_RUN')..'/timing-tm.bin','wb'))
 for i=0,447 do out:write(string.char(mem:readv_u8(base+2048+i)))end
 out:close()
 print('TIMING_TM_COMPLETE cases=7')
 cpu.debug:bpclear(probe);ending=nil;manager.machine:exit();dbg.execution_state='run';return true
end
return M
