#ifndef POP2_WINDOW_GEOMETRY_H
#define POP2_WINDOW_GEOMETRY_H
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif
namespace WindowGeometry {
struct Rect { int16_t top,left,bottom,right; };
inline bool wordRange(int32_t v) {return v>=-32768 && v<=32767;}
inline bool layout(const Rect& frame,Rect& port,Rect& pixels) {
    int32_t height=int32_t(frame.bottom)-frame.top,width=int32_t(frame.right)-frame.left;
    int32_t top=-int32_t(frame.top),left=-int32_t(frame.left);
    if(height<=0 || width<=0 || height>32767 || width>32767
       || !wordRange(top) || !wordRange(left) || !wordRange(top+480) || !wordRange(left+640))return false;
    port={0,0,int16_t(height),int16_t(width)};
    pixels={int16_t(top),int16_t(left),int16_t(top+480),int16_t(left+640)};
    return true;
}
inline bool frame(const Rect& port,const Rect& pixels,Rect& result) {
    if(port.top || port.left || port.bottom<=0 || port.right<=0
       || int32_t(pixels.bottom)-pixels.top!=480 || int32_t(pixels.right)-pixels.left!=640)return false;
    int32_t top=-int32_t(pixels.top),left=-int32_t(pixels.left);
    if(!wordRange(top) || !wordRange(left) || !wordRange(top+port.bottom) || !wordRange(left+port.right))return false;
    result={int16_t(top),int16_t(left),int16_t(top+port.bottom),int16_t(left+port.right)};
    return true;
}
inline void word(uint8_t* out,uint16_t value) {out[0]=uint8_t(value>>8);out[1]=uint8_t(value);}
// Measured WDEF 4 structure: bordered/title rectangle and its one-pixel shadow.
// This is region metadata only; no Macintosh window chrome is drawn.
inline bool structure4(const Rect& frame,uint8_t* out) {
    int32_t top=int32_t(frame.top)-19,left=int32_t(frame.left)-1;
    int32_t bottom=int32_t(frame.bottom)+2,right=int32_t(frame.right)+2;
    if(!out || frame.bottom<=frame.top || frame.right<=frame.left
       || top<-32768 || left<-32768 || bottom>=32767 || right>=32767)return false;
    const int16_t values[22]={44,int16_t(top),int16_t(left),int16_t(bottom),int16_t(right),
        int16_t(top),int16_t(left),int16_t(right-1),32767,
        int16_t(top+1),int16_t(right-1),int16_t(right),32767,
        int16_t(bottom-1),int16_t(left),int16_t(left+1),32767,
        int16_t(bottom),int16_t(left+1),int16_t(right),32767,32767};
    for(uint16_t i=0;i<22;++i)word(out+i*2,uint16_t(values[i]));
    return true;
}
}
#endif
