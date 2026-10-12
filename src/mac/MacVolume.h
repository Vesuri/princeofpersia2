#ifndef POP2_MAC_VOLUME_H
#define POP2_MAC_VOLUME_H
// Physical backing information, read from DOS inside one system window.
struct MacVolumeBacking {
    uint32_t blocks,used,blockBytes,created,modified;
    bool locked;
    void reserveGrowth(uint32_t logical,uint32_t stored) {
        if(!blockBytes || used>blocks || logical<=stored)return;
        uint32_t after=logical/blockBytes+(logical%blockBytes!=0);
        uint32_t before=stored/blockBytes+(stored%blockBytes!=0);
        uint32_t extra=after-before;
        used=extra>blocks-used ? blocks : used+extra;
    }
};
#endif
