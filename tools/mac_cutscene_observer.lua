-- Bounded visible checkpoints within original NIS calls; observation only.
local M={}
assert(os.getenv('POP2_ROUTE_DISCOVERY')=='1','CUTSCENE OBSERVER / DISCOVERY REQUIRED')
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger;local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local frame=dofile('tools/mame_mac_frame.lua')
local timers=os.getenv('POP2_SCENE_TIMERS')=='1'
local deadlines=os.getenv('POP2_SCENE_DEADLINES')=='1'
local points={};local calls={};local serial=0;local returned=0
local selected={[1]=true,[2]=true,[4]=true,[8]=true,[32]=true,[128]=true,[512]=true}
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
 if deadlines and seg==16 then
  -- Observe only eligible updates, not the potentially unbounded polling loop.
  -- At +0392 the first-poll flag has already initialized the target.
  for _,offset in ipairs({0x0392,0x0454})do
   guard(seg,base,offset,4);points[base+offset]='deadline-'..string.format('%04X',offset)
   local when=condition
   if offset==0x0392 then when='('..condition..') && (d@(a6+0xc) >= d@(a2+0xe))' end
   cpu.debug:bpset(base+offset,when,'logerror "SCENE_DEADLINE_STOP\\n"')
  end
 end
 if timers and seg==16 then
  guard(seg,base,0x159a,4);points[base+0x159a]='animation-return'
  cpu.debug:bpset(base+0x159a,condition,'logerror "SCENE_ANIMATION_STOP\\n"')
 end
 if timers and seg==17 then
  guard(seg,base,0x3136,4);points[base+0x3136]='timer'
  cpu.debug:bpset(base+0x3136,condition,'logerror "SCENE_TIMER_STOP\\n"')
 end
 if timers and seg==15 then
  for _,offset in ipairs({0x3152,0x321a,0x3984,0x3b72})do
   guard(seg,base,offset,2);points[base+offset]='clock-'..string.format('%04X',offset)
   cpu.debug:bpset(base+offset,condition,'logerror "SCENE_CLOCK_STOP\\n"')
  end
 end
 if seg~=2 and seg~=15 then return end
 local sites=seg==2 and {{0x031e,'enter'},{0x033e,'enter'},{0x037e,'enter'}} or {{0x0166,'return'},{0x3310,'copy'},{0x0418,'text'},{0x2a28,'fade'},{0x72dc,'tower'}}
 for _,site in ipairs(sites)do
  guard(seg,base,site[1],2);points[base+site[1]]=site[2]
  cpu.debug:bpset(base+site[1],condition,'logerror "CUTSCENE_STOP\\n"')
 end
end
-- Observe actual arguments at the CODE 2 call sites before LoadSeg.
-- The wrapper remaps story selectors and handles copy protection separately.
local function enter(selector)
 serial=serial+1
 calls[#calls+1]={ordinal=serial,selector=selector,copy=0,text=0,fade=0,tower=0}
 print(string.format('CUTSCENE_ENTER ordinal=%d selector=%d',serial,selector))
end
function M.poll()
 if dbg.execution_state~='stop' then return false end
 local kind=points[cpu.state.PC.value]
 if not kind then return false end
 if kind:match('^deadline%-') then
  if #calls>0 then
   local call=calls[#calls];local a6=cpu.state.A6.value&0xffffff
   local object=mem:readv_u32(a6+8)&0xffffff
   local delay=mem:readv_u16(object+0xc);if delay>=0x8000 then delay=delay-0x10000 end
   print(string.format('SCENE_DEADLINE ordinal=%d selector=%d site=%s tick=%d target=%d delay=%d frame=%d active=%d',
    call.ordinal,call.selector,kind:sub(10),mem:readv_u32(a6+0xc),mem:readv_u32(object+0xe),delay,
    mem:readv_u32(object+4),mem:readv_u16(object+8)))
  end
 elseif kind=='animation-return' then
  if #calls>0 then
   local call=calls[#calls]
   print(string.format('SCENE_ANIMATION ordinal=%d selector=%d frames=%d result=%d',call.ordinal,call.selector,cpu.state.D5.value,cpu.state.D0.value&0xffff))
  end
 elseif kind:match('^clock%-') then
  if #calls>0 then
   local call=calls[#calls];local a6=cpu.state.A6.value&0xffffff
   local site=tonumber(kind:sub(7),16);local progress=site==0x321a and mem:readv_u32(a6-0x6c) or 0
   local target=site==0x321a and (mem:readv_u32(a6-0x7c)+progress)&0xffffffff or 0
   local delay=site==0x321a and mem:readv_u32(a6-0xa8) or 0
   print(string.format('SCENE_CLOCK ordinal=%d selector=%d site=%04X ticks=%d duration=%d progress=%d target=%d delay=%d',call.ordinal,call.selector,site,mem:readv_u32(0x16a),mem:readv_u32(a6+0x10),progress,target,delay))
  end
 elseif kind=='timer' then
  if #calls>0 then
   local call=calls[#calls];local sp=cpu.state.SP.value&0xffffff
   print(string.format('SCENE_TIMER ordinal=%d selector=%d timer=%d delay=%d ticks=%d',call.ordinal,call.selector,mem:readv_u16(sp+4),mem:readv_u16(sp+8),mem:readv_u32(0x16a)))
  end
 elseif kind=='enter' then
  enter(mem:readv_u16(cpu.state.SP.value&0xffffff))
 elseif #calls>0 then
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
