#ifndef MODULE_H
#define MODULE_H
#define MAX_MODULES 16
typedef void (*module_init_fn)(void);
void module_register(const char* name, module_init_fn fn);
void modules_init(void);
void modules_list(void);
#define MODULE(name, fn) \
    static void __mod_##fn(void) __attribute__((constructor)); \
    static void __mod_##fn(void){ module_register(name, fn); }
#endif
