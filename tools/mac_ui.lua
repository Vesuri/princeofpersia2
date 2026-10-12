-- Read-only paired observer for original UI and Event Manager contracts.
-- Captured bytes are local evidence under the ignored reference directory.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local meta,pending;local serial=0
local probes,counts={},{}
local traps={0xa746,0xa346,0xa8fe,0xa912,0xa930,0xa9cc,0xa97b,0xa850,0xa852,0xa853,0xa856,
 0xa063,0xa032,0xa970,0xa971,0xa976,0xa860,0xa9c0,0xa93c,0xa949,0xa94d,
 0xa937,0xa944,0xa946,0xa947,0xa938,0xa939,0xa93b,0xa93d,0xa93e,0xa945,0xa94a,0xa94b,0xa91f}
local selected={};for _,t in ipairs(traps)do selected[t]=true end
local function ptr(a)return mem:readv_u32(a)&0xffffff end
local function dump(label,address,size)
 local f=assert(io.open(run..'/ui-'..serial..'-'..label..'.bin','wb'))
 local bytes={};for i=0,size-1 do bytes[#bytes+1]=string.char(mem:readv_u8(address+i))end
 f:write(table.concat(bytes));f:close()
end
function M.arm(metadata,condition)
 meta=metadata
 for _,t in ipairs(traps)do
  probes[t]=cpu.debug:bpset(meta.dispatcher.address,condition..string.format(' && w@(d@(sp+2))==0x%x',t),'logerror "UI_STOP\\n"')
 end
end
function M.poll()
 if not meta or dbg.execution_state~='stop'then return false end
 local pc=cpu.state.PC.value
 if pending and pc==pending.returnPC then
  dump('after-stack',cpu.state.SP.value,64);dump('after-low',0,0x1000)
  if pending.output then dump('after-output',pending.output,pending.size)end
  print(string.format('UI_RETURN id=%d sp=%08X d0=%08X a0=%08X',serial,cpu.state.SP.value,cpu.state.D0.value,cpu.state.A0.value))
  cpu.debug:bpclear(pending.probe);pending=nil;dbg.execution_state='run';return true
 end
 if pc~=meta.dispatcher.address then return false end
 local sp=cpu.state.SP.value;local caller=ptr(sp+2);local trap=mem:readv_u16(caller)
 if not selected[trap]then return false end
 local original=false;local a5=cpu.state.A5.value&0xffffff
 for seg,info in pairs(meta.segments)do
  local e=a5+info.jt
  if mem:readv_u16(e)==seg and mem:readv_u16(e+2)==0x4ef9 then
   local base=ptr(e+4)-info.offset
   if caller>=base and caller<base+#info.hex/2 then original=true end
  end
 end
 if not original then dbg.execution_state='run';return true end
 assert(not pending,'UI NESTED ORIGINAL CALL');serial=serial+1
 counts[trap]=(counts[trap]or 0)+1
 if counts[trap]==8 then cpu.debug:bpclear(probes[trap]);probes[trap]=nil end
 print(string.format('TRAP pc=%08X trap=%04X d0=%08X a0=%08X a1=%08X a5=%08X sp=%08X',caller,trap,cpu.state.D0.value,cpu.state.A0.value,cpu.state.A1.value,cpu.state.A5.value,sp))
 print(string.format('UI_CALL id=%d pc=%08X trap=%04X sp=%08X d0=%08X',serial,caller,trap,sp+8,cpu.state.D0.value))
 local p={returnPC=caller+2}
 if trap==0xa860 then p.output=ptr(sp+16);p.size=16 end
 if trap==0xa976 then p.output=ptr(sp+8);p.size=16 end
 if trap==0xa970 or trap==0xa971 then p.output=ptr(sp+8);p.size=16 end
 dump('before-stack',sp+8,64);dump('before-low',0,0x1000)
 if p.output then dump('before-output',p.output,p.size)end
 p.probe=cpu.debug:bpset(p.returnPC,'1','logerror "UI_STOP\\n"');pending=p
 dbg.execution_state='run';return true
end
function M.finish()
 assert(not pending and serial>0,'UI TRACE INCOMPLETE')
 print(string.format('UI_TRACE_COMPLETE calls=%d',serial))
end
return M
