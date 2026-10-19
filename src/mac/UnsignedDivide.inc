// Include inside a namespace with uint32_t available. Shared from Alone in the Dark.
// Freestanding 68020 builds have no 64-bit division runtime. Return a bounded
// 32-bit quotient using shifts/subtractions; never silently truncate overflow.
inline bool divide(unsigned long long numerator,unsigned long long denominator,uint32_t& result) {
    if(!denominator)return false;
    result=0;
    if(numerator<denominator)return true;
    uint32_t high=uint32_t(numerator>>32),low=uint32_t(numerator);
    unsigned nbits=high ? 64-__builtin_clz(high) : 32-__builtin_clz(low);
    high=uint32_t(denominator>>32);low=uint32_t(denominator);
    unsigned dbits=high ? 64-__builtin_clz(high) : 32-__builtin_clz(low);
    unsigned shift=nbits-dbits;
    // A 32-bit quotient can still fit when the bit-length difference is 32;
    // reject only when that actual quotient bit would be set.
    if(shift>=32) {
        if(shift>32 || numerator>=(denominator<<32))return false;
        shift=31;
    }
    if(shift)denominator=(static_cast<unsigned long long>((high<<shift)|(low>>(32-shift)))<<32)
                         | uint32_t(low<<shift);
    do {
        if(numerator>=denominator) {
            numerator-=denominator;result|=uint32_t(1)<<shift;
        }
        denominator>>=1;
    } while(shift--);
    return true;
}
