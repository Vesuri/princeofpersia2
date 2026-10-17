-- Opt-in original System 7.5.5 freed-master layout experiment.
-- Reuses the private-zone/trap coroutine protocol from mac_heap.lua.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local worker,code,finish,probe,zone
local function call(trap,d0,a0)
 mem:writev_u16(code,trap);mem:writev_u16(code+2,0x4e71)
 assert(mem:readv_u16(code)==trap,'FREED trap bytes')
 cpu.state.PC.value=code;cpu.state.D0.value=d0 or 0;cpu.state.A0.value=a0 or 0
 probe=cpu.debug:bpset(code+2,'1','');dbg.execution_state='run';coroutine.yield()
 assert(cpu.state.PC.value==code+2,'FREED trap return');cpu.debug:bpclear(probe)
 return cpu.state.D0.value,cpu.state.A0.value,mem:readv_u16(0x220)
end
local function snapshot()
 local bytes={};for i=0,65535 do bytes[#bytes+1]=string.char(mem:readv_u8(zone+i))end
 return table.concat(bytes)
end
local function run()
 local savedZone=mem:readv_u32(0x118)
 local _,p,e=call(0xa31e,65536);assert(e==0 and p~=0,'FREED private arena');zone=p
 for _,count in ipairs({1,2,3,8,32,64,65})do
  for _,reverse in ipairs({false,true})do
   mem:writev_u32(code+16,zone);mem:writev_u32(code+20,zone+65536)
   mem:writev_u16(code+24,64);mem:writev_u32(code+26,0)
   local _,_,err=call(0xa019,0,code+16);assert(err==0,'FREED InitZone')
   local handles={}
   for i=1,count do
    local _,h,err=call(0xa322,16);assert(err==0 and h~=0,'FREED NewHandle');handles[i]=h
   end
   for _,trap in ipairs({0xa02a,0xa049})do
    local d,_,err=call(trap,0,handles[1]);assert(d==0 and err==0,'FREED live control')
    print(string.format('FREED_LIVE count=%d reverse=%d trap=%04X error=0000',count,reverse and 1 or 0,trap))
   end
   for i=1,count do
    local h=handles[reverse and count+1-i or i]
    local _,_,err=call(0xa023,0,h);assert(err==0,'FREED DisposeHandle')
   end
   for i,h in ipairs(handles)do
    for _,trap in ipairs({0xa02a,0xa049})do
     local before=snapshot();local master=mem:readv_u32(h)
     local d,a,err=call(trap,0,h)
     assert(a==h,'FREED A0 changed')
     local after=snapshot();local unchanged=after==before
     local changed=0;for n=1,#before do if before:byte(n)~=after:byte(n)then changed=changed+1 end end
     print(string.format('FREED count=%d reverse=%d index=%d trap=%04X slot=%08X master=%08X preceding=%08X d0=%08X error=%04X ccr=%02X unchanged=%d changed=%d',count,reverse and 1 or 0,i,trap,h-zone,master==0 and 0 or master-zone,master==0 and 0 or mem:readv_u32(master-8),d,err,cpu.state.SR.value&31,unchanged and 1 or 0,changed))
     if not unchanged then
      -- Explicit fixture isolation: preserve the failed/corrupting observation,
      -- then restore private-zone bytes before testing the next call.
      for n=1,#before do mem:writev_u8(zone+n-1,before:byte(n))end
      assert(snapshot()==before,'FREED isolation restore')
      print('FREED_FIXTURE_ISOLATION_RESTORE')
     end
    end
   end
  end
 end
 call(0xa01b,0,savedZone);local _,_,err=call(0xa01f,0,zone);assert(err==0 and mem:readv_u32(0x118)==savedZone,'FREED restore zone')
 print('FREED_FIXTURE_RESTORED');print('FREED_FIXTURE_COMPLETE');finish();manager.machine:exit()
end
function M.start(address,done)
 code=address;finish=done;cpu.state.SP.value=code-4096
 worker=coroutine.create(run);local ok,err=coroutine.resume(worker);assert(ok,err)
end
function M.poll()
 if not worker or coroutine.status(worker)=='dead' or dbg.execution_state~='stop' or cpu.state.PC.value~=code+2 then return false end
 local ok,err=coroutine.resume(worker);assert(ok,err);return true
end
return M
