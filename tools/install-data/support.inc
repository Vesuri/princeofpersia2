/* Installation-time support inputs. Original bytes are read from the verified
 * staged application; authored transformations mirror the host generators. */
#include "support.h"
static File application;
static char application_path[1100];
static unsigned char driver_packed[65536],driver_expanded[65536];
static void put16(unsigned char *p,unsigned v) { p[0]=(unsigned char)(v>>8);p[1]=(unsigned char)v; }
static int hexhash(const char *hex) {
    unsigned j,a,b; const char *digits="0123456789abcdef";
    for(j=0;j<32;j++) {
        for(a=0;a<16 && digits[a]!=hex[j*2];a++) {}
        for(b=0;b<16 && digits[b]!=hex[j*2+1];b++) {}
        REQUIRE(a<16 && b<16,"Invalid installed digest."); output[j]=(unsigned char)((a<<4)|b);
    }
    return writebytes(&writing,output,32);
}
static int support_original(unsigned which) {
    unsigned i,j,n,op,ext; uint32_t count,off,at,flags,word,size,state,cipher;
    TRY(join(application_path,sizeof(application_path),publishdir,"Prince of Persia 2.rsrc"));
    REQUIRE(io_open(&application,application_path,0),"Cannot read staged application.");
    if(which==0 || which==2) {
        count=which==0?sizeof(relocation_recipe)/sizeof(relocation_recipe[0]):sizeof(seam_recipe)/sizeof(seam_recipe[0]);
        put32(output,count); TRY(writebytes(&writing,output,4));
        for(i=0;i<count;i++) {
            n=which==0?relocation_recipe[i].length:seam_recipe[i].length;
            off=which==0?relocation_recipe[i].file_offset:seam_recipe[i].file_offset;
            memset(output,0,32); put16(output,which==0?relocation_recipe[i].segment:seam_recipe[i].segment);put16(output+2,n);
            put32(output+4,which==0?relocation_recipe[i].code_offset:seam_recipe[i].code_offset);
            TRY(readat(&application,off,output+8,n));memcpy(output+20,output+8,n);
            if(which==2) memcpy(output+20,seam_recipe[i].replacement,n);
            else {
                op=be16(output+20);
                if(relocation_recipe[i].mode==3) {
                    REQUIRE(op==0x2140 && n==4 && be16(output+22)==0x20,"Invalid indirect store relocation.");
                    put16(output+20,0xaff9);put16(output+22,0x4e71);
                } else if(relocation_recipe[i].mode==1) {
                    REQUIRE((op&0xf1ff)==0x207c && n==6,"Invalid pointer relocation.");
                    at=be32(output+22); REQUIRE(at<0x2000,"Invalid low memory pointer.");
                    put16(output+20,0x41f5|(op&0xe00));put16(output+22,0x0160);put16(output+24,0x2000+at);
                } else if(relocation_recipe[i].mode==2) {
                    REQUIRE(op==0x203c && n==6,"Invalid relocated address cell.");
                    at=be32(output+22); REQUIRE(at==0x308 || at==0x356,"Unknown address cell.");
                    put16(output+20,0x202d);put16(output+22,at==0x308?0x3000:0x3004);put16(output+24,0x4e71);
                } else {
                    if((op&0x3f)==0x38) {
                        put16(output+20,(op&~0x3f)|0x2d);j=op&0xff00;
                        ext=(j==0x0800 || j==0x0c00 || j==0x0000 || j==0x0200 || j==0x0400 || j==0x0600 || j==0x0a00)?2:1;
                    } else {
                        REQUIRE((op>>12)>=1 && (op>>12)<=3 && (op&0xfc0)==0x1c0,"Unknown absolute relocation.");
                        put16(output+20,(op&~0xfc0)|0xb40);ext=n/2-1;
                    }
                    REQUIRE(ext*2+2<=n && be16(output+20+ext*2)<0x2000,"Invalid absolute extent.");
                    put16(output+20+ext*2,be16(output+20+ext*2)+0x2000);
                }
            }
            TRY(writebytes(&writing,output,32));
        }
    } else if(which==1) {
        memcpy(output,"PFC1",4);put32(output+4,32);TRY(writebytes(&writing,output,8));
        for(i=0;i<96;i+=3) {
            n=(unsigned)strlen(originals[i].name);put16(output,n);TRY(writebytes(&writing,output,2));
            TRY(writebytes(&writing,originals[i].name,n));
            for(j=0;j<2;j++) {
                const Original *o=&originals[i+(j?2:0)];put32(output,o->size);TRY(writebytes(&writing,output,4));TRY(hexhash(o->hash));
            }
            TRY(hexhash(originals[i+1].hash));
        }
    } else if(which==3) {
        REQUIRE(DRIVER_SIZE<=sizeof(driver_packed),"Driver input exceeds workspace.");
        TRY(readat(&application,DRIVER_OFFSET,driver_packed,DRIVER_SIZE));state=0xdce5;
        for(i=7;i<DRIVER_SIZE;i++) { cipher=driver_packed[i];driver_packed[i]=(unsigned char)(cipher^(state>>8));state=((cipher+state)*0xce6d+0x58bf)&65535; }
        size=be32(driver_packed); REQUIRE(size<=sizeof(driver_expanded) && size>=0x7456,"Driver output exceeds workspace.");
        i=4;at=0;
        while(i<DRIVER_SIZE && at<size) {
            flags=driver_packed[i++];
            for(j=0;j<8 && i<DRIVER_SIZE && at<size;j++) {
                if(flags&(1U<<j)) driver_expanded[at++]=driver_packed[i++];
                else {
                    REQUIRE(i+2<=DRIVER_SIZE,"Truncated driver back reference.");word=be16(driver_packed+i);i+=2;
                    off=4096-(word&4095);n=(word>>12)+3;
                    REQUIRE(off<=at && n<=size-at,"Invalid driver back reference.");
                    while(n--) { driver_expanded[at]=driver_expanded[at-off];at++; }
                }
            }
        }
        REQUIRE(i==DRIVER_SIZE && at==size,"Invalid driver stream extent.");
        TRY(writebytes(&writing,driver_expanded+0x7256,512));
    } else return fail("Unknown installation support file.");
    return closefile(&application);
}
