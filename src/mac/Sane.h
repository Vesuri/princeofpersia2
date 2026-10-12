#ifndef POP2_SANE_H
#define POP2_SANE_H
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif
// The five original FP68K operations, default extended precision / nearest-even.
// Arithmetic is integer-only. Unsupported encodings/results leave destination intact.
class Sane {
    typedef unsigned long long UInt64;
    static_assert(sizeof(UInt64)==8,"SANE requires a 64-bit integer");
    struct Wide {
        uint32_t w[6];
        Wide():w{0,0,0,0,0,0} {}
        explicit Wide(UInt64 n):w{(uint32_t)n,(uint32_t)(n>>32),0,0,0,0} {}
        bool bit(int32_t n) const { return n>=0 && n<192 && ((w[n/32]>>(n%32))&1); }
        int32_t top() const { for(int32_t n=191;n>=0;--n)if(bit(n))return n;return -1; }
        Wide left(uint16_t n) const {
            Wide out;
            for(int32_t i=0;i<192-n;++i)if(bit(i))out.w[(i+n)/32]|=uint32_t(1)<<((i+n)%32);
            return out;
        }
        int compare(const Wide& b) const {
            for(int i=5;i>=0;--i)if(w[i]!=b.w[i])return w[i]>b.w[i] ? 1 : -1;
            return 0;
        }
        void add(const Wide& b) {
            UInt64 carry=0;
            for(uint16_t i=0;i<6;++i) { UInt64 sum=UInt64(w[i])+b.w[i]+carry;w[i]=(uint32_t)sum;carry=sum>>32; }
        }
        void sub(const Wide& b) {
            UInt64 borrow=0;
            for(uint16_t i=0;i<6;++i) { UInt64 v=UInt64(b.w[i])+borrow;borrow=UInt64(w[i])<v;w[i]=(uint32_t)(UInt64(w[i])-v); }
        }
        void multiply(uint32_t n) {
            UInt64 carry=0;
            for(uint16_t i=0;i<6;++i) { UInt64 v=UInt64(w[i])*n+carry;w[i]=(uint32_t)v;carry=v>>32; }
        }
        UInt64 rounded(int32_t shift,bool& carry) const {
            UInt64 v=0;carry=false;
            for(int32_t i=0;i<64;++i)if(bit(i+shift))v|=UInt64(1)<<i;
            bool sticky=false;
            for(int32_t i=0;i<192 && i<shift-1;++i)sticky=sticky||bit(i);
            if(bit(shift-1) && (sticky || (v&1))) { ++v;carry=!v; }
            return v;
        }
    };
    struct Extended { UInt64 mantissa;int32_t exponent;bool negative; };
    static uint16_t word(const uint8_t* p) { return (uint16_t(p[0])<<8)|p[1]; }
    static uint32_t longword(const uint8_t* p) { return (uint32_t(word(p))<<16)|word(p+2); }
    static void putWord(uint8_t* p,uint16_t v) { p[0]=v>>8;p[1]=(uint8_t)v; }
    static bool decode(const uint8_t* p,Extended& x) {
        uint16_t e=word(p);x.negative=(e&0x8000)!=0;e&=0x7fff;
        x.mantissa=(UInt64(longword(p+2))<<32)|longword(p+6);
        if(e==0x7fff || (e && !(x.mantissa>>63)) || (!e && (x.mantissa>>63)))return false;
        x.exponent=(e ? e : 1)-16383-63;return true;
    }
    static bool encode(const Wide& n,int32_t exponent,bool negative,uint8_t* out) {
        int32_t top=n.top(),biased=top+exponent+16383;
        UInt64 mantissa=0;uint16_t e=0;
        if(top>=0) {
            if(biased>=0x7fff)return false;
            bool carry;
            if(biased>0) {
                mantissa=n.rounded(top-63,carry);
                if(carry) { mantissa=UInt64(1)<<63;++biased; }
                if(biased>=0x7fff)return false;
                e=(uint16_t)biased;
            } else {
                mantissa=n.rounded(-16445-exponent,carry);
                if(carry)return false;
                if(mantissa>>63)e=1;
            }
        }
        putWord(out,e|(negative ? 0x8000 : 0));
        for(uint16_t i=0;i<8;++i)out[2+i]=(uint8_t)(mantissa>>(56-8*i));
        return true;
    }
    static bool addWord(const Extended& x,int16_t n,uint8_t* out) {
        bool negative=n<0;uint32_t magnitude=negative ? -int32_t(n) : n;
        if(!magnitude)return encode(Wide(x.mantissa),x.exponent,x.mantissa && x.negative,out);
        if(!x.mantissa)return encode(Wide(magnitude),0,negative,out);
        // Beyond this separation the smaller operand is below a quarter ulp.
        if(x.exponent>128)return encode(Wide(x.mantissa),x.exponent,x.negative,out);
        if(x.exponent<-128)return encode(Wide(magnitude),0,negative,out);
        Wide a(x.mantissa),b(magnitude);int32_t exponent=x.exponent;
        if(exponent>0) { a=a.left((uint16_t)exponent);exponent=0; }
        else b=b.left((uint16_t)-exponent);
        bool sign=x.negative;
        if(sign==negative)a.add(b);
        else if(a.compare(b)>=0) { a.sub(b);if(a.top()<0)sign=false; }
        else { b.sub(a);a=b;sign=negative; }
        return encode(a,exponent,sign,out);
    }
public:
    static bool apply(uint16_t operation,const uint8_t* source,uint8_t* destination) {
        if(!destination || (operation!=0x16 && !source))return false;
        uint8_t result[10];uint16_t size=10;bool ok=false;Extended x;
        if(operation==0x200e) {
            int32_t value=(int16_t)word(source);
            ok=encode(Wide(value<0 ? -value : value),0,value<0,result);
        } else if(operation==0x1004) {
            if(!decode(destination,x))return false;
            uint32_t value=longword(source),e=(value>>23)&255,m=value&0x7fffff;
            if(e==255)return false;
            int32_t exponent=e ? int32_t(e)-127-23 : -149;
            if(e)m|=0x800000;
            Wide product(x.mantissa);product.multiply(m);
            ok=encode(product,x.exponent+exponent,x.negative!=((value>>31)!=0),result);
        } else if(operation==0x2000) {
            if(!decode(destination,x))return false;
            ok=addWord(x,(int16_t)word(source),result);
        } else if(operation==0x16) {
            if(!decode(destination,x))return false;
            UInt64 integer=x.mantissa;
            if(x.exponent<0)integer=x.exponent<=-64 ? 0 : integer>>(-x.exponent);
            ok=encode(Wide(integer),x.exponent<0 ? 0 : x.exponent,x.negative,result);
        } else if(operation==0x2010) {
            if(!decode(source,x))return false;
            if(x.mantissa && x.exponent>=0)return false; // larger than a signed word
            bool carry;UInt64 integer=Wide(x.mantissa).rounded(-x.exponent,carry);
            if(carry || integer>(x.negative ? 32768u : 32767u))return false;
            putWord(result,(uint16_t)(x.negative ? -int32_t(integer) : integer));size=2;ok=true;
        }
        if(ok)for(uint16_t i=0;i<size;++i)destination[i]=result[i];
        return ok;
    }
};
#endif
