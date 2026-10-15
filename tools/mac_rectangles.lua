-- Authored pure-rectangle ABI cases, executed by System 7.5.5.
local M={}
local cpu,mem,dbg,base,index,cases,finish
local rects={{0,0,10,20},{5,10,15,30},{10,20,20,40},{0,0,0,0},{10,20,0,0},{-32768,-32768,32767,32767},{-32768,-1,0,1},{0,-32768,1,32767},{-20,-30,-10,-5},{32767,32767,-32768,-32768}}
local function nextCase()
 local c=cases[index]
 for n,r in ipairs({rects[c.a],rects[c.b],{123,456,789,1234}})do for i,v in ipairs(r)do mem:writev_u16(base+256+(n-1)*8+(i-1)*2,v&65535)end end
 local sp=base+1024;local dst=base+256+c.alias*8
 mem:writev_u16(base,c.trap);mem:writev_u16(base+2,0x4e71)
 if c.trap==0xa8ae then mem:writev_u32(sp,base+256);mem:writev_u16(sp+4,0x7f5a)
 else mem:writev_u32(sp,dst);mem:writev_u32(sp+4,base+264);mem:writev_u32(sp+8,base+256);mem:writev_u16(sp+12,0x7f5a)end
 for i=0,7 do cpu.state['D'..i].value=0x11220000+i*0x101 end
 for i=0,4 do cpu.state['A'..i].value=0x22330000+i*0x101 end
 cpu.state.SR.value=(cpu.state.SR.value&0xffe0)|c.cc
 cpu.state.SP.value=sp;cpu.state.PC.value=base;dbg.execution_state='run'
end
function M.start(address,done)
 finish=done
 cpu=manager.machine.devices[':maincpu'];mem=cpu.spaces['program'];dbg=manager.machine.debugger;base=address;index=1;cases={}
 for _,trap in ipairs({0xa8aa,0xa8ab,0xa8ae})do for a=1,#rects do for b=1,(trap==0xa8ae and 1 or #rects)do for alias=0,(trap==0xa8ae and 0 or 2)do for _,cc in ipairs({0,31})do cases[#cases+1]={trap=trap,a=a,b=b,alias=alias,cc=cc}end end end end end
 cpu.debug:bpset(base+2,'1','logerror "RECTANGLE_STOP\\n"');nextCase()
end
function M.poll()
 if not base or dbg.execution_state~='stop' or cpu.state.PC.value~=base+2 then return false end
 local c=cases[index];local consumed=c.trap==0xa8ae and 4 or 12
 assert((cpu.state.SP.value&0xffffff)==base+1024+consumed,'RECTANGLE STACK')
 local words={};for i=0,11 do words[#words+1]=string.format('%04X',mem:readv_u16(base+256+2*i))end
 local regs={};for i=0,7 do regs[#regs+1]=string.format('%08X',cpu.state['D'..i].value)end
 -- Addresses are separately checked by native fixture; volatile A0/A1 are ROM implementation details.
 print(string.format('RECTANGLE_RESULT index=%d trap=%04X a=%d b=%d alias=%d cc=%d sr=%02X result=%04X rects=%s data=%s',index,c.trap,c.a,c.b,c.alias,c.cc,cpu.state.SR.value&31,mem:readv_u16(base+1024+consumed),table.concat(words,','),table.concat(regs,',')))
 index=index+1
 if index>#cases then print('RECTANGLE_COMPLETE calls='..#cases);finish();manager.machine:exit();dbg.execution_state='run'else nextCase()end
 return true
end
return M
