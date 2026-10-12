#include "Palette24Bit.h"

Palette24Bit::Palette24Bit(const uint32_t* colors, uint16_t colorCount, uint16_t fade, uint32_t fadeBaseColor) :
    sourceColors(0),
    currentColors(0),
    fade_(fade),
    fadeBaseColor(fadeBaseColor),
    isDirty(false),
    ownsColors(true)
{
    setColors(colors, colorCount);
}

Palette24Bit::Palette24Bit(uint32_t* colors, uint32_t* current, uint16_t colorCount, uint16_t fade, uint32_t fadeBaseColor) :
    sourceColors(colors), currentColors(current), colorCount_(colorCount),
    fade_(fade), fadeBaseColor(fadeBaseColor), isDirty(true), ownsColors(false)
{
}

Palette24Bit::~Palette24Bit()
{
    if (ownsColors) {
        delete[] sourceColors;
        delete[] currentColors;
    }
}

void Palette24Bit::setColors(const uint32_t* colors, uint16_t colorCount)
{
    if (ownsColors) {
        delete[] sourceColors;
        delete[] currentColors;
    }

    ownsColors = true;
    sourceColors = new uint32_t[colorCount];
    currentColors = new uint32_t[colorCount];
    colorCount_ = colorCount;
    isDirty = true;

    if (colors) {
        for (int i = 0; i < colorCount; i++) {
            sourceColors[i] = colors[i];
        }
    }
}

uint16_t Palette24Bit::colorCount() const
{
    return colorCount_;
}

void Palette24Bit::setFade(uint16_t fade)
{
    if (fade_ != fade) {
        fade_ = fade;
        isDirty = true;
    }
}

uint16_t Palette24Bit::fade() const
{
    return fade_;
}

void Palette24Bit::setFadeBaseColor(uint32_t fadeBaseColor)
{
    if (this->fadeBaseColor != fadeBaseColor) {
        this->fadeBaseColor = fadeBaseColor;
        isDirty = true;
    }
}

bool Palette24Bit::update()
{
    if (isDirty) {
        if (fade_ == 0) {
            for (uint16_t i = 0; i < colorCount_; i++) {
                currentColors[i] = fadeBaseColor;
            }
        } else {
            uint16_t multiplier = fade_ + 1;
            uint32_t baseR = fadeBaseColor >> 16;
            uint32_t baseG = (fadeBaseColor & 0x00ff00) >> 8;
            uint32_t baseB = fadeBaseColor & 0x0000ff;
            for (uint16_t i = 0; i < colorCount_; i++) {
                uint32_t color = sourceColors[i];
                uint32_t r = color >> 16;
                uint32_t g = (color & 0x00ff00) >> 8;
                uint32_t b = color & 0x0000ff;
                int32_t rDelta = r - baseR;
                int32_t gDelta = g - baseG;
                int32_t bDelta = b - baseB;
                uint32_t fadedR = baseR + (rDelta >= 0 ? (uint32_t)((multiplier * rDelta) >> 8) : -(uint32_t)((multiplier * -rDelta) >> 8));
                uint32_t fadedG = baseG + (gDelta >= 0 ? (uint32_t)((multiplier * gDelta) >> 8) : -(uint32_t)((multiplier * -gDelta) >> 8));
                uint32_t fadedB = baseB + (bDelta >= 0 ? (uint32_t)((multiplier * bDelta) >> 8) : -(uint32_t)((multiplier * -bDelta) >> 8));
                currentColors[i] = (uint32_t)((fadedR << 16) | (fadedG << 8) | fadedB);
            }
        }

        isDirty = false;
        return true;
    }

    return false;
}
