-- Execute the byte-pinned original loop on the Macintosh, with private memory.
local M={}
local cpu=manager.machine.devices[':maincpu'];local mem=cpu.spaces.program
local dbg=manager.machine.debugger
local base,arena,cases,index,finish,saved,bp,allocbp,output,n,frame,stack
local function nextCase()
 local c=cases[index];local width,src,dst,cc=table.unpack(c)
 n=width+math.max(src,dst)+16;frame=(n+255)&~3;stack=frame+128
 for j=0,stack+127 do mem:writev_u8(arena+j,(j*37+(index-1)*13)&255)end
 mem:writev_u32(arena+frame-32,arena+src);mem:writev_u32(arena+frame-36,arena+dst)
 mem:writev_u32(arena+frame-48,0);mem:writev_u32(arena+frame-8,width)
 mem:writev_u32(arena+stack,base+36)
 for j=0,7 do cpu.state['D'..j].value=0x11220000+j*0x101 end
 for j=0,6 do cpu.state['A'..j].value=0x11220000+(j+8)*0x101 end
 cpu.state.D0.value=0;cpu.state.D3.value=width;cpu.state.A6.value=arena+frame
 cpu.state.SP.value=arena+stack;cpu.state.SR.value=(cpu.state.SR.value&0xffe0)|0x0700|cc
 cpu.state.PC.value=base;dbg.execution_state='run'
end
function M.start(address,done)
 base=address;finish=done;index=1;saved={}
 for _,name in ipairs({'D0','D1','D2','D3','D4','D5','D6','D7','A0','A1','A2','A3','A4','A5','A6','SP','SR','PC'})do saved[name]=cpu.state[name].value end
 local directory=assert(os.getenv('POP2_SAVE_RECT_FIXTURE'))
 cases=dofile(directory..'/cases.lua')
 local f=assert(io.open(directory..'/original.bin','rb'));local image=f:read('*a');f:close();assert(#image==36)
 for i=1,#image do mem:writev_u8(base+i-1,image:byte(i))end
 mem:writev_u16(base+36,0x4e71)
 -- NewPtr for a full maximum-width pair of buffers, separate from fixture stack.
 mem:writev_u16(base+64,0xa11e);mem:writev_u16(base+66,0x4e71)
 allocbp=cpu.debug:bpset(base+66,'1','logerror "SAVE_RECT_ALLOC\\n"')
 bp=cpu.debug:bpset(base+36,'1','logerror "SAVE_RECT_RETURN\\n"')
 output=assert(io.open(assert(os.getenv('POP2_REFERENCE_RUN'))..'/save-rect.bin','wb'))
 cpu.state.D0.value=0x12000;cpu.state.PC.value=base+64;dbg.execution_state='run'
end
function M.poll()
 if not base or dbg.execution_state~='stop' then return false end
 if cpu.state.PC.value==base+66 then
  assert(cpu.state.D0.value==0,'SAVE RECT / NEWPTR');arena=cpu.state.A0.value
  assert(arena>0 and arena+0x12000<0x800000,'SAVE RECT / ARENA')
  cpu.debug:bpclear(allocbp);nextCase();return true
 end
 if cpu.state.PC.value~=base+36 then return false end
 local row={}
 for j=0,7 do row[#row+1]=cpu.state['D'..j].value&0xffffffff end
 for j=0,6 do local v=cpu.state['A'..j].value;if j==0 or j==6 then v=v-arena end;row[#row+1]=v&0xffffffff end
 row[#row+1]=(cpu.state.SR.value&31)<<16;row[#row+1]=(cpu.state.SP.value-arena)&0xffffffff
 output:write(string.pack('>'..string.rep('I4',17),table.unpack(row)))
 mem:writev_u32(arena+frame-32,mem:readv_u32(arena+frame-32)-arena)
 mem:writev_u32(arena+frame-36,mem:readv_u32(arena+frame-36)-arena)
 local bytes={};for j=0,n-1 do bytes[#bytes+1]=string.char(mem:readv_u8(arena+j))end;output:write(table.concat(bytes))
 bytes={};for j=frame-64,frame-1 do bytes[#bytes+1]=string.char(mem:readv_u8(arena+j))end;output:write(table.concat(bytes))
 index=index+1
 if index>#cases then
  output:close();print('SAVE_RECT_COMPLETE cases='..#cases);finish();cpu.debug:bpclear(bp)
  for name,value in pairs(saved)do cpu.state[name].value=value end
  print('SAVE_RECT_RESTORED');base=nil;manager.machine:exit();dbg.execution_state='run'
 else nextCase()end
 return true
end
return M
