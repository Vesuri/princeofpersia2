-- Read current indexed Mac video memory using the card's hardware palette.
-- Unlike screen:pixels(), this is not an earlier field's cached rendering.
local frame={}
function frame.rgb(path)
 local mem=manager.machine.devices[':maincpu'].spaces.program
 local function ptr(a)return mem:readv_u32(a)&0xffffff end
 local gd=ptr(ptr(0x8a4));local pm=ptr(ptr(gd+22));local pixels=mem:readv_u32(pm)
 local stride=mem:readv_u16(pm+4)&0x3fff
 assert(mem:readv_u16(pm+32)==8 and stride>=640)
 assert(mem:readv_i16(pm+10)-mem:readv_i16(pm+6)==480 and mem:readv_i16(pm+12)-mem:readv_i16(pm+8)==640)
 print(string.format('FRAMEBUFFER gd=%08X pm=%08X pixels=%08X stride=%d',gd,pm,pixels,stride))
 local palette=assert(manager.machine.palettes[':nbc:mdc48']);local colors={}
 for i=0,255 do local c=palette:pen_color(i);colors[i]=string.char((c>>16)&255,(c>>8)&255,c&255)end
 local f=assert(io.open(path,'wb'));f:write('P6\n640 480\n255\n')
 for y=0,479 do local row={};for x=0,639 do row[#row+1]=colors[mem:read_u8(pixels+y*stride+x)]end;f:write(table.concat(row))end
 f:close()
end
function frame.indexed(path)
 local mem=manager.machine.devices[':maincpu'].spaces.program
 local function ptr(a)return mem:readv_u32(a)&0xffffff end
 local pm=ptr(ptr(ptr(ptr(0x8a4))+22));local base=mem:readv_u32(pm)
 local stride=mem:readv_u16(pm+4)&0x3fff
 assert(mem:readv_u16(pm+32)==8 and stride>=640,'INDEXED DEPTH')
 local f=assert(io.open(path,'wb'))
 for y=0,479 do local row={};for x=0,639 do row[#row+1]=string.char(mem:read_u8(base+y*stride+x))end;f:write(table.concat(row))end
 f:close()
end
function frame.palette(path)
 local palette=assert(manager.machine.palettes[':nbc:mdc48']);local colors={}
 for i=0,255 do local c=palette:pen_color(i);colors[#colors+1]=string.char((c>>16)&255,(c>>8)&255,c&255)end
 local f=assert(io.open(path,'wb'));f:write(table.concat(colors));f:close()
end
function frame.logical(path)
 local mem=manager.machine.devices[':maincpu'].spaces.program
 local function ptr(a)return mem:readv_u32(a)&0xffffff end
 local pm=ptr(ptr(ptr(ptr(0x8a4))+22));local clut=ptr(ptr(pm+42))
 assert(mem:readv_u16(clut+6)==255,'LOGICAL CLUT SIZE')
 local f=assert(io.open(path,'wb'));local bytes={}
 for i=0,2055 do bytes[#bytes+1]=string.char(mem:readv_u8(clut+i))end
 f:write(table.concat(bytes));f:close()
end
return frame
