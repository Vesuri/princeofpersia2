-- Adapted from Alone in the Dark tools/mac_movehhi.lua.
-- Original System 7.5.5 private-zone placement and payload oracle.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=assert(manager.machine.debugger)
local worker,code,zone,finish,probe
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
 local d,h,e=call(0xa322,size);assert(e==0 and h~=0 and ptr(h)~=0,'NewHandle')
 return h
end
local function init()
 mem:writev_u32(code+16,zone);mem:writev_u32(code+20,zone+65536)
 mem:writev_u16(code+24,64);mem:writev_u32(code+26,0)
 local _,_,e=call(0xa019,0,code+16);assert(e==0 and ptr(0x118)==zone,'InitZone')
end
local function pattern(h,n,seed)
 for i=0,n-1 do mem:writev_u8(ptr(h)+i,(i+seed)&255)end
end
local function check(h,n,seed)
 assert(ptr(h)~=0,'unexpected purge')
 for i=0,n-1 do assert(mem:readv_u8(ptr(h)+i)==((i+seed)&255),'payload mismatch')end
end
local function test(name,h,others)
 local before=ptr(h);local addresses={}
 for i,v in ipairs(others)do addresses[i]=ptr(v)end
 local free=mem:readv_u32(zone+12)
 local d,_,e=call(0xa064,0,h)
 local line=string.format('MOVEHHI %s error=%d result=%X target=%X->%X free=%d->%d',name,e>=32768 and e-65536 or e,d,before-zone,ptr(h)-zone,free,mem:readv_u32(zone+12))
 for i,v in ipairs(others)do line=line..string.format(' other%d=%X->%X',i,addresses[i]-zone,ptr(v)==0 and 0 or ptr(v)-zone)end
 print(line)
 assert(e==0 and free==mem:readv_u32(zone+12),'result/free space')
 if name=='free-above' or name=='purgeable' then
  assert(ptr(h)>before and ptr(others[1])==addresses[1],'free-tail placement')
 elseif name=='locked-barrier' then
  assert(ptr(h)==before and ptr(others[1])==addresses[1],'locked barrier')
 else assert(ptr(h)>before and ptr(others[1])==before,'compacted neighbour')end
 check(h,1024,17)
 for _,v in ipairs(others)do if ptr(v)~=0 then check(v,512,31)end end
end
local function run()
 local _,p,e=call(0xa31e,65536);assert(e==0 and p~=0,'private arena');zone=p
 print(string.format('MOVEHHI zone=%X bytes=65536 system=%X',zone,mem:readv_u16(0x15a)))
 for _,name in ipairs({'free-above','locked-barrier','no-room','purgeable','fragmented','interior-hole','small-gap','small-tail','purgeable-no-room'})do
  init();local h=handle(1024);local hole
  if name=='interior-hole' or name=='small-tail' then hole=handle(2048)
  elseif name=='small-gap' then hole=handle(8)end
  local b=handle(512);pattern(h,1024,17);pattern(b,512,31)
  if hole then
   local tail;if name=='small-tail' then tail=handle(512)end
   local barrier=handle(512);call(0xa029,0,barrier);call(0xa023,0,hole)
   if tail then call(0xa023,0,tail)end
  end
  if name=='locked-barrier' then call(0xa029,0,b)
  elseif name=='purgeable' then call(0xa049,0,b)
  elseif name=='no-room' or name=='purgeable-no-room' then
   if name=='purgeable-no-room' then call(0xa049,0,b)end
   local free=mem:readv_u32(zone+12);local rest=handle(free-8);call(0xa029,0,rest)
  elseif name=='fragmented' then
   local holes={};for i=1,12 do holes[i]=handle(512)end
   local free=mem:readv_u32(zone+12);local rest=handle(free-8);call(0xa029,0,rest)
   for i=1,12,2 do call(0xa023,0,holes[i])end
  end
  test(name,h,{b})
 end
 init();local h=handle(1024);local b=handle(512);local barrier=handle(128)
 call(0xa029,0,barrier);pattern(h,1024,17);call(0xa023,0,b)
 local address=ptr(h)
 for _,size in ipairs({1024,256,512,1024})do
  local _,_,e=call(0xa024,size,h);assert(e==0 and ptr(h)==address,'in-place resize')
  local actual=call(0xa025,0,h);assert(actual==size,'logical resize extent')
  check(h,math.min(size,256),17)
  print(string.format('HEAP_RESIZE size=%d preserved=256 moved=0 error=0',size))
 end
 call(0xa029,0,h)
 local _,_,e=call(0xa024,131072,h);assert(e==0xff94 and ptr(h)==address,'locked low-memory failure')
 local size=call(0xa025,0,h);assert(size==1024,'failure size preserved');check(h,256,17)
 print('HEAP_RESIZE failure=-108 size=1024 preserved=256 moved=0')
 print('HEAP_FIXTURE_COMPLETE movehhi=9 resize=5');finish();manager.machine:exit()
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
