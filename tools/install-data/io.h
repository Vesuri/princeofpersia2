#ifndef VETTE_INSTALL_IO_H
#define VETTE_INSTALL_IO_H
#include <stddef.h>
#ifdef INSTALL_AMIGA
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
void *memcpy(void *,const void *,size_t);
void *memset(void *,int,size_t);
int memcmp(const void *,const void *,size_t);
size_t strlen(const char *);
#else
#include <stdint.h>
#endif
typedef struct { void *handle; uint32_t size, pos; } File;
int io_open(File *f, const char *path, int writing);
int io_read(File *f, void *buf, uint32_t n);
int io_write(File *f, const void *buf, uint32_t n);
int io_seek(File *f, uint32_t off);
int io_close(File *f);
int io_exists(const char *path);
int io_mkdir(const char *path);
int io_remove(const char *path);
int io_rename(const char *from, const char *to);
void *io_alloc(uint32_t n);
void io_free(void *p);
int io_cancelled(void);
void io_message(const char *s);
int install_data(const char *source, const char *dest, const char *temp);
#endif
