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
 if seg==20 then
  -- The HandToHand that copies the 373,788-byte LoadSaveDisRes data.
  guard(seg,base,0x858,2);points[base+0x858]='h2h'
  cpu.debug:bpset(base+0x858,condition,'logerror "CREDITS_STOP\\n"')
 end
 if seg~=2 then return end
 for _,site in ipairs({{0x2b70,'enter',4},{0x2da8,'pen',2},{0x2c20,'page',2},{0x2d02,'return',2},
                       {0x475c,'nis-7',2},{0x476e,'nis-4',2},{0x4780,'nis-8',2},{0x4576,'attract',2},{0x68b2,'frame',2}})do
  guard(seg,base,site[1],site[3]);points[base+site[1]]=site[2]
  cpu.debug:bpset(base+site[1],condition,'logerror "CREDITS_STOP\\n"')
 end
end
-- Walk the application zone; accept the header layout that ends exactly at bkLim.
local function zone()
 local z=mem:readv_u32(0x2aa)&0xffffff;local limit=mem:readv_u32(z)&0xffffff
 for _,wide in ipairs({true,false})do
  local p=z+52;local s={free=0,largest=0,blocks=0,rel=0,nonrel=0,locked=0,purgeable=0,relbytes=0,nonrelbytes=0,lockedbytes=0,purgebytes=0}
  local ok=true
  while p<limit do
   local tag,size,flags
   if wide then tag=mem:readv_u8(p)>>6;size=mem:readv_u32(p+4);flags=mem:readv_u8(p+1)
   else local h=mem:readv_u32(p);tag=h>>30;size=h&0xffffff;flags=0
    if tag==2 then flags=mem:readv_u8(z+(mem:readv_u32(p+4)&0xffffff))end end
   if size<8 or size&3~=0 or p+size>limit then ok=false;break end
   s.blocks=s.blocks+1
   if tag==0 then s.free=s.free+size;if size>s.largest then s.largest=size end
   elseif tag==1 then s.nonrel=s.nonrel+1;s.nonrelbytes=s.nonrelbytes+size
   elseif tag==2 then s.rel=s.rel+1;s.relbytes=s.relbytes+size
    if flags&0x80~=0 then s.locked=s.locked+1;s.lockedbytes=s.lockedbytes+size
    elseif flags&0x40~=0 then s.purgeable=s.purgeable+1;s.purgebytes=s.purgebytes+size end
   else ok=false;break end
   p=p+size
  end
  if ok and p==limit then return z,limit,wide,s end
 end
 return z,limit,nil,nil
end
function M.poll()
 if dbg.execution_state~='stop' then return false end
 local kind=points[cpu.state.PC.value]
 if not kind then return false end
 local a5=cpu.state.A5.value&0xffffff
 if kind=='h2h' then
  local z,limit,wide,st=zone();local h=cpu.state.A0.value&0xffffff
  assert(st,'CREDITS ZONE WALK')
  print(string.format('CREDITS ZONE HANDTOHAND zone=%X bytes=%d header=%d source=%X blocks=%d free=%d largest=%d rel=%d/%d locked=%d/%d purgeable=%d/%d nonrel=%d/%d',
   z,limit-z,wide and 12 or 8,mem:readv_u32(h)&0xffffff,st.blocks,st.free,st.largest,st.rel,st.relbytes,st.locked,st.lockedbytes,st.purgeable,st.purgebytes,st.nonrel,st.nonrelbytes))
 elseif kind=='enter' then
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
