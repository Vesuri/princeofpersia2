#ifndef _PALETTE24BIT_H
#define _PALETTE24BIT_H

#include "Util.h"

class Palette24Bit {
public:
    Palette24Bit(const uint32_t* colors, uint16_t colorCount, uint16_t fade = 255, uint32_t fadeBaseColor = 0x000000);
    /* Allocation-free view. Both caller-owned arrays must outlive this object. */
    Palette24Bit(uint32_t* colors, uint32_t* current, uint16_t colorCount, uint16_t fade = 255, uint32_t fadeBaseColor = 0x000000);
    ~Palette24Bit();

    void setColors(const uint32_t* colors, uint16_t colorCount);
    __inline uint16_t colorCount() const;
    void setFade(uint16_t fade);
    __inline uint16_t fade() const;
    void setFadeBaseColor(uint32_t fadeBaseColor);
    bool update();

    inline uint32_t operator[](int index) { return currentColors[index]; }
    inline const uint32_t operator[](int index) const { return currentColors[index]; }

private:
    uint32_t* sourceColors;
    uint32_t* currentColors;
    uint16_t colorCount_;
    uint16_t fade_;
    uint32_t fadeBaseColor;
    bool isDirty;
    bool ownsColors;
};

#endif
