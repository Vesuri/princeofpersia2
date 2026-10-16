#include "install_config.h"
#define _POSIX_C_SOURCE 200809L
#include "io.h"
#include <stdio.h>
#include <stdlib.h>
void *io_alloc(uint32_t n) { return malloc(n); }
void io_free(void *p) { free(p); }
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
static volatile sig_atomic_t cancelled;
static void cancel(int sig) { (void)sig; cancelled=1; }
int io_cancelled(void) { return cancelled; }
int io_open(File *f,const char *p,int w) {
    long size;
    FILE *h=fopen(p,w?"wb":"rb");
    if(!h) return 0;
    f->handle=h; f->pos=f->size=0;
    if(!w) {
        if(fseek(h,0,SEEK_END) || (size=ftell(h))<0 || size>0x7fffffffL || fseek(h,0,SEEK_SET)) {
            io_close(f); return 0;
        }
        f->size=(uint32_t)size;
    }
    return 1;
}
int io_read(File *f,void *b,uint32_t n) {
    if(fread(b,1,n,f->handle)!=n) return 0;
    f->pos+=n; return 1;
}
int io_write(File *f,const void *b,uint32_t n) {
    if(fwrite(b,1,n,f->handle)!=n) return 0;
    f->pos+=n; if(f->pos>f->size) f->size=f->pos; return 1;
}
int io_seek(File *f,uint32_t o) {
    if(fseek(f->handle,(long)o,SEEK_SET)) return 0;
    f->pos=o; return 1;
}
int io_close(File *f) {
    int ok=1; if(f->handle) ok=fclose(f->handle)==0;
    f->handle=NULL; return ok;
}
int io_exists(const char *p) { struct stat s; return lstat(p,&s)==0; }
int io_mkdir(const char *p) { return mkdir(p,0700)==0; }
int io_remove(const char *p) { return remove(p)==0; }
int io_rename(const char *a,const char *b) { return rename(a,b)==0; }
void io_message(const char *s) { fputs(s,stdout); fputc('\n',stdout); fflush(stdout); }
int main(int argc,char **argv) {
    signal(SIGINT,cancel); signal(SIGTERM,cancel);
    if(argc!=3 && argc!=4) { io_message(INSTALL_USAGE); return 20; }
    return install_data(argv[1],argv[2],argc==4?argv[3]:argv[2]);
}
