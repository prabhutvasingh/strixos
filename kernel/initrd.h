#ifndef INITRD_H
#define INITRD_H
#include <stdint.h>
#include <stddef.h>
#define INITRD_MAX_FILES 32
struct initrd_entry {
    char name[32];
    uint8_t* data;
    size_t size;
};
extern struct initrd_entry initrd_files[];
extern int initrd_count;
void initrd_init(void);
int initrd_save(const char* name, const uint8_t* data, size_t size);
int initrd_remove(const char* name);
#endif
