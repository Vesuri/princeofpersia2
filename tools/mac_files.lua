-- Optional read-only File Manager observer. Raw evidence stays in the run directory.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local dispatcher,condition,entryProbe,pending,metadata
local serial=0
local function u(a)return mem:readv_u32(a)end
local function bytes(a,n)
 assert(n>=0 and n<=1048576,'FILE TRACE EXTENT')
 local out={}
 for i=0,n-1 do out[#out+1]=string.char(mem:readv_u8(a+i))end
 return table.concat(out)
end
local function save(label,data)
 local f=assert(io.open(run..'/file-'..serial..'-'..label..'.bin','wb'))
 f:write(data);f:close()
end
local function armEntry()
 entryProbe=cpu.debug:bpset(dispatcher,condition,'logerror "FILE_STOP\\n"')
end
function M.arm(meta,cond)
 metadata=meta
 dispatcher=meta.dispatcher.address
 -- Classic and hierarchical forms share parameter-block layouts. Async calls
 -- need a completion observer and deliberately stop this synchronous survey.
 local traps={0xa000,0xa200,0xa001,0xa201,0xa002,0xa003,0xa004,0xa005,
  0xa006,0xa007,0xa207,0xa008,0xa208,0xa009,0xa00c,0xa00d,0xa011,
  0xa012,0xa013,0xa014,0xa214,0xa015,0xa018,0xa044,0xa060,0xa260}
 local terms={}
 for _,t in ipairs(traps)do
  terms[#terms+1]=string.format('(w@(d@(sp+2))&0xfbff)==0x%x',t)
 end
 condition=cond..' && ('..table.concat(terms,' || ')..')'
 armEntry()
end
function M.poll()
 if not dispatcher or dbg.execution_state~='stop'then return false end
 local pc=cpu.state.PC.value
 if pending and pc==pending.returnPC then
  local p=pending
  cpu.debug:bpclear(p.probe)
  save('after',bytes(p.pb,128))
  local result=mem:readv_u16(p.pb+16)
  assert(result==(cpu.state.D0.value&0xffff),'FILE RESULT REGISTER')
  if p.trap==0xa002 or p.trap==0xa003 then
   local actual=u(p.pb+40)
   assert(actual<=p.request,'FILE ACTUAL COUNT')
   save('transfer',bytes(p.buffer,actual))
  end
  if p.outputName and result==0 then save('returned-name',bytes(p.outputName+1,mem:readv_u8(p.outputName)))end
  print(string.format('FILE_RETURN id=%d result=%04X',serial,result))
  pending=nil;dbg.execution_state='run';return true
 end
 if pc~=dispatcher then return false end
 local sp=cpu.state.SP.value
 local caller=u(sp+2)&0xffffff
 local trap=mem:readv_u16(caller)
 print(string.format('TRAP pc=%08X trap=%04X d0=%08X a0=%08X a1=%08X a5=%08X sp=%08X',caller,trap,cpu.state.D0.value,cpu.state.A0.value,cpu.state.A1.value,cpu.state.A5.value,sp))
 local original=false
 local a5=cpu.state.A5.value&0xffffff
 for seg,info in pairs(metadata.segments)do
  local e=a5+info.jt
  if mem:readv_u16(e)==seg and mem:readv_u16(e+2)==0x4ef9 then
   local base=(u(e+4)&0xffffff)-info.offset
   if caller>=base and caller<base+#info.hex/2 then original=true end
  end
 end
 if not original then dbg.execution_state='run';return true end
 assert((trap&0x0400)==0,'FILE ASYNC COMPLETION NOT OBSERVED')
 assert(not pending,'FILE NESTED ORIGINAL CALL')
 serial=serial+1
 local pb=cpu.state.A0.value&0xffffff
 local p={pb=pb,trap=trap,returnPC=caller+2}
 save('before',bytes(pb,128))
 local name=u(pb+18)&0xffffff
 local selector=cpu.state.D0.value&0xffff
 local named=trap==0xa000 or trap==0xa200 or trap==0xa004 or trap==0xa005
  or trap==0xa006 or trap==0xa008 or trap==0xa009 or trap==0xa00c or trap==0xa00d or ((trap==0xa060 or trap==0xa260) and selector==26)
 local hfs=trap==0xa060 or trap==0xa260
 if hfs and selector==9 and mem:readv_i16(pb+28)==0 then named=true end
 if name~=0 and (trap==0xa014 or trap==0xa214 or trap==0xa207 or trap==0xa00c or (hfs and (selector==8 or selector==9)))then p.outputName=name end
 if named and name~=0 then save('name',bytes(name+1,mem:readv_u8(name)))end
 if trap==0xa002 or trap==0xa003 then
  p.buffer=u(pb+32)&0xffffff;p.request=u(pb+36)
  if trap==0xa003 then save('requested',bytes(p.buffer,p.request))end
 end
 print(string.format('FILE_CALL id=%d pc=%08X trap=%04X selector=%04X pb=%08X',serial,caller,trap,cpu.state.D0.value&0xffff,pb))
 p.probe=cpu.debug:bpset(p.returnPC,'sp==0x'..string.format('%X',sp+8),'logerror "FILE_STOP\\n"')
 pending=p;dbg.execution_state='run';return true
end
function M.finish()
 assert(not pending and serial>0,'FILE TRACE INCOMPLETE')
 print(string.format('FILE_TRACE_COMPLETE calls=%d',serial))
end
return M
