#include <fstream>
#include <vector>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include "mac/BitmapFont.h"
std::vector<uint8_t> read(const char* p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){assert(argc==5);auto a=read(argv[1]),b=read(argv[2]);BitmapFont::Family family;assert(BitmapFont::association(a.data(),a.size(),std::atoi(argv[3]),std::atoi(argv[4]),0,family));BitmapFont font;assert(font.openBitmap(b.data(),b.size()));std::printf("METRICS %u %u %u %u\n",font.ascent(),font.descent(),font.advance(),font.leading());for(unsigned i=0;i<256;++i)std::printf("WIDTH %u %u\n",i,font.integerWidth(i));for(unsigned n=0;n<b.size();++n){BitmapFont bad;assert(!bad.openBitmap(b.data(),n));}}
