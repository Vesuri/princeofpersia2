#ifndef POP2_PRINCE_SCREEN_H
#define POP2_PRINCE_SCREEN_H
#include "Planar8.h"
#include "AgaPalette.h"
// Owns one 512x384 indexed viewport; every field's copper list is immutable
// while active. No original game bytes or colour tables live in this class.
class PrinceScreen {
public:
    bool allocate();
    void start();
    void field();
    void paceFrame();
    void wait() {while(pending_) {__asm__ volatile("nop" ::: "memory");}}
    void quiesce();
    void release();
    bool publish(const uint8_t* chunky,const Planar8::Rect& viewport,
                 const Planar8::Rect* dirty,uint16_t count,const uint32_t* rgb,bool prepareScene=false);
    static constexpr uint16_t copperMoves=16+13+AgaPalette::moves+1;
private:
    uint8_t* pixels_[2]={};
    uint32_t* copper_=nullptr;
    volatile uint16_t front_=0,pending_=0;
    bool started_=false,valid_=false;
    uint32_t lastFrameField_=0;
    Planar8::Rect viewport_={};
    Planar8::Rect sync_[64];uint16_t syncCount_=0;
    void synchronize(const Planar8::Rect& r,uint16_t back);
    void lists(uint16_t buffer,const uint32_t* rgb);
};
#endif
