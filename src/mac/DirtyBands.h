#ifndef AMIGAXDEV_DIRTY_BANDS_H
#define AMIGAXDEV_DIRTY_BANDS_H
#ifndef __mc68000__
#include <stdint.h>
#endif
// Coordinate-only planner extracted from Alone in the Dark. Caller supplies
// viewport dimensions, bounded input storage and a signed 16-bit rectangle.
namespace DirtyBands {
// Kalms needs a width divisible by 32, but its output may start on any word.
// Anchoring the block at a 16-pixel boundary avoids expanding a narrow actor
// across two fixed 32-pixel columns. Keep the final block inside the viewport.
template<int Width,class Rect>
inline void alignSpan(Rect& r) {
    r.left=int16_t(r.left&~15);
    r.right=int16_t(r.left+((r.right-r.left+31)&~31));
    if(r.right>Width) {r.left-=r.right-Width;r.right=Width;}
}

// Plan disjoint conversion bands from clipped draw coordinates, before alignment.
// No pixels are read. Each band has a constant set of dirty horizontal intervals;
// aligned overlaps become one span, then identical adjacent bands coalesce.
template<int Width,int Height,unsigned MaxDirty,class Rect>
inline bool plan(const Rect& viewport,const Rect* dirty,uint16_t dirtyCount,
                 Rect* output,uint16_t capacity,uint16_t& count) {
    if((viewport.top<0 || viewport.left<0 || viewport.bottom>480 || viewport.right>640 || viewport.bottom-viewport.top!=Height || viewport.right-viewport.left!=Width) || dirtyCount>MaxDirty || !capacity
       || (dirtyCount && !dirty) || !output)return false;
    Rect clipped[MaxDirty],bounds={Height,Width,0,0};uint16_t n=0;
    for(uint16_t i=0;i<dirtyCount;++i) {
        const Rect& d=dirty[i];
        if(d.top>d.bottom || d.left>d.right)return false;
        const int32_t top=d.top>viewport.top ? d.top : viewport.top;
        const int32_t left=d.left>viewport.left ? d.left : viewport.left;
        const int32_t bottom=d.bottom<viewport.bottom ? d.bottom : viewport.bottom;
        const int32_t right=d.right<viewport.right ? d.right : viewport.right;
        if(top>=bottom || left>=right)continue;
        Rect r={int16_t(top-viewport.top),int16_t(left-viewport.left),
                int16_t(bottom-viewport.top),int16_t(right-viewport.left)};
        clipped[n++]=r;
        if(r.top<bounds.top)bounds.top=r.top;
        if(r.left<bounds.left)bounds.left=r.left;
        if(r.bottom>bounds.bottom)bounds.bottom=r.bottom;
        if(r.right>bounds.right)bounds.right=r.right;
    }
    count=0;
    if(!n)return true;
    // Common one-rectangle publication needs no sweep.
    if(n==1) {alignSpan<Width>(clipped[0]);output[count++]=clipped[0];return true;}
    for(int16_t top=bounds.top;top<bounds.bottom;) {
        int16_t bottom=bounds.bottom;
        struct Span {int16_t left,right;} spans[MaxDirty];uint16_t spanCount=0;
        for(uint16_t i=0;i<n;++i) {
            const Rect& r=clipped[i];
            if(r.top>top && r.top<bottom)bottom=r.top;
            if(r.bottom>top && r.bottom<bottom)bottom=r.bottom;
            if(r.top>top || r.bottom<=top)continue;
            uint16_t at=spanCount++;
            while(at && spans[at-1].left>r.left) {spans[at]=spans[at-1];--at;}
            spans[at]={r.left,r.right};
        }
        for(uint16_t i=0;i<spanCount;) {
            Rect r={top,spans[i].left,bottom,spans[i].right};++i;
            // Union actual touching/overlapping coordinates before alignment.
            while(i<spanCount && spans[i].left<=r.right) {
                if(spans[i].right>r.right)r.right=spans[i].right;
                ++i;
            }
            alignSpan<Width>(r);
            while(i<spanCount) {
                Rect next={top,spans[i].left,bottom,spans[i].right};
                alignSpan<Width>(next);
                if(next.left>r.right)break;
                if(spans[i].right>r.right)r.right=spans[i].right;
                alignSpan<Width>(r);++i;
            }
            bool extended=false;
            for(uint16_t j=0;j<count;++j) {
                Rect& prior=output[j];
                if(prior.bottom==top && prior.left==r.left && prior.right==r.right) {
                    prior.bottom=bottom;extended=true;break;
                }
            }
            if(!extended) {
                if(count==capacity) {
                    // Bounded storage fallback covers all inputs, including later bands.
                    alignSpan<Width>(bounds);output[0]=bounds;count=1;return true;
                }
                output[count++]=r;
            }
        }
        top=bottom;
    }
    return true;
}

}
#endif
