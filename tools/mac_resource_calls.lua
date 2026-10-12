-- Read-only original Resource Manager / FSSpec call observer.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local meta,dispatcher,pending;local serial=0
local chain,files={},{}
local consumed={[0xa997]=4,[0xa998]=2,[0xa99a]=2,[0xa994]=0,[0xa9a0]=6,
 [0xa9af]=0,[0xa9a4]=4,[0xa9a8]=16,[0xa9a3]=4,[0xa992]=4,[0xa9a2]=4,[0xa9a5]=4,[0xa9a6]=4}
local selectors={[1]=14,[4]=14,[5]=4,[7]=6,[8]=14,[9]=8}
local function u(a)return mem:readv_u32(a)end
local function dump(label,address,size)
 local f=assert(io.open(run..'/resource-'..serial..'-'..label..'.bin','wb'))
 for i=0,size-1 do f:write(string.char(mem:readv_u8(address+i)))end
 f:close()
end
function M.arm(metadata,condition)
 meta=metadata;dispatcher=meta.dispatcher.address
 local terms={'w@(d@(sp+2))==0xaa52'}
 for trap,_ in pairs(consumed)do terms[#terms+1]=string.format('w@(d@(sp+2))==0x%x',trap)end
 cpu.debug:bpset(dispatcher,condition..' && ('..table.concat(terms,' || ')..')','logerror "RESOURCE_STOP\\n"')
end
function M.poll()
 if not dispatcher or dbg.execution_state~='stop'then return false end
 local pc=cpu.state.PC.value
 if pending and pc==pending.returnPC then
  local p=pending;cpu.debug:bpclear(p.probe)
  local returned=cpu.state.SP.value
  dump('after',returned,16)
  if p.trap==0xa997 and mem:readv_u16(returned)~=0xffff then
   local ref=mem:readv_u16(returned)
   if not files[ref]then table.insert(chain,1,ref)end
   files[ref]=p.path
  elseif p.trap==0xa99a and mem:readv_u16(0xa60)==0 then
   files[p.ref]=nil
   for i,ref in ipairs(chain)do if ref==p.ref then table.remove(chain,i);break end end
  elseif p.trap==0xa9a0 then
   local handle=u(returned)&0xffffff
   if handle~=0 then
    local start=false;local size
    for _,ref in ipairs(chain)do
     if ref==p.current then start=true end
     if start then
      local entries=meta.resources[files[ref]]
      if entries and entries[p.resource]then size=entries[p.resource];break end
     end
    end
    assert(size,'RESOURCE SOURCE NOT IDENTIFIED')
    local body=u(handle)&0xffffff
    print(string.format('RESOURCE_BODY id=%d handle=%08X body=%08X size=%d',serial,handle,body,size))
    if body~=0 then dump('payload',body,size)end
   end
  end
  if p.spec then dump('spec-after',p.spec,70)end
  if p.info then
   dump('info-id',p.info[3],2);dump('info-type',p.info[2],4)
   dump('info-name',p.info[1]+1,mem:readv_u8(p.info[1]))
  end
  print(string.format('RESOURCE_RETURN id=%d error=%04X d0=%08X',serial,mem:readv_u16(0xa60),cpu.state.D0.value))
  pending=nil;dbg.execution_state='run';return true
 end
 if pc~=dispatcher then return false end
 local sp=cpu.state.SP.value;local caller=u(sp+2)&0xffffff;local trap=mem:readv_u16(caller)
 if trap~=0xaa52 and consumed[trap]==nil then return false end
 print(string.format('TRAP pc=%08X trap=%04X d0=%08X a0=%08X a1=%08X a5=%08X sp=%08X',caller,trap,cpu.state.D0.value,cpu.state.A0.value,cpu.state.A1.value,cpu.state.A5.value,sp))
 local original=false;local a5=cpu.state.A5.value&0xffffff
 for seg,info in pairs(meta.segments)do
  local e=a5+info.jt
  if mem:readv_u16(e)==seg and mem:readv_u16(e+2)==0x4ef9 then
   local base=(u(e+4)&0xffffff)-info.offset
   if caller>=base and caller<base+#info.hex/2 then original=true end
  end
 end
 if not original then dbg.execution_state='run';return true end
 assert(not pending,'RESOURCE NESTED ORIGINAL CALL')
 local selector=cpu.state.D0.value&0xffff
 local pop=trap==0xaa52 and selectors[selector] or consumed[trap]
 assert(pop,'RESOURCE UNKNOWN SELECTOR')
 serial=serial+1;local args=sp+8
 local current=mem:readv_u16(0xa5a)
 if #chain==0 then chain[1]=current;files[current]='Prince of Persia 2' end
 local p={returnPC=caller+2,trap=trap,current=current}
 if trap==0xa99a then p.ref=mem:readv_u16(args)end
 if trap==0xa9a0 then p.resource=string.format('%x:%d',u(args+2),mem:readv_i16(args))end
 dump('before',args,64)
 local name
 if trap==0xa997 then name=u(args)&0xffffff end
 if trap==0xa9a8 then p.info={u(args)&0xffffff,u(args+4)&0xffffff,u(args+8)&0xffffff}end
 if trap==0xaa52 and selector==1 then name=u(args+4)&0xffffff;p.spec=u(args)&0xffffff end
 if name and name~=0 then
  local n=mem:readv_u8(name);dump('name',name+1,n)
  if trap==0xa997 then
   local text={};for i=1,n do text[#text+1]=string.char(mem:readv_u8(name+i))end
   p.path=table.concat(text):gsub('^:',''):gsub(':','/')
  end
 end
 print(string.format('RESOURCE_CALL id=%d pc=%08X trap=%04X selector=%04X current=%04X',serial,caller,trap,selector,mem:readv_u16(0xa5a)))
 p.probe=cpu.debug:bpset(p.returnPC,string.format('sp==0x%x',args+pop),'logerror "RESOURCE_STOP\\n"')
 pending=p;dbg.execution_state='run';return true
end
function M.finish()
 assert(not pending and serial>0,'RESOURCE TRACE INCOMPLETE')
 print(string.format('RESOURCE_TRACE_COMPLETE calls=%d',serial))
end
return M
