-- Bounded visible checkpoints within original NIS calls; observation only.
local M={}
assert(os.getenv('POP2_ROUTE_DISCOVERY')=='1','CUTSCENE OBSERVER / DISCOVERY REQUIRED')
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger;local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local frame=dofile('tools/mame_mac_frame.lua')
local points={};local calls={};local serial=0;local returned=0
local selected={[1]=true,[8]=true,[32]=true,[128]=true,[512]=true}
local function capture(call,kind,count)
 local prefix='nis-'..call.ordinal..'-'..call.selector..'-'..kind..'-'..count
 frame.indexed(run..'/'..prefix..'.indices');frame.palette(run..'/'..prefix..'.rgb')
 local a5=cpu.state.A5.value&0xffffff
 local f=assert(io.open(run..'/'..prefix..'-a5.bin','wb'))
 for i=-22266,-1 do f:write(string.char(mem:readv_u8(a5+i)))end
 f:close()
 print(string.format('CUTSCENE_FRAME ordinal=%d selector=%d kind=%s count=%d prefix=%s',call.ordinal,call.selector,kind,count,prefix))
end
function M.map(seg,base,guard,condition)
 if seg~=15 then return end
 for _,site in ipairs({{0x0166,'return'},{0x3310,'copy'},{0x0418,'text'}})do
  guard(seg,base,site[1],2);points[base+site[1]]=site[2]
  cpu.debug:bpset(base+site[1],condition,'logerror "CUTSCENE_STOP\\n"')
 end
end
-- The CODE 2 wrapper is mapped before the first LoadSeg of CODE 15.
-- A CODE 15 entry breakpoint would miss that first call.
function M.enter(selector)
 serial=serial+1
 calls[#calls+1]={ordinal=serial,selector=selector,copy=0,text=0}
 print(string.format('CUTSCENE_ENTER ordinal=%d selector=%d',serial,selector))
end
function M.poll()
 if dbg.execution_state~='stop' then return false end
 local kind=points[cpu.state.PC.value]
 if not kind then return false end
 if #calls>0 then
  local call=calls[#calls]
  if kind=='return' then
   capture(call,'return',1)
   print(string.format('CUTSCENE_RETURN ordinal=%d selector=%d result=%d copy=%d text=%d',call.ordinal,call.selector,cpu.state.D0.value&65535,call.copy,call.text))
   table.remove(calls);returned=returned+1
  else
   call[kind]=call[kind]+1
   if selected[call[kind]]then capture(call,kind,call[kind])end
  end
 elseif kind=='return' then error('CUTSCENE OBSERVER / UNOWNED RETURN')end
 dbg.execution_state='run';return true
end
function M.finish()
 assert(serial>0 and returned==serial and #calls==0,'CUTSCENE OBSERVER / INCOMPLETE CALLS')
 print('CUTSCENE_OBSERVER_COMPLETE calls='..serial)
end
return M
