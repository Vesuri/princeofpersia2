#include "mac/CopperPaletteCache.h"
#include "platform/amiga/AgaPalette.h"
#include <cassert>
#include <cstdio>
int main() {
 CopperPaletteCache::State<256,2> cache;uint32_t colors[256]={},lists[2][2][AgaPalette::moves],oracle[AgaPalette::moves];
 unsigned builds=0,skips=0,seed=17;
 for(unsigned step=0;step<2048;++step) {
  unsigned selected=step%2;
  // Includes repeated palettes, single-pen changes, zero palette, key-only
  // changes, reset, and changes before the other buffer can consume a palette.
  if(step%7==0){seed=seed*1664525+1013904223;colors[(seed>>8)%256]=seed&0xffffff;}
  if(step%29==0)for(auto& c:colors)c=0;
  if(step%127==0)cache.reset();
  unsigned key=(step/19)%2;
  cache.update(colors,key);
  AgaPalette::build(oracle,colors,key);
  if(cache.needs(selected)){
   for(unsigned parity=0;parity<2;++parity)AgaPalette::build(lists[selected][parity],cache.colors(),key);
   cache.mark(selected);++builds;
  } else ++skips;
  for(unsigned p=0;p<2;++p)for(unsigned i=0;i<AgaPalette::moves;++i)assert(lists[selected][p][i]==oracle[i]);
  assert(!cache.needs(selected));assert(!cache.update(colors,key));
 }
 assert(builds && skips);cache.reset();assert(!cache.needs(0));cache.mark(0);assert(cache.update(colors));assert(cache.needs(0)&&cache.needs(1));
 printf("COPPER PALETTE CACHE PASS publications=2048 builds=%u skips=%u\n",builds,skips);
}
