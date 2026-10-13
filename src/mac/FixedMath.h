#pragma once
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif
namespace FixedMath {
inline uint32_t magnitude(int32_t n){return n<0?0-uint32_t(n):uint32_t(n);}
inline uint32_t multiply(int32_t a,int32_t b){
    bool negative=(a<0)!=(b<0);uint32_t limit=negative?0x80000000UL:0x7fffffffUL;
    unsigned long long result=((unsigned long long)(magnitude(a))*magnitude(b)+32768)>>16;
    if(result>limit)return limit;
    return negative?0-uint32_t(result):uint32_t(result);
}
inline uint32_t divide(int32_t a,int32_t b){
    bool negative=(a<0)!=(b<0);uint32_t limit=negative?0x80000000UL:0x7fffffffUL;
    uint32_t numerator=magnitude(a),denominator=magnitude(b);
    if(!denominator)return a<0?0x80000000UL:0x7fffffffUL;
    uint32_t whole=numerator/denominator,remainder=numerator%denominator;
    if(whole>=32768)return limit;
    uint32_t result=whole;
    for(unsigned i=0;i<16;++i){result<<=1;remainder<<=1;if(remainder>=denominator){remainder-=denominator;++result;}}
    // Measured System 7.5.5 FixDiv: round nonzero magnitudes at a half,
    // but an exact half-unit underflow remains zero.
    if(remainder>denominator-remainder || (result && remainder==denominator-remainder))++result;
    if(result>limit)return limit;
    return negative?0-result:result;
}
}
