-- Read-only normal-quit contracts; the original handles Command-Q and cleanup.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger;local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local dispatcher,thePort,pending,disposeCaller;local disposed=0;local exits=0
local function ptr(a)return mem:readv_u32(a)&0xffffff end
local function dump(name,a,n)
 assert(a>=0x1000 and a+n<0x800000,'QUIT CAPTURE EXTENT')
 local f=assert(io.open(run..'/quit-'..name..'.bin','wb'))
 for i=0,n-1 do f:write(string.char(mem:readv_u8(a+i)))end
 f:close()
end
function M.arm(meta,condition)
 dispatcher=meta.dispatcher.address
 cpu.debug:bpset(dispatcher,condition..' && w@(d@(sp+2))==0xa047','logerror "QUIT_PATCH target=%04X address=%08X caller=%08X\\n",d0,a0,d@(sp+2);g')
 cpu.debug:bpset(dispatcher,condition..' && (w@(d@(sp+2))==0xa86e || w@(d@(sp+2))==0xa914 || w@(d@(sp+2))==0xa9f4)','logerror "QUIT_STOP\\n"')
end
function M.map(seg,base,guard)
 if seg==2 then guard(seg,base,0x3e36,2);disposeCaller=base+0x3e36 end
end
function M.audio()
 local a5=cpu.state.A5.value&0xffffff;local driver=ptr(a5-0xf24)
 assert(mem:readv_u32(driver)==0x202f0004 and mem:readv_u32(driver+4)==0x222f0008 and mem:readv_u16(driver+0xec)==0x4e75,'QUIT AUDIO ENTRY')
 for _,alias in ipairs({driver,driver|0x80000000})do
  cpu.debug:bpset(alias,'d@(sp+4)==0x8','logerror "QUIT_AUDIO_ENTER selector=%X argument=%08X caller=%08X\\n",d@(sp+4),d@(sp+8),d@sp;g')
  cpu.debug:bpset(alias+0xec,'d@(sp+4)==0x8','logerror "QUIT_AUDIO_RETURN result=%08X argument=%08X\\n",d0,d@(sp+8);g')
 end
end
function M.poll()
 if dbg.execution_state~='stop' then return false end
 local pc=cpu.state.PC.value
 if pending and pc==pending.pc then
  local p=pending;assert(cpu.state.SP.value==p.sp+4,'QUIT PASCAL STACK')
  cpu.debug:bpclear(p.bp);pending=nil
  if p.trap==0xa914 and p.original then
   disposed=disposed+1
   print(string.format('QUIT_DISPOSE_RETURN count=%d current=%08X disposed=%08X sr=%04X',disposed,ptr(thePort),p.window,cpu.state.SR.value))
   dump('port-after',ptr(thePort),108)
  end
  dbg.execution_state='run';return true
 end
 if pc~=dispatcher then return false end
 local sp=cpu.state.SP.value;local caller=ptr(sp+2);local trap=mem:readv_u16(caller)
 if trap~=0xa86e and trap~=0xa914 and trap~=0xa9f4 then return false end
 assert(not pending,'QUIT NESTED CALL')
 if trap==0xa9f4 then
  exits=exits+1;print(string.format('QUIT_EXIT_TO_SHELL count=%d caller=%08X',exits,caller))
 else
  local args=sp+8
  local p={pc=caller+2,sp=args,trap=trap,original=caller==disposeCaller}
  if trap==0xa86e then thePort=ptr(args)
  else
   assert(thePort,'QUIT INITGRAF MISSING');p.window=ptr(args)
   print(string.format('QUIT_DISPOSE_ENTER window=%08X current=%08X sr=%04X',p.window,ptr(thePort),cpu.state.SR.value))
   if p.original then dump('window-before',p.window,156);dump('stack-before',args,16)end
  end
  p.bp=cpu.debug:bpset(p.pc,string.format('sp==0x%x',args+4),'logerror "QUIT_STOP\\n"');pending=p
 end
 dbg.execution_state='run';return true
end
function M.finish()
 assert(not pending and disposed==1 and exits==1,'QUIT CLEANUP INCOMPLETE')
 print('QUIT_REFERENCE_COMPLETE Finder')
end
return M
