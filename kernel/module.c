#include "module.h"
#include "heap.h"
extern void vga_puts(const char* s);
extern void serial_puts(const char* s);

static struct { const char* name; module_init_fn fn; } mods[MAX_MODULES];
static int mod_cnt=0;

void module_register(const char* name, module_init_fn fn){
    if(mod_cnt>=MAX_MODULES) return;
    mods[mod_cnt].name=name;
    mods[mod_cnt].fn=fn;
    mod_cnt++;
}
void modules_init(void){
    vga_puts("[MOD] init "); serial_puts("[MOD] init ");
    for(int i=0;i<mod_cnt;i++){
        vga_puts(mods[i].name); vga_puts(" ");
        serial_puts(mods[i].name); serial_puts(" ");
        mods[i].fn();
    }
    vga_puts("\n"); serial_puts("\n");
}
void modules_list(void){
    for(int i=0;i<mod_cnt;i++){ vga_puts("  mod: "); vga_puts(mods[i].name); vga_puts("\n"); }
}
