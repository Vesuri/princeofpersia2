#pragma once
// Fixed-width types are supplied by the caller (including Amiga compat types).

// Register-state bookkeeping from Alone in the Dark's Paula restart protocol.
// The caller owns the hardware exclusively and performs the indicated waits.
// A silent volume is not a settled DAC: only the completed DMA-off/DAT-zero
// handshake permits markQuiet(). Reset this state on every hardware takeover.
class PaulaChannelState {
    uint16_t ceiling_=65535;
    bool quiet_=false;
    bool disabled_=false;
public:
    bool quiet() const {return quiet_;}
    uint16_t disableLines() const {
        return disabled_ ? 0 : uint16_t((uint32_t(ceiling_)*2)/227+2);
    }
    void disabled() {ceiling_=0;disabled_=true;}
    void period(uint16_t value) {if(value>ceiling_)ceiling_=value;}
    void markQuiet() {quiet_=true;}
    void started() {quiet_=false;disabled_=false;}
};
