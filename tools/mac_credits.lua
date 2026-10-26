-- Credits page in the second natural attract pass (item 23); observation only.
-- The sites match the native credits observer: entry, each GetPenState
-- record, each displayed page, the credits return, attract cutscenes 7, 4
-- and 8, the attract return and the following completed demo frames.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger;local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local frame=dofile('tools/mame_mac_frame.lua')
local points={};local pages=0;local pens=0;local nis=0;local after=0;local complete=false
local function dump(path,address,size)
 local f=assert(io.open(run..'/'..path,'wb'))
 for i=0,size-1 do f:write(string.char(mem:readv_u8(address+i)))end
 f:close()
end
local function capture(prefix)
 frame.indexed(run..'/'..prefix..'.indices');frame.palette(run..'/'..prefix..'.rgb')
 dump(prefix..'-a5.bin',(cpu.state.A5.value&0xffffff)-22266,22266)
end
function M.map(seg,base,guard,condition)
 -- Timing-cause observations in the attract demo: Random results around the
 -- step-265 room-two call, and PrinceSndOn results that start the death countdown.
 if seg==17 then
  for _,offset in ipairs({0x1c4e,0x6e1e})do
   guard(seg,base,offset,2)
   cpu.debug:bpset(base+offset,condition..' && w@(a5-0x349a)>=0xff && w@(a5-0x349a)<=0x113',
    string.format('logerror "DEMO RANDOM step=%%d site=%04x result=%%04x seed=%%08x caller=%%08x\\n",w@(a5-0x349a),w@sp,d@(a5-0x18fc),d@(a6+4);g',offset))
  end
 end
 if seg==4 then
  for _,offset in ipairs({0x3c6a,0x3c78})do
   guard(seg,base,offset,2)
   cpu.debug:bpset(base+offset,condition,
    string.format('logerror "DEMO SOUNDON step=%%d site=%04x result=%%04x\\n",w@(a5-0x349a),d0&0xffff;g',offset))
  end
 end
 if seg~=2 then return end
 for _,site in ipairs({{0x2b70,'enter',4},{0x2da8,'pen',2},{0x2c20,'page',2},{0x2d02,'return',2},
                       {0x475c,'nis-7',2},{0x476e,'nis-4',2},{0x4780,'nis-8',2},{0x4576,'attract',2},{0x68b2,'frame',2}})do
  guard(seg,base,site[1],site[3]);points[base+site[1]]=site[2]
  cpu.debug:bpset(base+site[1],condition,'logerror "CREDITS_STOP\\n"')
 end
end
function M.poll()
 if dbg.execution_state~='stop' then return false end
 local kind=points[cpu.state.PC.value]
 if not kind then return false end
 local a5=cpu.state.A5.value&0xffffff
 if kind=='enter' then
  print(string.format('CREDITS ENTER level=%d minutes=%d',mem:readv_u16(a5-0x4faa),mem:readv_u16(a5-0x4fce)))
 elseif kind=='pen' then
  pens=pens+1;local p=(cpu.state.A6.value&0xffffff)-0x14
  print(string.format('CREDITS PENSTATE %d loc=%04x,%04x size=%04x,%04x mode=%04x pat=%08x%08x',pens,
   mem:readv_u16(p),mem:readv_u16(p+2),mem:readv_u16(p+4),mem:readv_u16(p+6),mem:readv_u16(p+8),mem:readv_u32(p+10),mem:readv_u32(p+14)))
 elseif kind=='page' then
  pages=pages+1
  print(string.format('CREDITS PAGE %d d5=%d d6=%d',pages,cpu.state.D5.value&0xffff,cpu.state.D6.value&0xffff))
  capture('credits-page-'..pages)
 elseif kind=='return' then
  print(string.format('CREDITS RETURN result=%d pages=%d pens=%d',cpu.state.D6.value&0xffff,pages,pens))
 elseif kind:match('^nis%-') then
  nis=nis+1
  print(string.format('ATTRACT NIS %s result=%d',kind:sub(5),cpu.state.D0.value&0xffff))
 elseif kind=='attract' then
  print(string.format('ATTRACT RETURN result=%d level=%d pages=%d nis=%d',cpu.state.D0.value&0xffff,mem:readv_u16(a5-0x4faa),pages,nis))
  if pages>0 then after=1 end
 elseif kind=='frame' and pages==0 then
  print(string.format('DEMO FRAME step=%d seed=%08x level=%d room=%d clock=%d',mem:readv_u16(a5-0x349a),mem:readv_u32(a5-0x18fc),
   mem:readv_u16(a5-0x4fd2),mem:readv_u16(a5-0x4fc2),mem:readv_u16(a5-0x4fca)))
 elseif kind=='frame' and after>0 and not complete then
  after=after+1
  if after==3 then
   print(string.format('CREDITS CYCLE COMPLETE level=%d room=%d',mem:readv_u16(a5-0x4fd2),mem:readv_u16(a5-0x4fc2)))
   capture('credits-after');complete=true
  end
 end
 dbg.execution_state='run';return true
end
function M.complete()return complete end
return M
