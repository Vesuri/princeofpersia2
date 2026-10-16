/*
 * XADStuffItArsenicHandle.m
 *
 * Copyright (c) 2017-present, MacPaw Way Ltd. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
 */
/* Adapted to bounded C99 by Vesuri for Alone in the Dark.
 * Arithmetic/MTF/BWT format follows XADMaster; no Objective-C dependency.
 * Included by install.c to share buffered I/O and error handling. */
static const uint16_t RandomizationTable[]=
{
	0xee,  0x56,  0xf8,  0xc3,  0x9d,  0x9f,  0xae,  0x2c,
	0xad,  0xcd,  0x24,  0x9d,  0xa6, 0x101,  0x18,  0xb9,
	0xa1,  0x82,  0x75,  0xe9,  0x9f,  0x55,  0x66,  0x6a,
	0x86,  0x71,  0xdc,  0x84,  0x56,  0x96,  0x56,  0xa1,
	0x84,  0x78,  0xb7,  0x32,  0x6a,   0x3,  0xe3,   0x2,
	0x11, 0x101,   0x8,  0x44,  0x83, 0x100,  0x43,  0xe3,
	0x1c,  0xf0,  0x86,  0x6a,  0x6b,   0xf,   0x3,  0x2d,
	0x86,  0x17,  0x7b,  0x10,  0xf6,  0x80,  0x78,  0x7a,
	0xa1,  0xe1,  0xef,  0x8c,  0xf6,  0x87,  0x4b,  0xa7,
	0xe2,  0x77,  0xfa,  0xb8,  0x81,  0xee,  0x77,  0xc0,
	0x9d,  0x29,  0x20,  0x27,  0x71,  0x12,  0xe0,  0x6b,
	0xd1,  0x7c,   0xa,  0x89,  0x7d,  0x87,  0xc4, 0x101,
	0xc1,  0x31,  0xaf,  0x38,   0x3,  0x68,  0x1b,  0x76,
	0x79,  0x3f,  0xdb,  0xc7,  0x1b,  0x36,  0x7b,  0xe2,
	0x63,  0x81,  0xee,   0xc,  0x63,  0x8b,  0x78,  0x38,
	0x97,  0x9b,  0xd7,  0x8f,  0xdd,  0xf2,  0xa3,  0x77,
	0x8c,  0xc3,  0x39,  0x20,  0xb3,  0x12,  0x11,   0xe,
	0x17,  0x42,  0x80,  0x2c,  0xc4,  0x92,  0x59,  0xc8,
	0xdb,  0x40,  0x76,  0x64,  0xb4,  0x55,  0x1a,  0x9e,
	0xfe,  0x5f,   0x6,  0x3c,  0x41,  0xef,  0xd4,  0xaa,
	0x98,  0x29,  0xcd,  0x1f,   0x2,  0xa8,  0x87,  0xd2,
	0xa0,  0x93,  0x98,  0xef,   0xc,  0x43,  0xed,  0x9d,
	0xc2,  0xeb,  0x81,  0xe9,  0x64,  0x23,  0x68,  0x1e,
	0x25,  0x57,  0xde,  0x9a,  0xcf,  0x7f,  0xe5,  0xba,
	0x41,  0xea,  0xea,  0x36,  0x1a,  0x28,  0x79,  0x20,
	0x5e,  0x18,  0x4e,  0x7c,  0x8e,  0x58,  0x7a,  0xef,
	0x91,   0x2,  0x93,  0xbb,  0x56,  0xa1,  0x49,  0x1b,
	0x79,  0x92,  0xf3,  0x58,  0x4f,  0x52,  0x9c,   0x2,
	0x77,  0xaf,  0x2a,  0x8f,  0x49,  0xd0,  0x99,  0x4d,
	0x98, 0x101,  0x60,  0x93, 0x100,  0x75,  0x31,  0xce,
	0x49,  0x20,  0x56,  0x57,  0xe2,  0xf5,  0x26,  0x2b,
	0x8a,  0xbf,  0xde,  0xd0,  0x83,  0x34,  0xf4,  0x17
};

#define ARSENIC_LIMIT (1UL<<20)
typedef struct { unsigned frequency[128],first,count,increment,limit,total; } Model;
static Model initial,selector,mtfmodel[7];
static unsigned char mtflist[256];
static uint32_t counts[256],arange,acode,crc32table[256];
static unsigned char *block;
static uint32_t *transform;
static unsigned abit,abits;
static void model_reset(Model *m) {
    unsigned i; m->total=m->count*m->increment;
    for(i=0;i<m->count;i++) m->frequency[i]=m->increment;
}
static void model_init(Model *m,unsigned first,unsigned last,unsigned increment,unsigned limit) {
    m->first=first; m->count=last-first+1; m->increment=increment; m->limit=limit; model_reset(m);
}
static unsigned next_bit(void) {
    unsigned v;
    if(!abits) { abit=getbyte(); abits=8; }
    v=(abit>>7)&1; abit<<=1; abits--; return v;
}
static unsigned arithmetic(Model *m) {
    uint32_t factor,frequency,cumulative=0,low; unsigned n,i;
    if(error) return 0;
    factor=arange/m->total;
    if(!factor) { fail("Invalid Arsenic arithmetic range."); return 0; }
    frequency=acode/factor;
    for(n=0;n<m->count-1;n++) {
        if(cumulative+m->frequency[n]>frequency) break;
        cumulative+=m->frequency[n];
    }
    low=factor*cumulative;
    if(acode<low || acode>=arange) { fail("Invalid Arsenic arithmetic code."); return 0; }
    acode-=low;
    if(cumulative+m->frequency[n]==m->total) arange-=low;
    else arange=m->frequency[n]*factor;
    while(arange<=(1UL<<24) && !error) { arange<<=1; acode=(acode<<1)|next_bit(); }
    m->frequency[n]+=m->increment; m->total+=m->increment;
    if(m->total>m->limit) {
        m->total=0;
        for(i=0;i<m->count;i++) { m->frequency[i]=(m->frequency[i]+1)>>1; m->total+=m->frequency[i]; }
    }
    return n+m->first;
}
static uint32_t arithmetic_bits(unsigned n) {
    uint32_t v=0; unsigned i;
    for(i=0;i<n && !error;i++) v|=(uint32_t)arithmetic(&initial)<<i;
    return v;
}
static unsigned mtf(unsigned index) {
    unsigned v=mtflist[index];
    while(index) { mtflist[index]=mtflist[index-1]; index--; }
    mtflist[0]=(unsigned char)v; return v;
}
static int arsenic(void) {
    unsigned blockbits,i,j,sel,randomized,end,count,last,v,randindex;
    uint32_t blocksize,n,index,zero,state,bytecount,randcount,sum,next,crc32=0xffffffffUL,expected=0;
    abit=abits=0; arange=1UL<<25; acode=0;
    for(i=0;i<26;i++) acode=(acode<<1)|next_bit();
    model_init(&initial,0,1,1,256); model_init(&selector,0,10,8,1024);
    model_init(&mtfmodel[0],2,3,8,1024); model_init(&mtfmodel[1],4,7,4,1024);
    model_init(&mtfmodel[2],8,15,4,1024); model_init(&mtfmodel[3],16,31,4,1024);
    model_init(&mtfmodel[4],32,63,2,1024); model_init(&mtfmodel[5],64,127,2,1024);
    model_init(&mtfmodel[6],128,255,1,1024);
    REQUIRE(arithmetic_bits(8)=='A' && arithmetic_bits(8)=='s',"Invalid Arsenic signature.");
    blockbits=arithmetic_bits(4)+9; blocksize=1UL<<blockbits;
    REQUIRE(blocksize<=ARSENIC_LIMIT,"Arsenic block exceeds supported 1 MiB limit.");
    block=io_alloc(blocksize); transform=io_alloc(blocksize*sizeof(uint32_t));
    REQUIRE(block && transform,"Not enough memory for Arsenic extraction.");
    for(i=0;i<256;i++) {
        next=i; for(j=0;j<8;j++) next=(next>>1)^((next&1)?0xedb88320UL:0);
        crc32table[i]=next;
    }
    end=arithmetic(&initial);
    while(!end && !error) {
        for(i=0;i<256;i++) mtflist[i]=(unsigned char)i;
        randomized=arithmetic(&initial); index=arithmetic_bits(blockbits); n=0;
        for(;;) {
            sel=arithmetic(&selector);
            REQUIRE(!error,"Invalid Arsenic selector.");
            if(sel<2) {
                state=1; zero=0;
                while(sel<2 && !error) {
                    REQUIRE(state<=blocksize && (sel+1)*state<=blocksize-zero,"Arsenic zero run exceeds block.");
                    zero+=(sel+1)*state; state<<=1; sel=arithmetic(&selector);
                }
                REQUIRE(zero<=blocksize-n,"Arsenic block overflow.");
                memset(block+n,mtf(0),zero); n+=zero;
            }
            REQUIRE(!error && sel<=10,"Invalid Arsenic selector.");
            if(sel==10) break;
            v=sel==2?1:arithmetic(&mtfmodel[sel-3]);
            REQUIRE(!error && v<256 && n<blocksize,"Arsenic block overflow.");
            block[n++]=(unsigned char)mtf(v);
        }
        REQUIRE(index<n,"Invalid Arsenic BWT index.");
        model_reset(&selector); for(i=0;i<7;i++) model_reset(&mtfmodel[i]);
        end=arithmetic(&initial); if(end) expected=arithmetic_bits(32);
        memset(counts,0,sizeof(counts));
        for(bytecount=0;bytecount<n;bytecount++) counts[block[bytecount]]++;
        sum=0; for(i=0;i<256;i++) { next=sum+counts[i]; counts[i]=sum; sum=next; }
        for(bytecount=0;bytecount<n;bytecount++) transform[counts[block[bytecount]]++]=bytecount;
        count=last=randindex=0; randcount=RandomizationTable[0];
        for(bytecount=0;bytecount<n;bytecount++) {
            index=transform[index]; v=block[index];
            if(randomized && randcount==bytecount) { v^=1; randindex=(randindex+1)&255; randcount+=RandomizationTable[randindex]; }
            if(count==4) {
                count=0;
                while(v--) { TRY(emit(last)); crc32=(crc32>>8)^crc32table[(crc32^last)&255]; }
            } else {
                if(v==last) count++; else { count=1; last=v; }
                TRY(emit(v)); crc32=(crc32>>8)^crc32table[(crc32^v)&255];
            }
        }
        REQUIRE(count!=4,"Truncated Arsenic RLE count.");
    }
    REQUIRE(!error && produced==wanted && expected==~crc32,"Arsenic size or CRC32 mismatch.");
    io_free(transform); transform=0; io_free(block); block=0;
    return flush_output();
}
