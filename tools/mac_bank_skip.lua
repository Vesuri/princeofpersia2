-- Execute the pinned original BankRLELR inner block on the reference Macintosh.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local base,arena,encoded,cases,finish,saved,bp,allocbp,output
local index,cc,count=1,0,0
local frame,stack=310272,310016
local function nextCase()
 local c=cases[index]
 for j=0,311295 do mem:writev_u8(arena+j,(j*37+(index-1)*13)&255)end
 mem:writev_u16(arena+frame-18,640)
 mem:writev_u32(arena+frame-16,arena+64+((index-1)&3))
 mem:writev_u32(arena+frame-8,encoded+c[1]);mem:writev_u32(arena+frame-12,encoded+c[1]+2)
 mem:writev_u16(arena+frame+16,c[2]);mem:writev_u16(arena+frame+18,c[3])
 mem:writev_u16(arena+frame+20,c[4]);mem:writev_u16(arena+frame+28,c[5])
 mem:writev_u32(arena+stack,base+178)
 for j=0,7 do cpu.state['D'..j].value=0x11220000+j*0x101 end
 for j=0,6 do cpu.state['A'..j].value=0x11220000+(j+8)*0x101 end
 cpu.state.D0.value=0;cpu.state.A6.value=arena+frame
 cpu.state.SP.value=arena+stack;cpu.state.SR.value=(cpu.state.SR.value&0xffe0)|0x0700|cc
 cpu.state.PC.value=base;dbg.execution_state='run'
end
function M.start(address,done)
 base=address;finish=done;saved={}
 for _,name in ipairs({'D0','D1','D2','D3','D4','D5','D6','D7','A0','A1','A2','A3','A4','A5','A6','SP','SR','PC'})do saved[name]=cpu.state[name].value end
 local directory=assert(os.getenv('POP2_BANK_SKIP_FIXTURE'))
 cases=dofile(directory..'/cases.lua');assert(#cases==1034)
 local f=assert(io.open(directory..'/original.bin','rb'));local image=f:read('*a');f:close();assert(#image==178)
 for i=1,#image do mem:writev_u8(base+i-1,image:byte(i))end
 mem:writev_u16(base+178,0x4e71)
 mem:writev_u16(base+256,0xa11e);mem:writev_u16(base+258,0x4e71)
 allocbp=cpu.debug:bpset(base+258,'1','logerror "BANK_ALLOC\\n"')
 bp=cpu.debug:bpset(base+178,'1','logerror "BANK_RETURN\\n"')
 output=assert(io.open(assert(os.getenv('POP2_REFERENCE_RUN'))..'/bank.effects','wb'))
 cpu.state.D0.value=0x80000;cpu.state.PC.value=base+256;dbg.execution_state='run'
end
function M.poll()
 if not base or dbg.execution_state~='stop' then return false end
 if cpu.state.PC.value==base+258 then
  assert(cpu.state.D0.value==0,'BANK / NEWPTR');arena=cpu.state.A0.value
  assert(arena>0 and arena+0x80000<0x800000,'BANK / ARENA');encoded=arena+311296
  local f=assert(io.open(os.getenv('POP2_BANK_SKIP_FIXTURE')..'/encoded.bin','rb'));local bytes=f:read('*a');f:close()
  assert(#bytes<0x80000-311296)
  for j=1,#bytes do mem:writev_u8(encoded+j-1,bytes:byte(j))end
  cpu.debug:bpclear(allocbp);nextCase();return true
 end
 if cpu.state.PC.value~=base+178 then return false end
 local row={}
 for j=0,7 do row[#row+1]=cpu.state['D'..j].value&0xffffffff end
 for j=0,6 do local v=cpu.state['A'..j].value;if j==6 then v=v-arena end;row[#row+1]=v&0xffffffff end
 row[#row+1]=(cpu.state.SR.value&31)<<16;row[#row+1]=(cpu.state.SP.value-arena)&0xffffffff
 output:write(string.pack('>'..string.rep('I4',17),table.unpack(row)))
 mem:writev_u32(arena+frame-16,mem:readv_u32(arena+frame-16)-arena)
 for _,offset in ipairs({-12,-8})do mem:writev_u32(arena+frame+offset,mem:readv_u32(arena+frame+offset)-encoded)end
 local bytes={};for j=frame-24,frame+31 do bytes[#bytes+1]=string.char(mem:readv_u8(arena+j))end;output:write(table.concat(bytes))
 local delta={}
 for j=0,307199 do local v=mem:readv_u8(arena+j);if v~=((j*37+(index-1)*13)&255)then delta[#delta+1]=string.pack('>I4B',j,v)end end
 output:write(string.pack('>I4',#delta));output:write(table.concat(delta))
 count=count+1;cc=cc+1
 if cc>=(index<=14 and 32 or 1)then cc=0;index=index+1 end
 if count%64==0 then print('BANK_PROGRESS cases='..count)end
 if index>#cases then
  assert(count==1468);output:close();print('BANK_COMPLETE cases=1468');finish();cpu.debug:bpclear(bp)
  for name,value in pairs(saved)do cpu.state[name].value=value end
  print('BANK_RESTORED');base=nil;manager.machine:exit();dbg.execution_state='run'
 else nextCase()end
 return true
end
return M
