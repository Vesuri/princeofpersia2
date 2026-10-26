#ifndef POP2_NATIVE_DISPLAY_H
#define POP2_NATIVE_DISPLAY_H
#include "mac/MacHeap.h"
class PrinceScreen;
namespace NativeDisplay {
using Valid=bool (*)(uint32_t,uint32_t);
using Resource=bool (*)(uint32_t,int16_t,const uint8_t*&,uint32_t&);
// The physical screen is display memory outside the application zone, as on
// the Macintosh. Kalms prefetches one unused longword after its final source
// span, so the caller supplies private readable padding after the PixMap extent.
enum { screenBytes=640*480+4 };
bool prepare(MacHeap& heap,uint8_t* low,uint8_t* screen,Valid valid,Resource resource=nullptr);
void reset();
bool initWindowManager();
bool showOverlay(const uint8_t* image);
bool hideOverlay();
void attach(PrinceScreen* output,const uint8_t* transfer);
bool publish(bool prepareScene=false);
void wait();
void paceFrame();
bool handles(uint16_t trap);
bool dispatch(uint16_t trap,uint32_t* registers,uint8_t* arguments,uint32_t& consumed);
const char* error();
const uint8_t* screen();
}
#endif
