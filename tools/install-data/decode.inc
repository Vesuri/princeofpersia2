/* Shared bounded StuffIt I/O and decoding from Vette!/Alone in the Dark.
 * LGPL-2.1-or-later; see COPYING.LIB. Adapter supplies workspaces and File I/O. */
static int fail(const char *s) { if(!error) error=s; return 0; }
static uint16_t be16(const unsigned char *p) { return (uint16_t)((p[0]<<8)|p[1]); }
static uint32_t be32(const unsigned char *p) { return ((uint32_t)be16(p)<<16)|be16(p+2); }
static int span(uint32_t off,uint32_t n,uint32_t size) { return off<=size && n<=size-off; }
static int readat(File *f,uint32_t off,void *p,uint32_t n) {
    REQUIRE(!io_cancelled(),"Installation cancelled.");
    REQUIRE(span(off,n,f->size),"Truncated file or invalid file offset.");
    REQUIRE((f->pos==off || io_seek(f,off)) && io_read(f,p,n),"Cannot read input file."); return 1;
}
static int writebytes(File *f,const void *p,uint32_t n) {
    REQUIRE(!io_cancelled(),"Installation cancelled.");
    REQUIRE(io_write(f,p,n),"Cannot write output (disk full or write error)."); return 1;
}
static int closefile(File *f) { REQUIRE(io_close(f),"Cannot close output file (write error)."); return 1; }
static int join(char *out,unsigned cap,const char *dir,const char *name) {
    size_t a=strlen(dir),b=strlen(name);
    REQUIRE(a+b+2<cap,"Installation path is too long.");
    memcpy(out,dir,a);
    if(a && dir[a-1]!='/' && dir[a-1]!=':') out[a++]='/';
    memcpy(out+a,name,b+1); return 1;
}

/* Bounded compressed input, LSB-first bit reader. */
static File *infile;
static uint32_t inoff,inleft;
static unsigned ipos,ilen,bits,nbits;
static void begin_input(File *f,uint32_t off,uint32_t len) {
    infile=f; inoff=off; inleft=len; ipos=ilen=bits=nbits=0;
}
static unsigned getbyte(void) {
    unsigned n;
    if(error) return 0;
    if(ipos==ilen) {
        if(!inleft) { fail("Truncated compressed stream."); return 0; }
        n=inleft>BUFSIZE?BUFSIZE:(unsigned)inleft;
        if(!readat(infile,inoff,input,n)) return 0;
        inoff+=n; inleft-=n; ipos=0; ilen=n;
    }
    return input[ipos++];
}
static uint32_t getbits(unsigned n) {
    uint32_t value=0; unsigned i;
    for(i=0;i<n;i++) {
        if(!nbits) { bits=getbyte(); nbits=8; }
        value|=(uint32_t)(bits&1)<<i; bits>>=1; nbits--;
    }
    return value;
}
static uint16_t crc_table[256];
static void crc_init(void) {
    unsigned i,j; uint16_t c;
    for(i=0;i<256;i++) { c=(uint16_t)i; for(j=0;j<8;j++) c=(uint16_t)((c>>1)^((c&1)?0xa001:0)); crc_table[i]=c; }
}
static uint16_t crc_byte(uint16_t c,unsigned b) { return (uint16_t)((c>>8)^crc_table[(c^b)&255]); }

/* The same output/history workspace serves SIT13 and ADC. Matches may overlap. */
static File *outfile;
static uint32_t produced,wanted;
static unsigned opos;
static uint16_t crc;
static void begin_output(File *f,uint32_t n) { outfile=f; wanted=n; produced=opos=crc=0; }
static int flush_output(void) { if(opos) { TRY(writebytes(outfile,output,opos)); opos=0; } return 1; }
static int emit(unsigned b) {
    REQUIRE(produced<wanted,"Decompressed data exceeds its declared size.");
    history[produced&65535]=(unsigned char)b; output[opos++]=(unsigned char)b;
    produced++; crc=crc_byte(crc,b);
    if(opos==BUFSIZE) TRY(flush_output());
    return !error;
}
static int match(uint32_t distance,uint32_t n) {
    REQUIRE(distance && distance<=65536 && distance<=produced,"Invalid compressed back-reference.");
    REQUIRE(n<=wanted-produced,"Compressed match exceeds declared output size.");
    while(n--) TRY(emit(history[(produced-distance)&65535]));
    return 1;
}

/* Prefix trees, iterative construction and traversal. A negative edge is a symbol. */
typedef struct { int16_t child[644][2]; unsigned nodes; } Tree;
static Tree trees[4];
static int lengths[321];
static void tree_init(Tree *t) { memset(t,0,sizeof(*t)); t->nodes=1; }
static int tree_add(Tree *t,uint32_t code,unsigned len,unsigned symbol,int lowfirst) {
    unsigned node=1,i,b; int16_t *edge;
    REQUIRE(len && len<=32,"Invalid Huffman code length.");
    for(i=0;i<len;i++) {
        b=(code>>(lowfirst?i:len-1-i))&1; edge=&t->child[node][b];
        if(i+1==len) { REQUIRE(!*edge,"Overlapping Huffman codes."); *edge=-(int16_t)(symbol+1); }
        else {
            REQUIRE(*edge>=0,"Overlapping Huffman codes.");
            if(!*edge) { REQUIRE(t->nodes+1<644,"Huffman tree is too large."); *edge=(int16_t)++t->nodes; }
            node=(unsigned)*edge;
        }
    }
    return 1;
}
static int canonical(Tree *t,unsigned count) {
    uint32_t code=0; unsigned len,i; int exhausted=0;
    tree_init(t);
    for(len=1;len<=32;len++) {
        if(len>1) { if(code&0x80000000UL) exhausted=1; code<<=1; }
        for(i=0;i<count;i++) if(lengths[i]==(int)len) {
            REQUIRE(!exhausted && (len==32 || code<(1UL<<len)),"Oversubscribed Huffman table.");
            TRY(tree_add(t,code,len,i,0)); if(code==0xffffffffUL) exhausted=1; code++;
        }
    }
    return 1;
}
static unsigned symbol(Tree *t) {
    int node=1; unsigned i;
    for(i=0;i<32 && !error;i++) {
        node=t->child[node][getbits(1)];
        if(node<0) return (unsigned)(-node-1);
        if(!node) break;
    }
    fail("Invalid Huffman symbol."); return 0;
}
static int dynamic_tree(Tree *t,unsigned count) {
    unsigned i=0,v,repeat; int length=0;
    while(i<count && !error) {
        v=symbol(&trees[3]); repeat=1;
        if(v<31) length=(int)v+1;
        else if(v==31) length=-1;
        else if(v==32) length++;
        else if(v==33) length--;
        else if(v==34) repeat=1+getbits(1);
        else if(v==35) repeat=3+getbits(3);
        else if(v==36) repeat=11+getbits(6);
        REQUIRE(length>=-1 && length<=32 && repeat<=count-i,"Invalid dynamic Huffman table.");
        while(repeat--) lengths[i++]=length;
    }
    REQUIRE(!error,"Invalid Huffman table."); return canonical(t,count);
}
static int sit13(void) {
    unsigned h=getbyte(),mode=h>>4,i,v,n,offbits,current=0; uint32_t distance;
    if(!mode) {
        tree_init(&trees[3]);
        for(i=0;i<37;i++) TRY(tree_add(&trees[3],MetaCodes[i],MetaCodeLengths[i],i,1));
        TRY(dynamic_tree(&trees[0],321));
        if(h&8) memcpy(&trees[1],&trees[0],sizeof(Tree));
        else TRY(dynamic_tree(&trees[1],321));
        TRY(dynamic_tree(&trees[2],(h&7)+10));
    } else {
        REQUIRE(mode<=5,"Unsupported StuffIt Huffman table.");
        for(i=0;i<321;i++) lengths[i]=FirstCodeLengths[mode-1][i];
        TRY(canonical(&trees[0],321));
        for(i=0;i<321;i++) lengths[i]=SecondCodeLengths[mode-1][i];
        TRY(canonical(&trees[1],321));
        for(i=0;i<OffsetCodeSize[mode-1];i++) lengths[i]=OffsetCodeLengths[mode-1][i];
        TRY(canonical(&trees[2],OffsetCodeSize[mode-1]));
    }
    while(!error) {
        v=symbol(&trees[current]);
        if(v==320) break;
        if(v<256) { TRY(emit(v)); current=0; }
        else {
            current=1;
            if(v<318) n=v-256+3;
            else n=65+getbits(v==318?10:15);
            offbits=symbol(&trees[2]);
            REQUIRE(offbits<=16,"Invalid StuffIt distance code.");
            distance=offbits<2?offbits+1:(1UL<<(offbits-1))+getbits(offbits-1)+1;
            TRY(match(distance,n));
        }
    }
    REQUIRE(!error && produced==wanted,"StuffIt output size mismatch.");
    return flush_output();
}

#include "arsenic.h"
