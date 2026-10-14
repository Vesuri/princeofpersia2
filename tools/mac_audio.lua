-- Read-only MIDI Synth 3.45 ABI observer; all output stays in the reference run.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local run=assert(os.getenv('POP2_REFERENCE_RUN'))
local install,probe,driver,death,deathProbe
local fixture,fixtureReturn,fixtureProbe,fixtureStack,fixtureIndex,fixtureCases
local callbackOrder=os.getenv('POP2_AUDIO_CALLBACK_ORDER')=='1'
local function dump(path,address,size)
 local f=assert(io.open(run..'/'..path,'wb'))
 for i=0,size-1 do f:write(string.char(mem:readv_u8(address+i)))end
 f:close()
end
local callAction,payloads= nil,0
local driverHex
local dead=false
function M.dead()return dead end
local function u(a)return mem:readv_u32(a)end
function M.map(seg,base,guard,metadata)
 driverHex=assert(metadata.driverHex)
 if seg==5 then
  guard(5,base,0x4afc,8)
  death=base+0x4afc
  deathProbe=cpu.debug:bpset(death,'1','logerror "AUDIO_DEATH_STOP\\n"')
 end
 if seg~=11 then return end
 guard(11,base,0x32aa,6)
 install=base+0x32aa
 probe=cpu.debug:bpset(install,'1','logerror "AUDIO_INSTALL_STOP\\n"')
end
local function fixtureCall()
 local row=fixtureCases[fixtureIndex];assert(row,'AUDIO FIXTURE CASE')
 local args=row.args or fixtureStack+128
 for i,v in ipairs(row)do if i>1 then mem:writev_u16(args+2*(i-2),v)end end
 mem:writev_u32(fixtureStack,fixtureReturn);mem:writev_u32(fixtureStack+4,row[1]);mem:writev_u32(fixtureStack+8,args)
 dump('audio-fixture-before-'..fixtureIndex..'.bin',driver+0x4600,0x32f0)
 cpu.state.SP.value=fixtureStack;cpu.state.PC.value=row[1]==256 and driver+0x20da or driver
 if row[1]==256 then cpu.state.A4.value=driver+0x4600 end
 dbg.execution_state='run'
end
function M.startFixture(checkpoint)
 assert(driver and not fixture,'AUDIO FIXTURE START')
 fixture=true;fixtureReturn=checkpoint;fixtureStack=(cpu.state.SP.value&0xffffff)-4096;fixtureIndex=1
 if callbackOrder then
  -- Authored fixture: replacing an active effect must run its callback before
  -- reading the replacement descriptor. The callback changes that descriptor.
  local old,new,query,pcm,callback=fixtureStack+128,fixtureStack+192,fixtureStack+256,fixtureStack+384,fixtureStack+512
  for i=0,63 do mem:writev_u8(old+i,0);mem:writev_u8(new+i,0);mem:writev_u8(query+i,0);mem:writev_u8(pcm+i,0x80)end
  for _,packet in ipairs({old,new})do
   mem:writev_u32(packet,pcm);mem:writev_u32(packet+4,64);mem:writev_u32(packet+8,0x2b110000)
  end
  mem:writev_u16(old+24,3);mem:writev_u32(old+30,callback);mem:writev_u32(old+34,new)
  mem:writev_u16(new+24,5);mem:writev_u16(query+24,7)
  -- move.l 4(sp),a0; move.w #7,24(a0); addq.l #1,40(a0); rts.
  local words={0x206f,0x0004,0x317c,0x0007,0x0018,0x52a8,0x0028,0x4e75}
  for i,v in ipairs(words)do mem:writev_u16(callback+2*(i-1),v)end
  fixtureCases={{22},{17,args=old},{17,args=new},{20,args=query},{20,args=query},{22}}
 else
  fixtureCases={{6}}
 for note=60,65 do fixtureCases[#fixtureCases+1]={9,0,note,20000,80,0}end
 fixtureCases[#fixtureCases+1]={12,0,62,0,0,0}
 fixtureCases[#fixtureCases+1]={9,0,66,20000,80,0}
 fixtureCases[#fixtureCases+1]={9,0,63,20000,80,0}
 fixtureCases[#fixtureCases+1]={12,0,63,0,0,0}
 fixtureCases[#fixtureCases+1]={9,0,63,20000,80,0}
 fixtureCases[#fixtureCases+1]={38}
 fixtureCases[#fixtureCases+1]={6};fixtureCases[#fixtureCases+1]={9,0,60,20000,80,0}
 for i=1,3 do fixtureCases[#fixtureCases+1]={256}end
 fixtureCases[#fixtureCases+1]={12,0,60,0,0,0}
 for i=1,8 do fixtureCases[#fixtureCases+1]={256}end
 end
 -- Allocation fixture only: mask interrupts so sample completion cannot
 -- turn an occupied voice into a free one between adjacent calls.
 cpu.state.SR.value=cpu.state.SR.value|0x700
 fixtureProbe=cpu.debug:bpset(fixtureReturn,string.format('sp==0x%x',fixtureStack+4),'logerror "AUDIO_FIXTURE_RETURN\\n"')
 print('AUDIO_FIXTURE_STARTED irq=masked');fixtureCall()
end
function M.poll()
 if driver and dbg.execution_state=='stop' and (cpu.state.PC.value&0xffffff)==driver
    and u(cpu.state.SP.value+4)==17 then
  local packet=u(cpu.state.SP.value+8)&0xffffff
  local source=u(packet)&0xffffff;local size=u(packet+4)
  assert(source>0x100000 and size>0 and size<=2097152,'AUDIO EFFECT EXTENT')
  payloads=payloads+1;assert(payloads<=(os.getenv('POP2_ROUTE_AUDIO_TRACE') and 512 or 128),'AUDIO EFFECT CAPACITY')
  dump('audio-effect-'..payloads..'.bin',source,size)
  dump('audio-packet-'..payloads..'.bin',packet,38)
  print(string.format('AUDIO_PAYLOAD index=%d source=%08X size=%d',payloads,source,size))
  dbg:command(callAction);return true
 end
 if fixture and dbg.execution_state=='stop' and cpu.state.PC.value==fixtureReturn then
  local row=fixtureCases[fixtureIndex]
  print(string.format('AUDIO_FIXTURE_RESULT index=%d selector=%d note=%d d0=%08X',fixtureIndex,row[1],row[3] or 0,cpu.state.D0.value))
  dump('audio-fixture-after-'..fixtureIndex..'.bin',driver+0x4600,0x32f0)
  if callbackOrder then
   local new,query=fixtureStack+192,fixtureStack+256
   if fixtureIndex>=3 then
    assert(mem:readv_u16(new+24)==7 and u(new+40)==1,'AUDIO CALLBACK ORDER MUTATION')
   end
   if fixtureIndex==4 then
    assert(cpu.state.D0.value==0,'AUDIO CALLBACK ORDER NEW ID NOT PLAYING')
    mem:writev_u16(query+24,5)
   elseif fixtureIndex==5 then
    assert(cpu.state.D0.value==1,'AUDIO CALLBACK ORDER OLD ID STILL PLAYING')
   end
   print(string.format('AUDIO_CALLBACK_ORDER index=%d id=%d callbacks=%d result=%08X',fixtureIndex,mem:readv_u16(new+24),u(new+40),cpu.state.D0.value))
  end
  fixtureIndex=fixtureIndex+1
  if fixtureIndex>#fixtureCases then
   cpu.debug:bpclear(fixtureProbe);print((callbackOrder and 'AUDIO_CALLBACK_ORDER_COMPLETE calls=' or 'AUDIO_FIXTURE_COMPLETE calls=')..#fixtureCases)
   manager.machine:exit();dbg.execution_state='run'
  else fixtureCall()end
  return true
 end
 if death and dbg.execution_state=='stop' and cpu.state.PC.value==death then
  cpu.debug:bpclear(deathProbe);death=nil;dead=true
  print(string.format('AUDIO_DEATH kind=%04X tick=%08X',mem:readv_u16(cpu.state.SP.value+4),u(0x16a)))
  print('AUDIO_PHASE death');dbg.execution_state='run';return true
 end
 if not install or dbg.execution_state~='stop' or cpu.state.PC.value~=install then return false end
 cpu.debug:bpclear(probe);install=nil
 driver=u((cpu.state.A5.value&0xffffff)-0xf24)&0xffffff
 assert(u(driver)==0x202f0004 and u(driver+4)==0x222f0008 and u(driver+8)==0x48e73ffe,'AUDIO DRIVER ENTRY')
 assert(mem:readv_u16(driver+0xec)==0x4e75,'AUDIO DRIVER RETURN')
 for i=0,#driverHex/2-1 do
  local expected=tonumber(driverHex:sub(i*2+1,i*2+2),16)
  if os.getenv('POP2_REFERENCE_CONTROL')=='bad-audio-guard' and i==0 then expected=expected~1 end
  assert(mem:readv_u8(driver+i)==expected,'AUDIO PROBE BYTES')
 end
 local f=assert(io.open(run..'/audio-driver.bin','wb'))
 for i=0,30959 do f:write(string.char(mem:readv_u8(driver+i)))end
 f:close()
 print(string.format('AUDIO_INSTALLED base=%08X size=30960',driver))
 local packet='(d@(sp+4)==0x9 || d@(sp+4)==0xc || d@(sp+4)==0x1b || d@(sp+4)==0x11 || d@(sp+4)==0x12 || d@(sp+4)==0x14 || d@(sp+4)==0x15 || d@(sp+4)==0x17 || d@(sp+4)==0x25)'
 local clockAddress=string.format('0x%x',driver+0x6096)
 local mixAddress=string.format('0x%x',driver+0x59a2)
 local effect='(d@(sp+4)==0x11 || d@(sp+4)==0x12 || d@(sp+4)==0x14)'
 local call='logerror "AUDIO_CALL selector=%X arg=%08X caller=%08X sp=%08X tick=%08X clock=%08X mix=%08X packet=%08X/%08X/%08X/%08X effectId=%04X callback=%08X context=%08X\\n",d@(sp+4),d@(sp+8),d@sp,sp,d@0x16a,d@'..clockAddress..',d@'..mixAddress..',if('..packet..',d@(d@(sp+8)),0),if('..packet..',d@(d@(sp+8)+4),0),if('..packet..',d@(d@(sp+8)+8),0),if('..packet..',d@(d@(sp+8)+0xc),0),if('..effect..',w@(d@(sp+8)+0x18),0),if('..effect..',d@(d@(sp+8)+0x1e),0),if('..effect..',d@(d@(sp+8)+0x22),0);g'
 callAction=call
 local ret='logerror "AUDIO_RETURN selector=%X d0=%08X d1=%08X sp=%08X tick=%08X\\n",d@(sp+4),d0,d1,sp,d@0x16a;g'
 for _,alias in ipairs({driver,driver|0x80000000})do
  cpu.debug:bpset(alias,'d@(sp+4)!=0x11',call)
  cpu.debug:bpset(alias,'d@(sp+4)==0x11','logerror "AUDIO_PAYLOAD_STOP\\n"')
  cpu.debug:bpset(alias+0xec,'1',ret)
  cpu.debug:bpset(alias+0x32f6,'1','logerror "AUDIO_COMPLETE slot=%X duration=%04X mix=%08X clock=%08X caller=%08X\\n",(a1-a4-0x24f2)/4,w@(a1+0x200),d@(a4+0x13a2),d@(a4+0x1a96),d@sp;g')
  cpu.debug:bpset(alias+0x3312,'1','logerror "AUDIO_CALLBACK slot=%X clock=%08X code=%08X context=%08X\\n",(a1-a4-0x24f2)/4,d@(a4+0x1a96),a0,d@sp;g')
  cpu.debug:bpset(alias+0x333a,'1','logerror "AUDIO_OFF note=%04X channel=%04X mix=%08X\\n",d1&0xffff,d5&0xffff,d@(a4+0x13a2);g')
  cpu.debug:bpset(alias+0x3372,'1','logerror "AUDIO_RELEASE slot=%X clock=%08X cursor=%08X end=%08X step=%08X\\n",(a0-a4-0x24f2)/4,d@(a4+0x1a96),d@a0,d@(a0+0x280),d@(a0+0x40);g')
  cpu.debug:bpset(alias+0x3382,'1','logerror "AUDIO_STATE mix=%08X status=%04X/%04X/%04X/%04X/%04X release=%04X/%04X/%04X/%04X/%04X\\n",d@(a4+0x13a2),w@(a4+0x26f2),w@(a4+0x26f6),w@(a4+0x26fa),w@(a4+0x26fe),w@(a4+0x2702),w@(a4+0x29f2),w@(a4+0x29f6),w@(a4+0x29fa),w@(a4+0x29fe),w@(a4+0x2a02);logerror "AUDIO_NOTE inst=%04X note=%04X duration=%04X volume=%04X channel=%04X owner=%04X tick=%08X\\n",d0&0xffff,d1&0xffff,d2&0xffff,d3&0xffff,d5&0xffff,d6&0xffff,d@0x16a;g')
  cpu.debug:bpset(alias+0x3570,'1','logerror "AUDIO_SELECT slot=%X old=%04X flags=%04X sample=%08X\\n",(a1-a4-0x24f2)/4,w@(a1+0x200),w@(a4+0x2ee4),d0;g')
  cpu.debug:bpset(alias+0x3760,'1','logerror "AUDIO_VOICE slot=%X note=%04X inst=%04X duration=%04X step=%08X pcm=%08X end=%08X loop=%08X/%08X\\n",(a1-a4-0x24f2)/4,w@(a1+0x480),w@(a1+0x4c0),w@(a4+0x2eda),d@(a1+0x40),d@(a1),d@(a1+0x280),d@(a1+0x2c0),d@(a1+0x300);g')
  cpu.debug:bpset(alias+0x37a6,'1','logerror "AUDIO_EFFECT slot=%X pcm=%08X size=%08X rate=%08X loop=%08X/%08X repeat=%08X id=%04X callback=%08X context=%08X\\n",(a1-a4-0x24f2)/4,d@a2,d@(a2+4),d@(a2+8),d@(a2+0xc),d@(a2+0x10),d@(a2+0x14),w@(a2+0x18),d@(a2+0x1e),d@(a2+0x22);g')
 end
 dbg.execution_state='run';return true
end
function M.finish()
 assert(driver,'AUDIO DRIVER NOT REACHED')
 print('AUDIO_OBSERVER_COMPLETE')
end
return M
