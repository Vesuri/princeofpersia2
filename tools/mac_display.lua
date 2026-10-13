-- Read-only paired QuickDraw observer. Captures are local evidence, not assets.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local meta,dispatcher,pending;local serial=0
local thePort
local bindings={}
local traps={0xa86e,0xa912,0xaa2a,0xaa29,0xaa46,0xa873,0xa874,0xa8ec,
 0xa915,0xa916,0xa91b,0xab1d,0xaa90,0xaa91,0xaa92,0xaa94,0xaa95,0xaa96,0xaa99,0xaa9a,
 0xaaa0,0xaa9b,0xaa9c,0xaa9d,0xaa9e,0xaaa2,0xaa03,0xaa14,0xaa15}
local selected={};for _,t in ipairs(traps)do selected[t]=true end
local function u(a)return mem:readv_u32(a)end
local function ptr(a)return u(a)&0xffffff end
local function dump(label,address,size,physical)
 assert(size>=0 and size<=1024*1024,'DISPLAY CAPTURE EXTENT')
 local f=assert(io.open(run..'/display-'..serial..'-'..label..'.bin','wb'))
 local bytes={}
 for i=0,size-1 do
  bytes[#bytes+1]=string.char(physical and mem:read_u8(address+i) or mem:readv_u8(address+i))
  if #bytes==4096 then f:write(table.concat(bytes));bytes={}end
 end
 f:write(table.concat(bytes));f:close()
 print(string.format('DISPLAY_DUMP id=%d label=%s address=%08X size=%d',serial,label,address,size))
end
local function map(label,address)
 if address==0 then return end
 if mem:readv_u16(address+4)&0xc000==0xc000 then
  dump(label..'-port',address-2,108);address=ptr(ptr(address))
 end
 dump(label,address,50)
 local stride=mem:readv_u16(address+4)&0x3fff
 local height=mem:readv_i16(address+10)-mem:readv_i16(address+6)
 if mem:readv_u16(address+4)&0x8000~=0 then
  local colors=ptr(address+42)
  if colors~=0 and ptr(colors)~=0 then
   local body=ptr(colors);local count=mem:readv_u16(body+6)+1
   assert(count<=256,'DISPLAY CLUT SIZE');dump(label..'-colors',body,8+count*8)
  end
 end
 if stride>0 and height>0 and stride*height<=1024*1024 then
  local pixels=u(address)
  if mem:readv_u16(address+14)==2 and pixels<0x800000 then pixels=ptr(pixels)end
  dump(label..'-pixels',pixels,stride*height,pixels>=0x800000)
 end
end
local function state(phase,p)
 dump(phase..'-stack',cpu.state.SP.value+(phase=='before' and 8 or 0),64)
 local gd=ptr(0x8a4)
 if gd~=0 then
  local body=ptr(gd);dump(phase..'-device',body,62)
  local pm=ptr(body+22)
  if pm~=0 then
   local body=ptr(pm);dump(phase..'-screen-map',body,50)
   dump(phase..'-screen-colors',ptr(ptr(body+42)),2056)
  end
 end
 if not p.palette then for _,bound in pairs(bindings)do p.palette=bound;break end end
 if p.palette and p.palette~=0 then
  local h=p.palette&0xffffff;local body=ptr(h)
  if body~=0 then dump(phase..'-palette',body,16+16*mem:readv_u16(body))end
 end
 if p.trap==0xaa91 and phase=='after' then
  local h=ptr(cpu.state.SP.value);dump('new-palette',ptr(h),16+16*mem:readv_u16(ptr(h)))
 end
 if p.trap==0xaa46 and phase=='after' then
  p.window=ptr(cpu.state.SP.value)
 end
 if p.window and p.window~=0 then
  dump(phase..'-window',p.window,156)
  map(phase..'-window-map',ptr(ptr(p.window+2)))
 end
 if thePort and ptr(thePort)~=0 then dump(phase..'-port',ptr(thePort),108)end
 if p.trap==0xaaa0 then
  local h=p.args[1]&0xffffff;local body=ptr(h)
  dump(phase..'-converted-colors',body,8+8*(mem:readv_u16(body+6)+1))
 end
 if p.trap==0xaa9c then dump(phase..'-entry-rgb',p.args[1]&0xffffff,6)end
 if p.trap==0xaa9a then
  dump(phase..'-animation',p.colors,8+8*(p.source+p.count))
  local palette=assert(manager.machine.palettes[':nbc:mdc48']);local colors={}
  for i=0,255 do local c=palette:pen_color(i);colors[#colors+1]=string.char((c>>16)&255,(c>>8)&255,c&255)end
  local f=assert(io.open(run..'/display-'..serial..'-'..phase..'-video-rgb.bin','wb'));f:write(table.concat(colors));f:close()
 end
 if p.trap==0xa86e then dump(phase..'-globals',p.args[1]-202,206)end
 if p.trap==0xa8ec then
  map(phase..'-source',p.args[5]);map(phase..'-destination',p.args[6])
  dump(phase..'-from',p.from,8);dump(phase..'-to',p.to,8)
  local port=ptr(thePort)
  dump(phase..'-vis',ptr(ptr(port+24))+2,8);dump(phase..'-clip',ptr(ptr(port+28))+2,8)
 elseif p.trap==0xab1d and p.selector==0 then
  dump(phase..'-bounds',p.bounds,8)
  if phase=='after' then
   local port=ptr(p.output)
   dump('new-port',port,108);map('new-map',ptr(ptr(port+2)))
  end
 end
end
function M.arm(metadata,condition)
 meta=metadata;dispatcher=meta.dispatcher.address
 local terms={};for _,trap in ipairs(traps)do terms[#terms+1]=string.format('w@(d@(sp+2))==0x%x',trap)end
 cpu.debug:bpset(dispatcher,condition..' && (d@(sp+2)&0xffffff)>=0x100000 && (d@(sp+2)&0xffffff)<0x800000 && ('..table.concat(terms,' || ')..')','logerror "DISPLAY_STOP\\n"')
end
function M.poll()
 if not dispatcher or dbg.execution_state~='stop'then return false end
 local pc=cpu.state.PC.value
 if pending and pc==pending.returnPC then
  local p=pending;cpu.debug:bpclear(p.probe)
  state('after',p)
  print(string.format('DISPLAY_RETURN id=%d sp=%08X d0=%08X a0=%08X a1=%08X',serial,cpu.state.SP.value,cpu.state.D0.value,cpu.state.A0.value,cpu.state.A1.value))
  pending=nil;dbg.execution_state='run';return true
 end
 if pc~=dispatcher then return false end
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
 assert(not pending,'DISPLAY NESTED ORIGINAL CALL')
 print(string.format('TRAP pc=%08X trap=%04X d0=%08X a0=%08X a1=%08X a5=%08X sp=%08X',caller,trap,cpu.state.D0.value,cpu.state.A0.value,cpu.state.A1.value,cpu.state.A5.value,sp))
 serial=serial+1
 local p={trap=trap,selector=cpu.state.D0.value&0xffff,returnPC=caller+2,stack=sp+8,args={}}
 p.bounds=u(p.stack+12)&0xffffff;p.output=u(p.stack+18)&0xffffff
 for i=1,8 do p.args[i]=u(p.stack+4*(i-1))end
 if trap==0xa915 or trap==0xa916 then p.window=p.args[1]&0xffffff end
 if trap==0xa91b then p.window=u(p.stack+6)&0xffffff end
 if trap==0xa86e then thePort=p.args[1]&0xffffff end
 if trap==0xaaa0 then p.palette=u(p.stack+4)end
 if trap==0xaa9c or trap==0xaa9e then p.palette=u(p.stack+6)end
 if trap==0xaa95 then p.window=u(p.stack+6)&0xffffff;p.palette=u(p.stack+2);bindings[u(p.stack+6)&0xffffff]=p.palette end
 if trap==0xaa9a then
  p.palette=bindings[u(p.stack+10)&0xffffff];p.colors=ptr(u(p.stack+6)&0xffffff)
  p.source=mem:readv_u16(p.stack+4);p.count=mem:readv_u16(p.stack)
 end
 -- CopyBits has a word-sized mode between its mask and rectangle pointers.
 if trap==0xa8ec then p.args[5]=u(p.stack+18);p.args[6]=u(p.stack+14);p.from=u(p.stack+10)&0xffffff;p.to=u(p.stack+6)&0xffffff end
 print(string.format('DISPLAY_CALL id=%d pc=%08X trap=%04X selector=%04X sp=%08X',serial,caller,trap,p.selector,p.stack))
 state('before',p)
 p.probe=cpu.debug:bpset(p.returnPC,'1','logerror "DISPLAY_STOP\\n"')
 pending=p;dbg.execution_state='run';return true
end
function M.finish()
 assert(not pending and serial>0,'DISPLAY TRACE INCOMPLETE')
 print(string.format('DISPLAY_TRACE_COMPLETE calls=%d',serial))
end
return M
