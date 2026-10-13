#ifdef POP2_CORE
#include "NativeDisplay.h"
#include "PrinceScreen.h"
#include "mac/GWorld8.h"
#include "mac/Palette8.h"
#include "mac/CopyBits8.h"
#include "mac/FillRect8.h"
#include "mac/RegionCopy.h"
#define AMIGAXDEV_REGION_UNION 1
#include "mac/RegionOps.h"
#include "mac/RectBounds.h"
#include "mac/BitmapFont.h"
#include "mac/BitmapText8.h"
#include "mac/AnimatePalette8.h"
extern "C" {uint32_t g_integrationScreen=0,g_integrationColors=0;}
namespace NativeDisplay {
namespace {
using H=MacHeap::Handle;
MacHeap* heap;Valid valid;Resource resource;uint8_t* low;uint8_t* pixels;uint8_t* thePort;
H device,map,colors;uint32_t currentDevice;
const char* reason="DISPLAY / NOT INITIALIZED";
struct World {uint8_t* port;H pm,pixels,vis,clip,vars,patterns[3];bool locked,isScreen;H palette;bool active;};
World worlds[16]={};
uint8_t regionWorkspace[32766];
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
    // MEASURED noNewDevice requests share the main CLUT and a 528-byte stride:
    // main buffer 512x384, opening text 510x87, title buffer 510x384.
    if(r.top || r.left || !(((width==512 || width==510) && height==384)||(width==510 && height==87))) {
        fail("DISPLAY / NEWGWORLD BOUNDS");return nullptr;
    }
    World* v=nullptr;for(auto& candidate:worlds)if(!candidate.port) {v=&candidate;break;}
    if(!v){fail("DISPLAY / GWORLD CAPACITY");return nullptr;}
    v->port=heap->newPtr(screen?156:108,true);
    if(!v->port){fail("DISPLAY / NEWGWORLD MEMORY");return nullptr;}
    v->pm=heap->newHandle(50,true);if(!screen)v->pixels=heap->newHandle(528*height,false);
    v->vis=heap->newHandle(10,true);v->clip=heap->newHandle(10,true);v->vars=heap->newHandle(46,true);
    for(unsigned i=0;i<3;++i)v->patterns[i]=heap->newHandle(28,true);
    if(!v->pm || (!screen && !v->pixels) || !v->vis || !v->clip || !v->vars || !v->patterns[0] || !v->patterns[1] || !v->patterns[2]) {dispose(*v);fail("DISPLAY / NEWGWORLD MEMORY");return nullptr;}
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
    H desktop=heap->newHandle(10,true);if(!desktop)return fail("UI / DESKTOP REGION MEMORY");
    sw(*desktop,10);GWorld8::rect(*desktop+2,{20,0,480,640});sl(low+0xbaa,(uint32_t)desktop);
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
    g_integrationScreen=(uint32_t)pixels;g_integrationColors=(uint32_t)colors;
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
    return trap==0xa88b || trap==0xa88d || trap==0xa88c || trap==0xa893 || trap==0xa884 || trap==0xa8a8 || trap==0xa8a9 || trap==0xa8ab || trap==0xa8ae || trap==0xa8aa || trap==0xa861 || trap==0xa8d8 || trap==0xa8d9 || trap==0xa8df || trap==0xa8e5 || trap==0xa879 || trap==0xa87a || trap==0xa90d || trap==0xa90a || trap==0xa887 || trap==0xa888 || trap==0xa88a || trap==0xa8a2 || trap==0xa87b || trap==0xa8a7 || trap==0xa91c || trap==0xa889 || trap==0xa89e || trap==0xa863 || trap==0xa8a3 || trap==0xaaa0 || trap==0xa91f || trap==0xa924 || trap==0xaa46 || trap==0xaa91 || trap==0xaa9c || trap==0xaa9e || trap==0xaa95 || trap==0xaa94 || trap==0xaa9a || trap==0xaa14 || trap==0xaa15 || trap==0xa915 || trap==0xa916 || trap==0xa91b || trap==0xa877 || trap==0xa878 || trap==0xa8ec || trap==0xa86e || trap==0xaa2a || trap==0xab1d || trap==0xa873 || trap==0xa874;
}
bool dispatch(uint16_t trap,uint32_t* regs,uint8_t* args,uint32_t& consumed) {
    if(!heap || !valid || !pointer((uint32_t)args,trap==0xab1d && uint16_t(regs[0])==0 ? 24 : (trap==0xab1d ? 8 : 4)))return fail("DISPLAY / ARGUMENT EXTENT");
    if(trap==0xa893){
        World* v=thePort?byPort(l(thePort)):nullptr;
        if(!v || !pointer((uint32_t)args,4))return fail("TEXT / MOVETO PORT");
        sw(v->port+48,w(args));sw(v->port+50,w(args+2));sw(v->port+14,0x8000);consumed=4;return true;
    }
    if(trap==0xa88b || trap==0xa88d || trap==0xa88c || trap==0xa884){
        World* v=thePort?byPort(l(thePort)):nullptr;
        if(!v || !resource || l(v->port+76))return fail("TEXT / FONT SELECTION");
        const uint8_t* bytes;uint32_t size;BitmapFont::Family family;BitmapFont font;
        if(!resource(0x464f4e44,int16_t(w(v->port+68)),bytes,size) ||
           !BitmapFont::association(bytes,size,int16_t(w(v->port+68)),w(v->port+74),v->port[70],family) ||
           !resource(0x4e464e54,family.bitmap,bytes,size) || !font.openBitmap(bytes,size))return fail("TEXT / FONT RESOURCE");
        if(trap==0xa88b){
            uint8_t* out=pointer(l(args),8);if(!out)return fail("TEXT / FONTINFO OUTPUT");
            sw(out,font.ascent());sw(out+2,font.descent());sw(out+4,font.advance());sw(out+6,font.leading());consumed=4;return true;
        }
        if(trap==0xa88d){sw(args+2,font.integerWidth(w(args)));consumed=2;return true;}
        const uint8_t* text=pointer(l(args),1);if(!text || !pointer(l(args),uint32_t(text[0])+1))return fail("TEXT / STRING EXTENT");
        if(trap==0xa88c){
            if(!pointer((uint32_t)args,6))return fail("TEXT / STRINGWIDTH RESULT");
            uint32_t width=0;for(unsigned i=0;i<text[0];++i)width+=font.integerWidth(text[i+1]);
            if(width>32767)return fail("TEXT / STRINGWIDTH OVERFLOW");sw(args+4,width);consumed=4;return true;
        }
        if(w(v->port+72)!=1 || w(v->port+66) || w(*v->clip)!=10 || w(*v->vis)!=10 ||
           l(v->port+80)>255 || (!v->isScreen && !v->locked))return fail("TEXT / DRAWSTRING FORM");
        int16_t pen=int16_t(w(v->port+50));uint8_t dirtyText[8];
        if(!BitmapText8::draw(v->isScreen?pixels:*v->pixels,v->isScreen?640*480:heap->handleSize(v->pixels),
            w(*v->pm+4)&0x3fff,*v->pm+6,v->port+16,*v->vis+2,*v->clip+2,font,text+1,text[0],
            int16_t(w(v->port+48)),pen,uint8_t(l(v->port+80)),dirtyText))return fail("TEXT / DRAWSTRING EXTENT");
        sw(v->port+50,uint16_t(pen));if(v->isScreen)changed(*v,dirtyText);consumed=4;return true;
    }
    if(trap==0xa8a8 || trap==0xa8a9){
        if(!pointer((uint32_t)args,8))return fail("DISPLAY / RECT ADJUST ARGUMENTS");
        uint8_t* rect=pointer(l(args+4),8);if(!rect)return fail("DISPLAY / RECT ADJUST EXTENT");
        RectBounds::adjust(rect,w(args+2),w(args),trap);consumed=8;return true;
    }
    if(trap==0xa8ab){
        if(!pointer((uint32_t)args,12) || !RectBounds::unite(pointer(l(args),8),pointer(l(args+8),8),pointer(l(args+4),8)))return fail("DISPLAY / UNIONRECT EXTENT");
        consumed=12;return true;
    }
    if(trap==0xa8ae){
        if(!pointer((uint32_t)args,6))return fail("DISPLAY / EMPTYRECT ARGUMENTS");
        const uint8_t* rect=pointer(l(args),8);if(!rect)return fail("DISPLAY / EMPTYRECT EXTENT");
        args[4]=(int16_t(w(rect))>=int16_t(w(rect+4)) || int16_t(w(rect+2))>=int16_t(w(rect+6)))?1:0;
        consumed=4;return true;
    }
    if(trap==0xa8aa){
        if(!pointer((uint32_t)args,14))return fail("DISPLAY / SECTRECT ARGUMENTS");
        bool nonempty=false;
        if(!RectBounds::intersect(pointer(l(args),8),pointer(l(args+8),8),pointer(l(args+4),8),nonempty))return fail("DISPLAY / SECTRECT EXTENT");
        args[12]=nonempty?1:0;consumed=12;return true;
    }
    if(trap==0xa861){
        if(!thePort)return fail("DISPLAY / RANDOM BEFORE INITGRAF");
        uint32_t seed=l(thePort-126),product=16807UL*uint16_t(seed);
        uint32_t folded=16807UL*(seed>>16)+(product>>16);
        seed=((folded&0x7fff)<<16)+((folded>>15)&65535)+(product&65535);
        sl(thePort-126,seed);sw(args,uint16_t(seed)==0x8000?0:seed);return true;
    }
    if(trap==0xa8d8){
        H h=heap->newHandle(10,true);if(!h)return fail("REGION / NEW MEMORY");
        sw(*h,10);sl(args,(uint32_t)h);regs[0]=0;regs[8]=(uint32_t)*h+10;return true;
    }
    if(trap==0xa8d9){
        H h=(H)l(args);if(!heap->isHandle(h) || heap->disposeHandle(h))return fail("REGION / DISPOSE OWNER");
        consumed=4;regs[0]=0;return true;
    }
    if(trap==0xa8df){
        if(!pointer((uint32_t)args,8))return fail("REGION / RECT ARGUMENTS");
        const uint8_t* r=pointer(l(args),8);H h=(H)l(args+4);uint8_t bounds[8];
        if(!r || !heap->isHandle(h))return fail("REGION / RECT OWNER");
        CopyBits8::copySpan(r,bounds,8);
        if(heap->setHandleSize(h,10))return fail("REGION / RECT MEMORY");
        if(int16_t(w(bounds))>=int16_t(w(bounds+4)) || int16_t(w(bounds+2))>=int16_t(w(bounds+6)))for(auto& b:bounds)b=0;
        sw(*h,10);CopyBits8::copySpan(bounds,*h+2,8);regs[0]=0;consumed=8;return true;
    }
    if(trap==0xa8e5){
        if(!pointer((uint32_t)args,12))return fail("REGION / UNION ARGUMENTS");
        H out=(H)l(args),b=(H)l(args+4),a=(H)l(args+8);uint16_t size=0;
        if(!heap->isHandle(out)||!heap->isHandle(a)||!heap->isHandle(b) || !*a || !*b || heap->handleSize(a)>32766 || heap->handleSize(b)>32766)return fail("REGION / UNION OWNER");
        if(!RegionOps::combine(*a,heap->handleSize(a),*b,heap->handleSize(b),RegionOps::Union,regionWorkspace,sizeof regionWorkspace,size))return fail("REGION / UNION FORM");
        if(heap->setHandleSize(out,size))return fail("REGION / UNION MEMORY");
        CopyBits8::copySpan(regionWorkspace,*out,size);regs[0]=0;consumed=12;return true;
    }
    if(trap==0xa879 || trap==0xa87a){
        World* v=thePort?byPort(l(thePort)):nullptr;H h=(H)l(args);uint16_t size=0;
        if(!v || !heap->isHandle(h))return fail("REGION / CLIP OWNER");
        H source=trap==0xa879?h:v->clip,destination=trap==0xa879?v->clip:h;
        if(RegionCopy::copy(*heap,source,*heap,destination,size))return fail("REGION / CLIP COPY");
        consumed=4;return true;
    }
    if(trap==0xa90d || trap==0xa90a){
        if(!pointer((uint32_t)args,8))return fail("UI / BEHIND ARGUMENTS");
        World* v=byPort(l(args+4));H region=(H)l(args);
        if(!v || !v->isScreen || !heap->isHandle(region))return fail("UI / BEHIND OWNER");
        // D8 removes desktop chrome and other Macintosh windows behind the client.
        consumed=8;regs[0]=0;return true;
    }
    if(trap==0xa887 || trap==0xa888 || trap==0xa88a){
        World* v=thePort?byPort(l(thePort)):nullptr;if(!v)return fail("DISPLAY / TEXT PORT");
        if(trap==0xa888)v->port[70]=args[0];else sw(v->port+(trap==0xa887?68:74),w(args));
        consumed=2;return true;
    }
    if(trap==0xa87b) {
        World* v=thePort?byPort(l(thePort)):nullptr;const uint8_t* r=pointer(l(args),8);
        if(!v || !r)return fail("DISPLAY / CLIPRECT OWNER");
        uint8_t bounds[8];CopyBits8::copySpan(r,bounds,8);
        if(heap->setHandleSize(v->clip,10))return fail("DISPLAY / CLIPRECT MEMORY");
        if(int16_t(w(bounds))>=int16_t(w(bounds+4)) || int16_t(w(bounds+2))>=int16_t(w(bounds+6)))for(auto& b:bounds)b=0;
        sw(*v->clip,10);CopyBits8::copySpan(bounds,*v->clip+2,8);consumed=4;return true;
    }
    if(trap==0xa8a7) {
        if(!pointer((uint32_t)args,12))return fail("DISPLAY / SETRECT ARGUMENTS");
        uint8_t* r=pointer(l(args+8),8);if(!r)return fail("DISPLAY / SETRECT OUTPUT");
        uint16_t bottom=w(args),right=w(args+2),top=w(args+4),left=w(args+6);
        GWorld8::rect(r,{int16_t(top),int16_t(left),int16_t(bottom),int16_t(right)});
        regs[0]=uint32_t(top)<<16|left;regs[1]=uint32_t(bottom)<<16|right;
        consumed=12;return true;
    }
    if(trap==0xa91c) {
        if(!pointer((uint32_t)args,6))return fail("UI / HILITE WINDOW ARGUMENTS");
        World* v=byPort(l(args+2));if(!v||!v->isScreen)return fail("UI / HILITE WINDOW OWNER");
        v->port[111]=args[0]?255:0;consumed=6;regs[0]=0;return true;
    }
    if(trap==0xa889 || trap==0xa89e || trap==0xa863 || trap==0xa8a3 || trap==0xa8a2) {
        World* v=thePort?byPort(l(thePort)):nullptr;if(!v)return fail("DISPLAY / CURRENT PORT");
        if(trap==0xa889) {sw(v->port+72,w(args));consumed=2;return true;}
        if(trap==0xa89e) {
            sw(v->port+52,1);sw(v->port+54,1);sw(v->port+56,8);
            uint8_t* pattern=*v->patterns[1];sw(pattern,0);
            for(unsigned i=0;i<8;++i)pattern[16+i]=255;
            regs[0]=0;return true;
        }
        if(trap==0xa863) {
            if(l(args)!=33 && l(args)!=30)return fail("DISPLAY / BACKCOLOR FORM");
            uint16_t rgb=l(args)==33?0:65535;
            for(unsigned i=0;i<3;++i)sw(v->port+42+i*2,rgb);
            sl(v->port+84,rgb?0:255);consumed=4;return true;
        }
        if(w(*v->clip)!=10 || w(*v->vis)!=10)return fail("DISPLAY / FILL COMPLEX REGION");
        if(trap==0xa8a2){
            if(w(v->port+56)!=8 || w(*v->patterns[1]))return fail("DISPLAY / PAINT PEN FORM");
            for(unsigned i=0;i<8;++i)if((*v->patterns[1])[16+i]!=255)return fail("DISPLAY / PAINT PATTERN");
        }
        const uint8_t* rect=pointer(l(args),8);uint8_t drawn[8];
        if(!rect || (!v->isScreen && !v->locked))return fail("DISPLAY / ERASE RECT OWNER");
        if(!FillRect8::solid(v->isScreen?pixels:*v->pixels,v->isScreen?640*480:heap->handleSize(v->pixels),
            w(*v->pm+4)&0x3fff,*v->pm+6,v->port+16,*v->vis+2,*v->clip+2,rect,uint8_t(l(v->port+(trap==0xa8a2?80:84))),drawn))return fail("DISPLAY / ERASE RECT FORM");
        if(v->isScreen)changed(*v,drawn);consumed=4;regs[0]=0;return true;
    }
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
    if(trap==0xaaa0) {
        if(!pointer((uint32_t)args,8))return fail("DISPLAY / PALETTE2CTAB ARGUMENTS");
        Palette* p=palette((H)l(args+4));H destination=(H)l(args);
        if(!p || !heap->isHandle(destination))return fail("DISPLAY / PALETTE2CTAB OWNER");
        if(heap->setHandleSize(destination,2056))return fail("DISPLAY / PALETTE2CTAB MEMORY");
        // MEASURED: zero seed/flags, sequential indices, exact RGB words.
        sl(*destination,0);sw(*destination+4,0);sw(*destination+6,255);
        for(unsigned i=0;i<256;++i) {
            uint8_t* entry=*destination+8+i*8;sw(entry,i);
            CopyBits8::copySpan(*p->body+16+i*16,entry+2,6);
        }
        regs[0]=0;consumed=8;return true;
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
            bool visible;RectBounds::intersect(*v->vis+2,v->port+16,*v->pm+6,visible);
            int16_t top=-int16_t(w(*v->pm+6)),left=-int16_t(w(*v->pm+8));
            for(unsigned y=0;y<384;++y)for(unsigned x=0;x<512;++x)pixels[uint32_t(y+top)*640+x+left]=uint8_t(l(v->port+84));
            changed(*v,v->port+16);
        }
        if(trap==0xa916) {v->port[110]=v->port[111]=0;for(unsigned i=0;i<8;++i)(*v->vis)[i+2]=0;}
        if(trap!=0xa916 && v->port[110] && !activate(*v))return fail("DISPLAY / ACTIVATE PALETTE");
        consumed=4;return true;
    }
    if(trap==0xa877 || trap==0xa878 || trap==0xaa14 || trap==0xaa15) {
        World* v=thePort?byPort(l(thePort)):nullptr;if(!v)return fail("DISPLAY / CURRENT PORT");
        if(trap==0xa877) {if(!position(*v,int16_t(w(args+2)),int16_t(w(args))))return fail("DISPLAY / MOVEPORTTO FORM");}
        else if(trap==0xa878) {
            int16_t dy=int16_t(w(args))-int16_t(w(v->port+16));
            int16_t dx=int16_t(w(args+2))-int16_t(w(v->port+18));
            uint8_t* bounds[]={v->port+16,*v->pm+6,*v->vis+2};
            for(auto r:bounds)for(unsigned i=0;i<4;++i)sw(r+i*2,uint16_t(w(r+i*2)+(i&1?dx:dy)));
        }
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
        if(!source || !destination)return fail("DISPLAY / COPYBITS BITMAP OWNER");
        if(!from || !to)return fail("DISPLAY / COPYBITS RECT EXTENT");
        if(source==destination)return fail("DISPLAY / COPYBITS OVERLAP");
        World* current=thePort?byPort(l(thePort)):nullptr;
        if(!current)return fail("DISPLAY / COPYBITS CURRENT PORT");
        const bool currentDestination=current==destination;
        const uint8_t* bounds=*destination->pm+6;
        if(int32_t(int16_t(w(from+4)))-int16_t(w(from))!=int32_t(int16_t(w(to+4)))-int16_t(w(to)) ||
           int32_t(int16_t(w(from+6)))-int16_t(w(from+2))!=int32_t(int16_t(w(to+6)))-int16_t(w(to+2)))return fail("DISPLAY / COPYBITS SCALING");
        // MEASURED title copies both read and write unlocked GWorlds without
        // changing PixMap lock state. Owned heap pixels remain stable.
        // CopyBits applies port clipping only when its destination is the
        // current port bitmap; the title copy keeps the window current.
        uint8_t drawn[8];
        if(!CopyBits8::copy(source->isScreen?pixels:*source->pixels,source->isScreen?640*480:heap->handleSize(source->pixels),w(*source->pm+4)&0x3fff,*source->pm+6,
            destination->isScreen?pixels:*destination->pixels,destination->isScreen?640*480:heap->handleSize(destination->pixels),w(*destination->pm+4)&0x3fff,*destination->pm+6,
            from,to,currentDestination?destination->port+16:bounds,
            currentDestination?*destination->vis+2:bounds,currentDestination?*destination->clip+2:bounds,drawn))return fail("DISPLAY / COPYBITS FORM");
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
        World* v=create(bounds);if(!v)return false;
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
