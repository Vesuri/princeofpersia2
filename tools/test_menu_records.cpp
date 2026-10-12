#include "../src/mac/MenuRecords.h"
#include <vector>
#include <cassert>
#include <cstdio>
#include <algorithm>
using Bytes=std::vector<uint8_t>;
static Bytes menu(const Bytes& first,const Bytes& second) {
    Bytes b(14,0);b[1]=128;b[10]=0x12;b[11]=0x34;b[12]=0x56;b[13]=0x78;
    b.push_back(4);for(auto c:Bytes{'M','e','n','u'})b.push_back(c);
    for(const auto* text:{&first,&second}) {
        b.push_back(text->size());b.insert(b.end(),text->begin(),text->end());
        b.insert(b.end(),{0x20,'X',0x12,0x03});
    }
    b.push_back(0);return b;
}
static void checkReplacement(unsigned length,unsigned number) {
    Bytes first={'F','i','r','s','t'},second={'S','e','c','o','n','d'};
    Bytes old=menu(first,second),text(length+1);text[0]=length;
    for(unsigned n=1;n<=length;++n)text[n]=uint8_t(n); // includes high MacRoman bytes
    MenuRecords::Item item;uint32_t size=0;
    assert(MenuRecords::replacement(old.data(),old.size(),number,length,item,size));
    Bytes edited=old;edited.resize(std::max<size_t>(old.size(),size)+16,0xa5);
    MenuRecords::replace(edited.data(),old.size(),item,text.data());
    for(unsigned n=std::max<size_t>(old.size(),size);n<edited.size();++n)assert(edited[n]==0xa5);
    edited.resize(size);
    Bytes label(text.begin()+1,text.end());Bytes expected=number==1?menu(label,second):menu(first,label);
    expected[2]=expected[3]=expected[4]=expected[5]=255;
    assert(edited==expected);
    uint8_t output[258];std::fill_n(output,258,0xcc);
    assert(MenuRecords::get(edited.data(),edited.size(),number,output+1));
    assert(output[0]==0xcc && output[length+2]==0xcc);
    assert(std::equal(text.begin(),text.end(),output+1));
}
int main() {
    for(unsigned n=1;n<=255;++n) { checkReplacement(n,1);checkReplacement(n,2); }
    Bytes b=menu({'A'},{'B'});MenuRecords::Item item;uint16_t count;
    assert(MenuRecords::scan(b.data(),b.size(),0,item,count) && count==2);
    for(unsigned n=0;n<b.size();++n)assert(!MenuRecords::scan(b.data(),n,0,item,count));
    uint32_t size;
    for(unsigned number:{0,3,65535})assert(!MenuRecords::replacement(b.data(),b.size(),number,4,item,size));
    assert(!MenuRecords::replacement(b.data(),b.size(),1,0,item,size));
    assert(!MenuRecords::replacement(b.data(),b.size(),1,256,item,size));
    b[14]=255;assert(!MenuRecords::scan(b.data(),b.size(),0,item,count));
    Bytes empty(16,0);assert(MenuRecords::scan(empty.data(),empty.size(),0,item,count) && count==0);
    assert(!MenuRecords::get(empty.data(),empty.size(),1,b.data()));
    std::puts("PASS menu records: 510 grow/shrink/equal replacements, exact flags/attributes/tails, output guards and malformed input");
}
