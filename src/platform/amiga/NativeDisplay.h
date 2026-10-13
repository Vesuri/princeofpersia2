#ifndef POP2_NATIVE_DISPLAY_H
#define POP2_NATIVE_DISPLAY_H
#include "mac/MacHeap.h"
class PrinceScreen;
namespace NativeDisplay {
using Valid=bool (*)(uint32_t,uint32_t);
using Resource=bool (*)(uint32_t,int16_t,const uint8_t*&,uint32_t&);
bool prepare(MacHeap& heap,uint8_t* low,Valid valid,Resource resource=nullptr);
void reset();
bool initWindowManager();
bool showOverlay(const uint8_t* image);
bool hideOverlay();
void attach(PrinceScreen* output,const uint8_t* transfer);
bool publish();
void wait();
void paceFrame();
bool handles(uint16_t trap);
bool dispatch(uint16_t trap,uint32_t* registers,uint8_t* arguments,uint32_t& consumed);
const char* error();
const uint8_t* screen();
}
#endif
