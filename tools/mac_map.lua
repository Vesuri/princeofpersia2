-- Optional static/live-map observer. All writes are made by original instructions.
local M={};local run=os.getenv('POP2_REFERENCE_RUN')
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local meta,cond,first,base1;local stops,seen,watches,handles={},{},{},{}
local function u(a)return mem:readv_u32(a)end
local function ptr(a)return u(a)&0xffffff end
local function dump(name,base,size)
 local f=assert(io.open(run..'/'..name,'wb'))
 for i=0,size-1 do f:write(string.char(mem:readv_u8(base+i)))end
 f:close()
end
local function stop(addr,callback)
 local id=cpu.debug:bpset(addr,cond,'logerror "MAP_STOP\\n"')
 stops[addr]={id=id,callback=callback}
end
local function watch(label,base,size)
 if not M.delta then return end
 for i=0,size-1 do assert(mem:read_u8(base+M.delta+i)==mem:readv_u8(base+i),'PHYSICAL MAPPING')end
 if watches[label] then cpu.debug:wpclear(watches[label])end
 watches[label]=cpu.debug:wpset(mem,'w',base+M.delta,size,'1',
  'logerror "CODEWRITE '..label..' pc=%08X addr=%08X size=%X data=%08X\\n",pc,wpaddr,wpsize,wpdata;g')
 print(string.format('WATCH %s base=%08X size=%d',label,base,size))
end
function M.map(seg,base,handle)
 if handle then handles[seg]=handle end
 local key=seg..':'..base;if seen[key]then return end;seen[key]=true
 local info=assert(meta.segments[seg]);local size=#info.hex/2
 for i=0,size-1 do assert(mem:readv_u8(base+i)==tonumber(info.hex:sub(2*i+1,2*i+2),16),'MAP ORIGINAL BYTES')end
 dump('map-CODE_'..seg..'-before.bin',base,size)
 watch('CODE_'..seg,base,size)
 print(string.format('LIVE_MAP seg=%d base=%08X size=%d',seg,base,size))
 if seg==2 then
  stop(base+0x3e90,function()
   local a5=cpu.state.A5.value
   dump("a5-core.bin",a5-22266,22266)
   dump("a5-above.bin",a5,8176)
   print(string.format("CORE_INITIALIZED a5=%08X pc=%08X",a5,cpu.state.PC.value))
  end)
  for _,off in ipairs({0x2efe,0x449e,0x5620})do
   cpu.debug:bpset(base+off,cond,'logerror "CACHED_GETKEYS pc=%08X target=%08X slot=%08X\\n",pc,a0,d@(a5-0x1f68);g')
  end
 end
 if seg==19 then
  stop(base+0x4ce2,function()print(string.format("CORE_RESOURCE type=5F535444 handle=%08X",u(cpu.state.SP.value)))end)
  stop(base+0x4cfe,function()print(string.format("CORE_RESOURCE type=5F535449 handle=%08X",u(cpu.state.SP.value)))end)
 end
 if seg==16 then
  cpu.debug:bpset(base+0x2840,cond,'logerror "GETKEYS_INSTALLED target=%08X\\n",d@(a5-0x1f68);g')
 end
 if seg==11 then
  stop(base+0x21d2,function()
   local handle=cpu.state.A4.value;local data=ptr(handle)
   print(string.format('DECRYPT_BEGIN handle=%08X base=%08X',handle,data))
   M.driverbase=data
  end)
  stop(base+0x21e4,function()
   local data=M.driverbase;local size=(cpu.state.A2.value&0xffffff)-data
   assert(size>7 and size<1000000,'MDRV EXTENT')
   M.driversize=size;dump('MDRV-encrypted.bin',data,size);watch('MDRV',data,size)
   print(string.format('DECRYPT_EXTENT base=%08X size=%d',data,size))
  end)
  stop(base+0x3a16,function()
   M.expandedhandle=cpu.state.A3.value;M.expanded=ptr(M.expandedhandle);M.expandedsize=u(cpu.state.A6.value-4)
   assert(M.expandedsize>0 and M.expandedsize<1000000,'EXPANDED EXTENT')
   watch('MDRV_EXPANDED',M.expanded,M.expandedsize)
  end)
  stop(base+0x3a34,function()
   dump('MDRV-expanded.bin',M.expanded,M.expandedsize);print('MDRV_EXPANDED_COMPLETE')
  end)
  stop(base+0x3a3e,function()
   cpu.debug:wpclear(watches.MDRV);watches.MDRV=nil;print('MDRV_ENCRYPTED_RETIRED')
  end)
  stop(base+0x3290,function()
   cpu.debug:wpclear(watches.MDRV_EXPANDED);watches.MDRV_EXPANDED=nil
   print('MDRV_EXPANSION_RETIRED')
  end)
  stop(base+0x32a6,function()
   M.expanded=ptr(M.expandedhandle)
   dump('MDRV-installed.bin',M.expanded,M.expandedsize)
   watch('MDRV_INSTALLED',M.expanded,M.expandedsize)
   print(string.format('MDRV_INSTALLED base=%08X size=%d',M.expanded,M.expandedsize))
  end)
  stop(base+0x21f6,function()
   dump('MDRV-decrypted.bin',M.driverbase,M.driversize)
   print('DECRYPT_COMPLETE')
  end)
 end
end
function M.arm(metadata,condition)
 meta=metadata;cond=condition
 first=cpu.debug:bpset(meta.dispatcher.address,cond..' && w@(d@(sp+2))==0xa9a0 && d6==0x53545253','logerror "TRAP pc=%08X trap=%04X d0=%08X a0=%08X a1=%08X a5=%08X sp=%08X\\n",d@(sp+2),w@(d@(sp+2)),d0,a0,a1,a5,sp')
end
function M.poll()
 if not meta or dbg.execution_state~='stop'then return false end
 local pc=cpu.state.PC.value
 if first and pc==meta.dispatcher.address then
  cpu.debug:bpclear(first);first=nil
  local caller=ptr(cpu.state.SP.value+2);base1=caller-0x2ea
  for i=0,15 do assert(mem:readv_u8(base1+i)==tonumber(meta.segments[1].hex:sub(2*i+1,2*i+2),16),'CODE1 INITIAL GUARD')end
  M.tap=mem:install_write_tap(0,0x7fffff,'write-control',function(a,d,mask)
   local pc=cpu.state.PC.value&0xffffff
   if not M.delta and pc==base1+0x26 then
    assert(mask==0xffffffff and d==cpu.state.A4.value,'WRITE CALIBRATION')
    M.delta=a-(base1+4)
    print(string.format('WRITE_CONTROL pc=%08X addr=%08X delta=%08X',pc,a,M.delta))
   end
  end)
  M.map(1,base1)
  dump("lowmem-start.bin",0,0x2000)
  stop(base1+0x2a,function()
   assert(M.delta,'WRITE CONTROL NOT REACHED');M.tap:remove()
   watch('CODE_1',base1,#meta.segments[1].hex/2)
   local a5=cpu.state.A5.value;local strs=u(base1+4)
   dump('a5-initial.bin',a5-22266,22266)
   print(string.format('A5_INITIAL a5=%08X strs=%08X',a5,strs))
  end)
  stop(base1+0x3c2,function()
   M.stubbase=cpu.state.A1.value;watch('STUB',M.stubbase,24)
  end)
  stop(base1+0x402,function()dump('runtime-stubs.bin',M.stubbase,24);print('STUBS_COMPLETE')end)
  stop(base1+0x7c,function()
   local seg=cpu.state.D7.value&0xffff;local h=ptr(cpu.state.SP.value)
   assert(h~=0,'LOADSEG RESOURCE');M.map(seg,ptr(h),h)
  end)
  dbg.execution_state='run';return true
 end
 local s=stops[pc]
 if s then s.callback();dbg.execution_state='run';return true end
 return false
end
function M.finish()
 if M.expanded then dump('MDRV-final.bin',M.expanded,M.expandedsize)end
 for key,_ in pairs(seen)do
  local seg,base=key:match('^(%d+):(%d+)$');seg=tonumber(seg);base=tonumber(base)
  if handles[seg] then base=ptr(handles[seg])end
  dump('map-CODE_'..seg..'-after.bin',base,#meta.segments[seg].hex/2)
 end
 print('LIVE_MAP_COMPLETE')
end
return M
