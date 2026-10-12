#ifndef POP2_CURSOR_VISIBILITY_H
#define POP2_CURSOR_VISIBILITY_H
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif
// QuickDraw's explicit hide count and temporary mouse-obscured state are distinct.
struct CursorVisibility {
    volatile int16_t level=0;
    volatile bool obscured=false;
    void init() { level=0;obscured=false; }
    bool visible() const { return level==0 && !obscured; }
    bool hide() { if(level==-32768)return false;--level;return true; }
    void show() { if(level<0)++level;else obscured=false; }
    bool obscure() { bool wasVisible=visible();obscured=true;return wasVisible; }
    void moved() { obscured=false; }
};
#endif
