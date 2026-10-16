#ifndef AMIGAXDEV_BLITTER_COMPLETION_H
#define AMIGAXDEV_BLITTER_COMPLETION_H
// Completion policy for the shared AmigaHardware blitter queue. The owner
// installs an INTB_BLIT handler, excludes producers while servicing completion,
// bounds the queued batch to the framework capacity, and drains before CPU
// access, OS handback or freeing DMA storage. No interrupt is enabled globally
// by this helper. Existing synchronous framework consumers remain unchanged.
namespace BlitterCompletion {
template<class Hardware> inline void advance() {
    Hardware::setInterrupts(0x40,false);
    // Ack BEFORE starting the next blit: even a very short blit must retain
    // its completion request. A stale completion with an idle queue is safe.
    Hardware::clearInterruptRequests(0x40);
    Hardware::processBlitterQueue();
    if(Hardware::hasQueuedBlits || Hardware::isBlitterBusy())
        Hardware::setInterrupts(0x40,true);
}
template<class Hardware> inline void drain() {
    Hardware::blitterDrain();
    Hardware::clearInterruptRequests(0x40);
}
}
#endif
