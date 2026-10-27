-- Original System 7.5.5 master-block and pointer placement in a private zone.
-- Reuses the private-zone/trap coroutine protocol from mac_heap.lua. Each case
-- builds handles, an optional freed hole and locked barrier, uses every free
-- master pointer, then allocates one nonrelocatable block: implicitly through
-- NewHandle, explicitly through MoreMasters, or through NewPtr. It reports the
-- address order of the new block and the data blocks, and which ones moved.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=assert(manager.machine.debugger)
local worker,code,zone,finish,probe
local MASTERS=8
local function ptr(a)return mem:readv_u32(a)&0xffffff end
local function call(trap,d0,a0)
 mem:writev_u16(code,trap);mem:writev_u16(code+2,0x4e71)
 assert(mem:readv_u16(code)==trap,'trampoline readback')
 dbg:command(string.format('pc=0x%x;d0=0x%x;a0=0x%x',code,d0 or 0,a0 or 0))
 probe=cpu.debug:bpset(code+2,'1','');dbg.execution_state='run';coroutine.yield()
 assert(cpu.state.PC.value==code+2,'trap return')
 cpu.debug:bpclear(probe)
 return cpu.state.D0.value, cpu.state.A0.value&0xffffff, mem:readv_u16(0x220)
end
local function handle(size)
 local _,h,e=call(0xa122,size);assert(e==0 and h~=0 and ptr(h)~=0,'NewHandle')
 return h
end
local function init()
 mem:writev_u32(code+16,zone);mem:writev_u32(code+20,zone+65536)
 mem:writev_u16(code+24,MASTERS);mem:writev_u32(code+26,0)
 local _,_,e=call(0xa019,0,code+16);assert(e==0 and ptr(0x118)==zone,'InitZone')
end
-- Layout names: h = unlocked 2048-byte handle, L = locked 2048-byte handle,
-- g = 2048-byte handle disposed after the layout is built (a free hole).
local CASES={
 {'unlocked-implicit','hhhh','implicit'},
 {'hole-implicit','hgLhh','implicit'},
 {'hole-explicit','hgLhh','explicit'},
 {'hole-pointer','hgLhh','pointer'},
 {'locked-low-implicit','Lghh','implicit'},
 {'locked-run-implicit','hgLLhgh','implicit'},
 {'locked-full-implicit','hLLh','implicit'},
 {'locked-full-pointer','hLLh','pointer'},
}
local function run()
 local _,p,e=call(0xa31e,65536);assert(e==0 and p~=0,'private arena');zone=p
 print(string.format('MASTERS zone=%X bytes=65536 count=%d',zone,MASTERS))
 for _,case in ipairs(CASES)do
  local name,layout,kind=case[1],case[2],case[3]
  init()
  local items,holes={},{}
  for i=1,#layout do
   local c=layout:sub(i,i);local h=handle(2048)
   if c=='g' then holes[#holes+1]=h
   else
    if c=='L' then call(0xa029,0,h)end
    items[#items+1]={label=c..#items+1,h=h}
   end
  end
  for _,h in ipairs(holes)do call(0xa023,0,h)end
  -- Use every remaining free master so the next handle needs a new block.
  local spare=0
  while ptr(zone+8)~=0 do
   local _,h,e2=call(0xa166);assert(e2==0 and h~=0,'NewEmptyHandle');spare=spare+1
   assert(spare<=MASTERS*4,'free master exhaustion')
  end
  local before={}
  for i,v in ipairs(items)do before[i]=ptr(v.h)end
  local block
  if kind=='implicit' then
   local _,h,e2=call(0xa122,512);assert(e2==0 and h~=0,'implicit NewHandle');block=h
   items[#items+1]={label='d',h=h,new=true}
  elseif kind=='explicit' then
   local _,_,e2=call(0xa036);assert(e2==0,'MoreMasters')
   local _,h,e3=call(0xa166);assert(e3==0 and h~=0,'NewEmptyHandle after MoreMasters');block=h
  else
   local _,q,e2=call(0xa11e,256);assert(e2==0 and q~=0,'NewPtr');block=q
  end
  local rows={{label=kind=='pointer' and 'P' or 'M',at=block}}
  local moved={}
  for i,v in ipairs(items)do
   rows[#rows+1]={label=v.label,at=ptr(v.h)}
   if not v.new and ptr(v.h)~=before[i] then moved[#moved+1]=v.label end
  end
  table.sort(rows,function(a,b)return a.at<b.at end)
  local order={};local offsets={}
  for i,r in ipairs(rows)do order[i]=r.label;offsets[i]=string.format('%s@0x%X',r.label,r.at-zone)end
  print(string.format('MASTERS %s spare=%d order=%s moved=%s at=%s',name,spare,table.concat(order,','),
   #moved>0 and table.concat(moved,',') or '-',table.concat(offsets,',')))
 end
 -- Relocatable request that fits only if the low movable handle moves across
 -- a locked one: 8 KB handle + 4 KB hole | locked | 9 KB hole | locked | locked rest.
 for _,request in ipairs({11264,13312})do
  init()
  local h1=handle(8192);local g=handle(4096);local l2=handle(1024);call(0xa029,0,l2)
  local f=handle(9216);local l3=handle(1024);call(0xa029,0,l3)
  local rest=handle(mem:readv_u32(zone+12)-64);call(0xa029,0,rest)
  call(0xa023,0,g);call(0xa023,0,f)
  local before=ptr(h1);local free=mem:readv_u32(zone+12)
  local _,h,e2=call(0xa122,request)
  local err=e2>=32768 and e2-65536 or e2
  print(string.format('MASTERS relocate request=%d error=%d free=%d h1=0x%X->0x%X l2=0x%X l3=0x%X new=0x%X',request,err,free,
   before-zone,ptr(h1)-zone,ptr(l2)-zone,ptr(l3)-zone,(err==0 and ptr(h)-zone) or 0))
 end
 print('MASTERS_FIXTURE_COMPLETE cases='..#CASES);finish();manager.machine:exit()
end
function M.start(address,done)
 code=address;finish=done;cpu.state.SP.value=code-4096
 worker=coroutine.create(run)
 local ok,err=coroutine.resume(worker);assert(ok,err)
end
function M.poll()
 if not worker or coroutine.status(worker)=='dead' or dbg.execution_state~='stop' or cpu.state.PC.value~=code+2 then return false end
 local ok,err=coroutine.resume(worker);assert(ok,err);return true
end
return M
