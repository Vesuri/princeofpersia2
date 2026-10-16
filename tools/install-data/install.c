/* Standalone Prince of Persia 2 installer. No archive/filesystem libraries.
 * All large workspaces live in BSS; decoding is iterative and bounded.
 * StuffIt format references and table licensing: see README.md.
 */
#include "io.h"
#include "sha256.h"
#include <string.h>
#include "sit13_tables.h"
#define BUFSIZE 4096
#define LIMIT (32UL*1024*1024)
#define REQUIRE(x,msg) do { if(!(x)) return fail(msg); } while(0)
#define TRY(x) do { if(!(x)) return 0; } while(0)
static const char *error;
static unsigned char input[BUFSIZE],output[BUFSIZE],history[65536],scratch[4096];
#include "originals.h"
static File archive,writing,checking;
static char publishdir[1024],target[1100],staged[1100];
static unsigned publish_owned,dest_owned,data_owned;
static unsigned char committed[ORIGINAL_COUNT];
#include "decode.inc"
static SHA256 sha;
static int verify_hash(File *f,uint32_t size,const char *hash) {
    uint32_t off=0,n; unsigned i; unsigned char digest[32]; const char *hex="0123456789abcdef";
    REQUIRE(f->size==size,"Installed file has an unsupported size."); sha_init(&sha);
    while(off<f->size) { n=f->size-off; if(n>BUFSIZE) n=BUFSIZE; TRY(readat(f,off,scratch,n)); sha_update(&sha,scratch,n); off+=n; }
    sha_final(&sha,digest);
    for(i=0;i<32;i++) REQUIRE(hex[digest[i]>>4]==hash[i*2] && hex[digest[i]&15]==hash[i*2+1],"Game data SHA-256 mismatch: damaged or unsupported original version.");
    return 1;
}
static void put32(unsigned char *p,uint32_t v) {
    p[0]=(unsigned char)(v>>24); p[1]=(unsigned char)(v>>16);
    p[2]=(unsigned char)(v>>8); p[3]=(unsigned char)v;
}
static int original_paths(unsigned i,const char *dest) {
    TRY(join(target,sizeof(target),dest,originals[i].name));
    TRY(join(staged,sizeof(staged),publishdir,originals[i].name)); return 1;
}
static int same_name(const char *a,const char *b) { return strlen(a)==strlen(b) && !memcmp(a,b,strlen(a)); }
static int verify_existing(unsigned i) {
    unsigned j;uint32_t checksum;
    REQUIRE(io_open(&checking,target,0),"Cannot read existing game data.");
    if(same_name(originals[i].name,"Data/Prince2.opt")) {
        REQUIRE(checking.size==16,"Existing preferences have an invalid size.");
    } else if(same_name(originals[i].name,"Data/Prince2.opt.finfo")) {
        REQUIRE(checking.size==32,"Existing preferences metadata has an invalid size.");
        TRY(readat(&checking,0,scratch,32));REQUIRE(!memcmp(scratch,"AFI1",4),"Invalid preferences metadata.");
        checksum=2166136261UL;for(j=0;j<28;j++)checksum=(checksum^scratch[j])*16777619UL;
        REQUIRE(be32(scratch+28)==checksum,"Invalid preferences metadata checksum.");
    } else TRY(verify_hash(&checking,originals[i].size,originals[i].hash));
    return closefile(&checking);
}
#include "support.inc"
static int extract_original(unsigned i) {
    const Original *s=&originals[i]; unsigned j; uint32_t checksum;
    REQUIRE(io_open(&writing,staged,1),"Cannot create staged game file.");
    if(s->method==254) { TRY(support_original((unsigned)s->off)); }
    else if(s->method==255) {
        TRY(readat(&archive,s->packed+4,scratch,16));
        memcpy(output,"AFI1",4); memcpy(output+4,scratch,10); memset(output+14,0,6);
        TRY(readat(&archive,s->off+10,output+20,8));
        checksum=2166136261UL;
        for(j=0;j<28;j++) checksum=(checksum^output[j])*16777619UL;
        put32(output+28,checksum);
        REQUIRE(be32(output+28)==checksum,"Metadata encoding failure.");
        TRY(writebytes(&writing,output,32));
    } else {
        REQUIRE(s->size<=LIMIT && span(s->off,s->packed,archive.size),"Invalid original fork extent.");
        begin_input(&archive,s->off,s->packed); begin_output(&writing,s->size);
        if(s->method==15 && s->size) TRY(arsenic());
        else if(s->method==13 && s->size) TRY(sit13());
        else {
            REQUIRE(s->size==s->packed && (!s->size || s->method==0),"Unsupported original fork method.");
            while(produced<wanted && !error) { TRY(emit(getbyte())); }
            TRY(flush_output());
        }
        REQUIRE(!error && (s->method==15 || crc==s->crc),"Original fork CRC mismatch.");
    }
    TRY(closefile(&writing));
    REQUIRE(io_open(&checking,staged,0),"Cannot verify staged game file.");
    TRY(verify_hash(&checking,s->size,s->hash)); return closefile(&checking);
}
static int private_dir(char *path,unsigned capacity,const char *parent,const char *prefix) {
    unsigned i,n=(unsigned)strlen(prefix); char suffix[32];
    REQUIRE(n+3<=sizeof(suffix),"Invalid temporary directory prefix.");
    memcpy(suffix,prefix,n); suffix[n+2]=0;
    for(i=0;i<100;i++) {
        suffix[n]="0123456789abcdef"[i>>4]; suffix[n+1]="0123456789abcdef"[i&15];
        TRY(join(path,capacity,parent,suffix)); if(io_mkdir(path)) return 1;
    }
    return fail("Cannot create private installer directory.");
}
static int perform(const char *source,const char *dest,const char *temp) {
    unsigned i,existing=0; (void)temp;
    if(same_name(source,"--verify-options")) {
        for(i=0;i<ORIGINAL_COUNT;i++) {
            if(!same_name(originals[i].name,"Data/Prince2.opt") &&
               !same_name(originals[i].name,"Data/Prince2.opt.finfo")) continue;
            TRY(join(target,sizeof(target),dest,originals[i].name));
            TRY(verify_existing(i)); existing++;
        }
        REQUIRE(existing==2,"Preferences manifest is incomplete.");
        return 1;
    }
    int verify_only=same_name(source,"--verify");
    if(!verify_only) {
        REQUIRE(io_open(&archive,source,0),"Cannot open source archive.");
        TRY(verify_hash(&archive,7716607UL,"02437dc253dffec4eee516f8e5e0f2ecaccc4bd31f3d6957d42ea4f87b80b080"));
    } else REQUIRE(io_exists(dest),"No existing installation.");
    crc_init();
    if(!io_exists(dest)) { REQUIRE(io_mkdir(dest),"Cannot create destination directory."); dest_owned=1; }
    for(i=0;i<ORIGINAL_COUNT;i++) {
        TRY(join(target,sizeof(target),dest,originals[i].name));
        if(io_exists(target)) { TRY(verify_existing(i)); existing++; }
    }
    if(existing==ORIGINAL_COUNT) return 1;
    REQUIRE(!verify_only,"Installed game data is incomplete.");
    TRY(private_dir(publishdir,sizeof(publishdir),dest,".pop2-publish-")); publish_owned=1;
    TRY(join(staged,sizeof(staged),publishdir,"Data"));
    REQUIRE(io_mkdir(staged),"Cannot create staging data directory.");
    io_message("Extracting and verifying original game files...");
    for(i=0;i<ORIGINAL_COUNT;i++) { TRY(original_paths(i,dest)); TRY(extract_original(i)); }
    TRY(closefile(&archive));
    TRY(join(target,sizeof(target),dest,"Data"));
    if(!io_exists(target)) { REQUIRE(io_mkdir(target),"Cannot create game data directory."); data_owned=1; }
    for(i=0;i<ORIGINAL_COUNT;i++) {
        TRY(original_paths(i,dest));
        if(io_exists(target)) TRY(verify_existing(i));
        else { REQUIRE(io_rename(staged,target),"Cannot install verified original."); committed[i]=1; }
    }
    return 1;
}
int install_data(const char *source,const char *dest,const char *temp) {
    unsigned i; int ok=perform(source,dest,temp);
    io_free(transform); transform=0; io_free(block); block=0;
    io_close(&application); io_close(&archive); io_close(&writing); io_close(&checking);
    if(!ok) {
        for(i=0;i<ORIGINAL_COUNT;i++) if(committed[i]) {
            if(join(target,sizeof(target),dest,originals[i].name)) io_remove(target);
        }
        if(data_owned && join(target,sizeof(target),dest,"Data")) io_remove(target);
    }
    if(publish_owned) {
        for(i=0;i<ORIGINAL_COUNT;i++) if(join(staged,sizeof(staged),publishdir,originals[i].name) && io_exists(staged))
            if(!io_remove(staged)) io_message("Warning: could not remove a staging file.");
        if(join(staged,sizeof(staged),publishdir,"Data") && io_exists(staged)) io_remove(staged);
        if(!io_remove(publishdir)) io_message("Warning: could not remove destination staging directory.");
    }
    if(!ok && dest_owned) io_remove(dest);
    if(!ok) { io_message(error?error:"Installation failed."); return 20; }
    io_message("Original Prince of Persia 2 data installed and verified."); return 0;
}
