#ifdef POP2_CORE
#include "NativeDisplay.h"
#include "PrinceScreen.h"
#include "mac/GWorld8.h"
#include "mac/Palette8.h"
#include "mac/CopyBits8.h"
#include "mac/AnimatePalette8.h"
namespace NativeDisplay {
namespace {
using H=MacHeap::Handle;
MacHeap* heap;Valid valid;Resource resource;uint8_t* low;uint8_t* pixels;uint8_t* thePort;
H device,map,colors;uint32_t currentDevice;
const char* reason="DISPLAY / NOT INITIALIZED";
struct World {uint8_t* port;H pm,pixels,vis,clip,vars,patterns[3];bool locked,isScreen;H palette;bool active;};
World worlds[16]={};
PrinceScreen* output;const uint8_t* transfer;
Planar8::Rect dirty[64];uint16_t dirtyCount=0;bool paletteDirty=false;
struct Palette {H body,privateData;};Palette palettes[8]={};
uint16_t w(const uint8_t* p) {return uint16_t(p[0])<<8|p[1];}
uint32_t l(const uint8_t* p) {return uint32_t(w(p))<<16|w(p+2);}
void sw(uint8_t* p,uint16_t v) {GWorld8::word(p,v);}
void sl(uint8_t* p,uint32_t v) {GWorld8::longword(p,v);}
bool fail(const char* s) {reason=s;return false;}
uint8_t* pointer(uint32_t address,uint32_t bytes) {return valid(address,bytes)?(uint8_t*)address:nullptr;}
World* byPort(uint32_t address) {for(auto& v:worlds)if(v.port && (uint32_t)v.port==address)return &v;return nullptr;}
World* byMap(uint32_t address) {for(auto& v:worlds)if(v.port && (uint32_t)v.pm==address)return &v;return nullptr;}
void refresh(World& v) {
    if(v.isScreen)return;
    sl(*v.pm,v.locked?(uint32_t)*v.pixels:(uint32_t)v.pixels);sw(*v.pm+14,v.locked?1:2);
}
void dispose(World& v) {
    H hs[]={v.pm,v.pixels,v.vis,v.clip,v.vars,v.patterns[0],v.patterns[1],v.patterns[2]};
    for(H h:hs)if(h)heap->disposeHandle(h);
    if(v.port)heap->disposePtr(v.port);v={};
}
World* create(const uint8_t* bounds,bool screen=false) {
    GWorld8::Rect r={int16_t(w(bounds)),int16_t(w(bounds+2)),int16_t(w(bounds+4)),int16_t(w(bounds+6))};
    int32_t width=int32_t(r.right)-r.left,height=int32_t(r.bottom)-r.top;
    // MEASURED noNewDevice requests: 512x384 and 510x87, sharing the main CLUT.
    // Other allocation forms stay loud until the original contract is captured.
    if(r.top || r.left || !((width==512 && height==384)||(width==510 && height==87)))return nullptr;
    World* v=nullptr;for(auto& candidate:worlds)if(!candidate.port) {v=&candidate;break;}
    if(!v)return nullptr;
    v->port=heap->newPtr(screen?156:108,true);
    if(!v->port)return nullptr;
    v->pm=heap->newHandle(50,true);if(!screen)v->pixels=heap->newHandle(528*height,false);
    v->vis=heap->newHandle(10,true);v->clip=heap->newHandle(10,true);v->vars=heap->newHandle(46,true);
    for(unsigned i=0;i<3;++i)v->patterns[i]=heap->newHandle(28,true);
    if(!v->pm || (!screen && !v->pixels) || !v->vis || !v->clip || !v->vars || !v->patterns[0] || !v->patterns[1] || !v->patterns[2]) {dispose(*v);return nullptr;}
    GWorld8::Layout layout={528,uint32_t(528*height)};
    GWorld8::pixmap(*v->pm,(uint32_t)v->pixels,(uint32_t)colors,r,layout);
    sw(*v->vis,10);GWorld8::rect(*v->vis+2,r);
    sw(*v->clip,10);GWorld8::rect(*v->clip+2,{-32767,-32767,32767,32767});
    GWorld8::port(v->port,(uint32_t)v->pm,(uint32_t)v->vars,(uint32_t)v->vis,(uint32_t)v->clip,
                 (uint32_t)v->patterns[0],(uint32_t)v->patterns[1],(uint32_t)v->patterns[2],r);
    v->isScreen=screen;
    if(screen) {
        GWorld8::Layout physical={640,640*480};
        GWorld8::pixmap(*v->pm,(uint32_t)pixels,(uint32_t)colors,{0,0,480,640},physical);
        sw(*v->pm+14,0);sw(v->port+6,0xc000);sw(v->port+68,1);
        for(unsigned j=0;j<6;++j)v->port[42+j]=0;
        sl(v->port+84,255);sw(v->port+108,8);
    }
    return v;
}
Palette* palette(H h) {for(auto& p:palettes)if(p.body==h && h)return &p;return nullptr;}
bool position(World& v,int16_t left,int16_t top) {
    if(!v.isScreen || left<0 || top<0 || left>128 || top>96)return false;
    GWorld8::rect(*v.pm+6,{int16_t(-top),int16_t(-left),int16_t(480-top),int16_t(640-left)});
    return true;
}
void changed(World& v,const uint8_t* bounds) {
    int16_t top=-int16_t(w(*v.pm+6)),left=-int16_t(w(*v.pm+8));
    if(dirtyCount==64) {dirtyCount=1;dirty[0]={top,left,int16_t(top+384),int16_t(left+512)};return;}
    dirty[dirtyCount++]={int16_t(top+int16_t(w(bounds))),int16_t(left+int16_t(w(bounds+2))),
                        int16_t(top+int16_t(w(bounds+4))),int16_t(left+int16_t(w(bounds+6)))};
}
bool activate(World& v) {
    if(!v.palette)return true; // An unbound window has the system device colours.
    Palette* p=palette(v.palette);if(!p)return false;
    if(v.active)return true;
    if(!AnimatePalette8::activate(*p->body,4112,*colors,2056))return false;
    sl(*colors,l(*colors)+1);sl(*p->privateData,l(*colors));v.active=true;paletteDirty=true;return true;
}
}
const char* error() {return reason;}
const uint8_t* screen() {return pixels;}
void reset() {heap=nullptr;valid=nullptr;low=pixels=thePort=nullptr;device=map=colors=nullptr;for(auto& v:worlds)v={};for(auto& p:palettes)p={};resource=nullptr;output=nullptr;transfer=nullptr;dirtyCount=0;paletteDirty=false;}
bool prepare(MacHeap& owner,uint8_t* shadow,Valid validator,Resource reader) {
    reset();heap=&owner;low=shadow;valid=validator;resource=reader;
    pixels=heap->newPtr(640*480,true);device=heap->newHandle(62,true);
    map=heap->newHandle(50,true);colors=heap->newHandle(2056,true);
    if(!pixels || !device || !map || !colors)return fail("DISPLAY / DEVICE MEMORY");
    Palette8::systemTable(*colors,1);
    GWorld8::Rect bounds={0,0,480,640};GWorld8::Layout layout={640,640*480};
    GWorld8::pixmap(*map,(uint32_t)pixels,(uint32_t)colors,bounds,layout);sw(*map+14,0);
    sw(*device,uint16_t(-50));sw(*device+10,4);sw(*device+20,0xbd21);
    sl(*device+22,(uint32_t)map);GWorld8::rect(*device+34,bounds);sl(*device+42,0x83);
    currentDevice=(uint32_t)device;sl(low+0x8a4,currentDevice);sl(low+0x8a8,currentDevice);
    reason="DISPLAY / READY";return true;
}
bool initWindowManager() {
    if(!thePort || l(thePort))return fail("UI / WINDOW MANAGER INITIALIZATION");
    uint8_t bounds[8];GWorld8::rect(bounds,{0,0,384,512});
    World* v=create(bounds,true);if(!v)return fail("UI / WINDOW MANAGER MEMORY");
    GWorld8::rect(v->port+16,{0,0,480,640});
    sl(thePort,(uint32_t)v->port);return true;
}
bool showOverlay(const uint8_t* image) {
    if(!output || !image)return fail("UI / PRESENTATION UNAVAILABLE");
    uint32_t rgb[256]={};rgb[0]=0xffffff;
    Planar8::Rect viewport={0,0,384,512};
    if(!output->publish(image,viewport,&viewport,1,rgb))return fail("UI / PRESENTATION");
    output->wait();return true;
}
bool hideOverlay() {
    World* window=nullptr;for(auto& v:worlds)if(v.port && v.isScreen && v.port[110])window=&v;
    if(window){uint8_t bounds[8];GWorld8::rect(bounds,{0,0,384,512});changed(*window,bounds);paletteDirty=true;return publish();}
    uint32_t rgb[256]={};Planar8::Rect viewport={0,0,384,512};
    if(!output || !output->publish(pixels,viewport,&viewport,1,rgb))return fail("UI / RESTORE PRESENTATION");
    output->wait();return true;
}
void attach(PrinceScreen* screen,const uint8_t* lookup) {output=screen;transfer=lookup;}
void wait() {if(output)output->wait();}
bool publish() {
    if(!output || (!dirtyCount && !paletteDirty))return true;
    if(!transfer)return fail("DISPLAY / VIDEO TRANSFER MISSING");
    World* window=nullptr;for(auto& v:worlds)if(v.port && v.isScreen && v.port[110]) {
        if(window)return fail("DISPLAY / MULTIPLE VISIBLE WINDOWS");window=&v;
    }
    if(!window)return true;
    int16_t top=-int16_t(w(*window->pm+6)),left=-int16_t(w(*window->pm+8));
    Planar8::Rect viewport={top,left,int16_t(top+384),int16_t(left+512)};
    uint32_t rgb[256];for(unsigned i=0;i<256;++i) {
        const uint8_t* color=*colors+8+i*8;
        rgb[i]=uint32_t(transfer[color[2]])<<16|uint32_t(transfer[color[4]])<<8|transfer[color[6]];
    }
    if(!output->publish(pixels,viewport,dirty,dirtyCount,rgb))return fail("DISPLAY / PUBLICATION");
    dirtyCount=0;paletteDirty=false;return true;
}
bool handles(uint16_t trap) {
    return trap==0xa91f || trap==0xa924 || trap==0xaa46 || trap==0xaa91 || trap==0xaa9c || trap==0xaa9e || trap==0xaa95 || trap==0xaa94 || trap==0xaa9a || trap==0xaa14 || trap==0xaa15 || trap==0xa915 || trap==0xa916 || trap==0xa91b || trap==0xa877 || trap==0xa878 || trap==0xa8ec || trap==0xa86e || trap==0xaa2a || trap==0xab1d || trap==0xa873 || trap==0xa874;
}
bool dispatch(uint16_t trap,uint32_t* regs,uint8_t* args,uint32_t& consumed) {
    if(!heap || !valid || !pointer((uint32_t)args,trap==0xab1d && uint16_t(regs[0])==0 ? 24 : (trap==0xab1d ? 8 : 4)))return fail("DISPLAY / ARGUMENT EXTENT");
    if(trap==0xa91f){
        if(!pointer((uint32_t)args,4))return fail("UI / SELECT WINDOW ARGUMENTS");
        World* v=byPort(l(args));if(!v||!v->isScreen)return fail("UI / SELECT WINDOW OWNER");
        // D8: one owned game client, with no Macintosh ordering/chrome to draw.
        consumed=4;return true;
    }
    if(trap==0xa924){
        if(!pointer((uint32_t)args,4))return fail("UI / FRONT WINDOW OUTPUT");
        World* front=nullptr;for(auto& v:worlds)if(v.port&&v.isScreen&&v.port[110]){
            if(front)return fail("UI / MULTIPLE FRONT WINDOWS");front=&v;
        }
        sl(args,front?(uint32_t)front->port:0);return true;
    }
    if(trap==0xaa46) {
        if(!pointer((uint32_t)args,14) || l(args)!=0xffffffffUL || l(args+4) || !resource)return fail("DISPLAY / NEW WINDOW FORM");
        const uint8_t* bytes=nullptr;uint32_t size=0;
        if(!resource(0x57494e44,int16_t(w(args+8)),bytes,size) || size<19 || w(bytes+8)!=4 || bytes[10] || bytes[12])return fail("DISPLAY / WIND RESOURCE");
        int16_t top=w(bytes),left=w(bytes+2),height=int16_t(w(bytes+4))-top,width=int16_t(w(bytes+6))-left;
        if(height!=384 || width!=512)return fail("DISPLAY / WINDOW SIZE");
        uint8_t bounds[8];GWorld8::rect(bounds,{0,0,height,width});
        World* v=create(bounds,true);if(!v || !position(*v,left,top))return fail("DISPLAY / WINDOW ALLOCATION");
        sl(v->port+152,l(bytes+14));sl(args+10,(uint32_t)v->port);regs[0]=0;consumed=10;return true;
    }
    if(trap==0xaa91) {
        if(!pointer((uint32_t)args,14) || w(args) || w(args+2)!=12 || l(args+4) || w(args+8)!=256)return fail("DISPLAY / NEW PALETTE FORM");
        unsigned i=0;while(i<8 && palettes[i].body)++i;if(i==8)return fail("DISPLAY / PALETTE CAPACITY");
        Palette& p=palettes[i];p.body=heap->newHandle(4112,true);p.privateData=heap->newHandle(4,true);
        if(!p.body || !p.privateData)return fail("DISPLAY / PALETTE ALLOCATION");
        sw(*p.body,256);sw(*p.body+6,i+2);sl(*p.body+12,(uint32_t)p.privateData);
        for(unsigned j=0;j<256;++j)sw(*p.body+16+j*16+6,12);
        sl(args+10,(uint32_t)p.body);regs[0]=0;consumed=10;return true;
    }
    if(trap==0xaa9c || trap==0xaa9e) {
        if(!pointer((uint32_t)args,10))return fail("DISPLAY / PALETTE ENTRY ARGUMENTS");
        Palette* p=palette((H)l(args+6));uint16_t index=w(args+4);
        if(!p || index>255)return fail("DISPLAY / PALETTE ENTRY OWNER");
        uint8_t* entry=*p->body+16+index*16;
        if(w(entry+10))return fail("DISPLAY / ACTIVE ENTRY EDIT");
        if(trap==0xaa9c) {
            uint8_t* rgb=pointer(l(args),6);if(!rgb)return fail("DISPLAY / ENTRY RGB");CopyBits8::copySpan(rgb,entry,6);
        } else {
            if(w(args) || w(args+2)!=2 || (index!=0 && index!=255))return fail("DISPLAY / ENTRY USAGE");
            sw(entry+6,2);
        }
        consumed=10;regs[0]=0;return true;
    }
    if(trap==0xaa95) {
        if(!pointer((uint32_t)args,10) || args[0]!=1)return fail("DISPLAY / SETPALETTE FORM");
        World* v=byPort(l(args+6));Palette* p=palette((H)l(args+2));
        if(!v || !v->isScreen || !p || v->palette)return fail("DISPLAY / PALETTE BINDING");
        v->palette=p->body;sw(*p->body+6,w(*p->body+6)|0xc000);sl(*p->body+8,1);
        if(v->port[110] && !activate(*v))return fail("DISPLAY / PALETTE ACTIVATION");
        consumed=10;return true;
    }
    if(trap==0xaa9a) {
        if(!pointer((uint32_t)args,14))return fail("DISPLAY / ANIMATE ARGUMENTS");
        World* v=byPort(l(args+10));H source=(H)l(args+6);
        if(!v || !v->palette || !heap->isHandle(source) || !*source)return fail("DISPLAY / ANIMATE OWNERSHIP");
        if(!AnimatePalette8::animate(*v->palette,heap->handleSize(v->palette),*colors,2056,*source,heap->handleSize(source),w(args+4),w(args+2),w(args),v->active))return fail("DISPLAY / ANIMATE FORM");
        if(v->active)paletteDirty=true;consumed=14;return true;
    }
    if(trap==0xa915 || trap==0xa916 || trap==0xaa94 || trap==0xa91b) {
        if(trap==0xa91b && !pointer((uint32_t)args,10))return fail("DISPLAY / MOVE WINDOW ARGUMENTS");
        World* v=byPort(l(args+(trap==0xa91b?6:0)));if(!v || !v->isScreen)return fail("DISPLAY / WINDOW OWNER");
        if(trap==0xa91b) {
            if(args[0] || !position(*v,int16_t(w(args+4)),int16_t(w(args+2))))return fail("DISPLAY / MOVE WINDOW FORM");
            consumed=10;return true;
        }
        if(trap==0xa915 && !v->port[110]) {
            v->port[110]=1;v->port[111]=255;
            int16_t top=-int16_t(w(*v->pm+6)),left=-int16_t(w(*v->pm+8));
            for(unsigned y=0;y<384;++y)for(unsigned x=0;x<512;++x)pixels[uint32_t(y+top)*640+x+left]=uint8_t(l(v->port+84));
            changed(*v,v->port+16);
        }
        if(trap==0xa916) {v->port[110]=v->port[111]=0;}
        if(trap!=0xa916 && v->port[110] && !activate(*v))return fail("DISPLAY / ACTIVATE PALETTE");
        consumed=4;return true;
    }
    if(trap==0xa877 || trap==0xa878 || trap==0xaa14 || trap==0xaa15) {
        World* v=thePort?byPort(l(thePort)):nullptr;if(!v)return fail("DISPLAY / CURRENT PORT");
        if(trap==0xa877) {if(!position(*v,int16_t(w(args+2)),int16_t(w(args))))return fail("DISPLAY / MOVEPORTTO FORM");}
        else if(trap==0xa878) {if(w(args) || w(args+2) || w(v->port+16) || w(v->port+18))return fail("DISPLAY / SETORIGIN FORM");}
        else {
            uint8_t* rgb=pointer(l(args),6);if(!rgb)return fail("DISPLAY / PORT RGB");
            uint16_t index;
            if(Palette8::rgb(rgb,0))index=255;
            else if(Palette8::rgb(rgb,65535))index=0;
            else {
                for(unsigned i=1;i<255;++i)if(w(*colors+8+i*8)!=0x4000)return fail("DISPLAY / RGB INDEX ENVIRONMENT");
                uint32_t sum=uint32_t(w(rgb))+w(rgb+2)+w(rgb+4);index=sum*2>=3UL*65535 ? 0 : 255;
            }
            CopyBits8::copySpan(rgb,v->port+(trap==0xaa14?36:42),6);sl(v->port+(trap==0xaa14?80:84),index);
        }
        consumed=4;return true;
    }
    if(trap==0xa8ec) {
        if(!pointer((uint32_t)args,22) || l(args) || w(args+4))return fail("DISPLAY / COPYBITS MODE OR MASK");
        World *source=nullptr,*destination=nullptr;
        for(auto& v:worlds)if(v.port) {
            if(l(args+18)==(uint32_t)*v.pm || l(args+18)==(uint32_t)v.port+2)source=&v;
            if(l(args+14)==(uint32_t)*v.pm || l(args+14)==(uint32_t)v.port+2)destination=&v;
        }
        const uint8_t* from=pointer(l(args+10),8);const uint8_t* to=pointer(l(args+6),8);
        if(!source || !destination || !from || !to || source==destination || (!source->isScreen && !source->locked) || (!destination->isScreen && !destination->locked))return fail("DISPLAY / COPYBITS OWNERSHIP");
        if(!thePort || l(thePort)!=(uint32_t)destination->port)return fail("DISPLAY / COPYBITS CURRENT PORT");
        if(int32_t(int16_t(w(from+4)))-int16_t(w(from))!=int32_t(int16_t(w(to+4)))-int16_t(w(to)) ||
           int32_t(int16_t(w(from+6)))-int16_t(w(from+2))!=int32_t(int16_t(w(to+6)))-int16_t(w(to+2)))return fail("DISPLAY / COPYBITS SCALING");
        uint8_t drawn[8];
        if(!CopyBits8::copy(source->isScreen?pixels:*source->pixels,source->isScreen?640*480:heap->handleSize(source->pixels),w(*source->pm+4)&0x3fff,*source->pm+6,
            destination->isScreen?pixels:*destination->pixels,destination->isScreen?640*480:heap->handleSize(destination->pixels),w(*destination->pm+4)&0x3fff,*destination->pm+6,
            from,to,destination->port+16,*destination->vis+2,*destination->clip+2,drawn))return fail("DISPLAY / COPYBITS FORM");
        if(destination->isScreen)changed(*destination,drawn);
        consumed=22;return true;
    }
    if(trap==0xa86e) {
        uint32_t at=l(args);
        if(thePort || at<202 || !pointer(at-202,206))return fail("DISPLAY / INITGRAF GLOBALS");
        thePort=(uint8_t*)at;
        // Public QuickDraw globals. Private bytes remain opaque to application code.
        sl(thePort,0);sl(thePort-126,1);sl(thePort-122,(uint32_t)pixels);
        sw(thePort-118,80);GWorld8::rect(thePort-116,{0,0,480,640});
        for(unsigned i=0;i<8;++i) {
            thePort[-8+int(i)]=0;thePort[-16+int(i)]=255;
            thePort[-24+int(i)]=(i&1)?0x55:0xaa;
            thePort[-32+int(i)]=(i&1)?0x22:0x88;
            thePort[-40+int(i)]=(i&1)?0xdd:0x77;
        }
        regs[0]=0;consumed=4;return true;
    }
    if(trap==0xaa2a) {sl(args,(uint32_t)device);consumed=0;return true;}
    if(trap==0xa873 || trap==0xa874) {
        if(!thePort)return fail("DISPLAY / PORT BEFORE INITGRAF");
        if(trap==0xa873) {if(!byPort(l(args)))return fail("DISPLAY / UNOWNED PORT");sl(thePort,l(args));}
        else {uint8_t* output=pointer(l(args),4);if(!output)return fail("DISPLAY / GETPORT OUTPUT");sl(output,l(thePort));}
        consumed=4;return true;
    }
    if(trap!=0xab1d)return fail("DISPLAY / UNKNOWN TRAP");
    uint16_t selector=regs[0];
    if(selector==0) {
        uint8_t* output=pointer(l(args+18),4);uint8_t* bounds=pointer(l(args+12),8);
        if(!output || !bounds || l(args)!=2 || l(args+4)!=(uint32_t)device || l(args+8) || w(args+16)!=8)
            return fail("DISPLAY / NEWGWORLD FORM");
        World* v=create(bounds);if(!v)return fail("DISPLAY / NEWGWORLD ALLOCATION");
        sl(output,(uint32_t)v->port);sw(args+22,0);regs[0]=0;consumed=22;return true;
    }
    if(selector==5 || selector==6) {
        if(!thePort)return fail("DISPLAY / GWORLD BEFORE INITGRAF");
        if(selector==5) {
            uint8_t* outDevice=pointer(l(args),4);uint8_t* outPort=pointer(l(args+4),4);
            if(!outDevice || !outPort)return fail("DISPLAY / GETGWORLD OUTPUT");
            sl(outDevice,currentDevice);sl(outPort,l(thePort));
        } else {
            if(l(args) && l(args)!=(uint32_t)device)return fail("DISPLAY / SETGWORLD DEVICE");
            World* v=byPort(l(args+4));if(!v)return fail("DISPLAY / SETGWORLD PORT");
            sl(thePort,(uint32_t)v->port);currentDevice=(uint32_t)device;
        }
        consumed=8;return true;
    }
    if(selector==4) {
        World* v=byPort(l(args));if(!v || v->isScreen || (thePort && l(thePort)==(uint32_t)v->port))return fail("DISPLAY / DISPOSEGWORLD OWNERSHIP");
        dispose(*v);consumed=4;return true;
    }
    if(selector==1 || selector==2 || selector==15) {
        World* v=byMap(l(args));
        if(!v) {
            if(selector==15 && l(args)==(uint32_t)map) {sl(args+4,(uint32_t)pixels);regs[0]=(uint32_t)pixels;consumed=4;return true;}
            return fail("DISPLAY / PIXMAP OWNERSHIP");
        }
        if(v->isScreen) {
            if(selector!=15)return fail("DISPLAY / SCREEN PIXEL LOCK");
            sl(args+4,(uint32_t)pixels);regs[0]=(uint32_t)pixels;consumed=4;return true;
        }
        refresh(*v);
        if(selector==15) {
            GWorld8::PixelAddress result;
            if(!GWorld8::pixelAddress(w(*v->pm+14),l(*v->pm),(uint32_t)v->pixels,(uint32_t)*v->pixels,(uint32_t)*v->pm,v->locked,regs[0],result))return fail("DISPLAY / PIXEL BASE STATE");
            sl(args+4,result.result);regs[0]=result.d0;regs[8]=result.a0;regs[9]=result.a1;
        } else {
            uint8_t state=heap->state(v->pixels);v->locked=selector==1;
            if(heap->setState(v->pixels,v->locked?state|0x80:state&~0x80))return fail("DISPLAY / PIXEL LOCK");
            refresh(*v);if(v->locked)args[4]=1;
            regs[0]=0;regs[8]=(uint32_t)v->pixels;regs[9]=(uint32_t)*v->pm;
        }
        consumed=4;return true;
    }
    return fail("DISPLAY / QDEXTENSIONS SELECTOR");
}
}
#endif
