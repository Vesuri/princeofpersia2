-- Original-routine fixtures. Explicit caller state is separate from normal play.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local base,packing,stack,returnPC,probe,phase,world
local menuReturn,menuOutput
local function invoke(address,args)
 args=args or '';local sp=stack-#args-4
 mem:writev_u32(sp,returnPC)
 for i=1,#args do mem:writev_u8(sp+3+i,args:byte(i))end
 cpu.state.SP.value=sp;cpu.state.PC.value=address
 probe=cpu.debug:bpset(returnPC,string.format('sp==0x%x',sp+4),'logerror "UI_ROUTINE_RETURN\\n"')
 dbg.execution_state='run'
end
local function menuFixture()
 local code=stack+1024;menuOutput=stack+2048;local words={}
 local function word(v)words[#words+1]=string.pack('>I2',v&65535)end
 local function long(v)word(v>>16);word(v)end
 local function item(offset,number,trap)word(0x2f2d);word(offset);word(0x3f3c);word(number);word(trap)end
 local function key(c)word(0x598f);word(0x3f3c);word(string.byte(c));word(0xa93e);word(0x24df)end
 word(0x45f9);long(menuOutput)
 item(-0x5310,5,0xa93a);key('n');key('s');item(-0x5310,5,0xa939);key('s')
 item(-0x5310,5,0xa93a);key('s');key('q');key('t');key('m');key('h')
 item(-0x5318,4,0xa939);key('f');item(-0x5318,7,0xa939)
 word(0x206d);word(-0x5318);word(0x2050);word(0x24e8);word(10)
 key('e');key('o');key('r');key('p');key('i')
 local bytes=table.concat(words);menuReturn=code+#bytes;bytes=bytes..string.pack('>I2',0x4e71)
 for i=1,#bytes do mem:writev_u8(code+i-1,bytes:byte(i))end
 for i=1,#bytes do assert(mem:readv_u8(code+i-1)==bytes:byte(i),'UI MENU FIXTURE BYTES')end
 cpu.state.SP.value=stack;cpu.state.PC.value=code
 probe=cpu.debug:bpset(menuReturn,'1','logerror "UI_MENU_RETURN\\n"');phase='menu';dbg.execution_state='run'
end
function M.phase()return phase end
function M.start(meta,checkpoint)
 world=cpu.state.A5.value&0xffffff;returnPC=checkpoint
 local function segment(id)
  local info=assert(meta.segments[id]);local entry=world+info.jt
  assert(mem:readv_u16(entry)==id and mem:readv_u16(entry+2)==0x4ef9,'UI ROUTINE SEGMENT')
  return (mem:readv_u32(entry+4)&0xffffff)-info.offset
 end
 base=segment(2);packing=segment(17)
 for _,site in ipairs({{2,base,0x1a36,0x196},{17,packing,0x4560,0x2c}})do
  local hex=meta.segments[site[1]].hex
  for i=site[3],site[3]+site[4]-1 do assert(mem:readv_u8(site[2]+i)==tonumber(hex:sub(i*2+1,i*2+2),16),'UI ROUTINE BYTES')end
 end
 stack=(cpu.state.SP.value&0xffffff)-4096;assert(stack>0x100000,'UI ROUTINE STACK')
 -- This is the original's already-accepted condition, supplied as fixture state.
 -- The harness never patches the protection instructions or invents their return.
 mem:writev_u16(world-0x56d8,1);mem:writev_u16(world-0x4ffe,0)
 phase='copy-accepted';invoke(base+0x1a36)
end
function M.poll()
 if not phase or dbg.execution_state~='stop'then return false end
 if phase=='menu' and cpu.state.PC.value==menuReturn then
  assert(cpu.state.SP.value==stack,'UI MENU STACK')
  cpu.debug:bpclear(probe)
  for i=0,14 do print(string.format('UI_MENU_RESULT index=%d value=%08X',i,mem:readv_u32(menuOutput+4*i)))end
  print('UI_MENU_COMPLETE calls=19');phase='packing';invoke(packing+0x4560);return true
 end
 if cpu.state.PC.value~=returnPC then return false end
 cpu.debug:bpclear(probe)
 if phase=='copy-accepted' then
  assert(mem:readv_u16(world-0x56d8)==1,'UI COPY ACCEPTANCE STATE')
  local h=mem:readv_u32(world-0x5318)&0xffffff;local p=mem:readv_u32(h)&0xffffff
  print(string.format('UI_ROUTINE_COMPLETE copy-accepted flags=%08X',mem:readv_u32(p+10)))
  menuFixture()
 elseif phase=='packing' or phase=='packing-game' then
  local was=phase
  assert(cpu.state.D0.value&255==0,'UI PACKING NOT DECLINED')
  print('UI_ROUTINE_COMPLETE '..was..'-declined result=0')
  if was=='packing' then phase='packing-game';invoke(packing+0x463a)
  else phase='about';invoke(packing+0x245e,string.pack('>I2',600))end
 elseif phase=='about' or phase:sub(1,6)=='alert-' then
  if phase=='about' then print('UI_ROUTINE_COMPLETE about')
  else
   local expected=phase:sub(-2)=='no' and 2 or 1
   assert(cpu.state.D0.value&65535==expected,'UI ALERT RESULT')
   print('UI_ROUTINE_COMPLETE '..phase..' result='..expected)
  end
  local order={'about','alert-401-yes','alert-404-yes','alert-405-yes','alert-405-no','alert-406-yes','alert-406-no'}
  local index;for i,label in ipairs(order)do if label==phase then index=i end end
  assert(index,'UI ALERT PHASE')
  if index==#order then phase='complete';manager.machine:exit();dbg.execution_state='run'
  else
   phase=order[index+1];local kind=tonumber(phase:sub(7,9));local message='Port UI probe'..string.char(0)
   for i=1,#message do mem:writev_u8(stack+255+i,message:byte(i))end
   invoke(packing+0x6d42,string.pack('>I2I4',kind,stack+256))
  end
 else error('UI ROUTINE UNKNOWN PHASE')end
 return true
end
return M
