// The shared CRT drops main's return value. Keep its constructor protocol,
// then return the actual result to DOS. The shared _start is renamed at build.
extern int main(void);
extern void (*__preinit_array_start[])(void),(*__preinit_array_end[])(void);
extern void (*__init_array_start[])(void),(*__init_array_end[])(void);
extern void (*__fini_array_start[])(void),(*__fini_array_end[])(void);
__attribute__((used,section(".text.unlikely"))) int _start(void) {
    void (**p)(void);
    for(p=__preinit_array_start;p<__preinit_array_end;++p)(*p)();
    for(p=__init_array_start;p<__init_array_end;++p)(*p)();
    int result=main();
    for(p=__fini_array_end;p>__fini_array_start;)(*--p)();
    return result;
}
