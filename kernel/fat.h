#ifndef FAT_H
#define FAT_H
#include <stdint.h>
#include <stddef.h>
// In-memory FAT12 image embedded via ld -b binary
void fat_init(void* img, size_t sz);
int  fat_open(const char* path);
long fat_read(int fd, void* buf, size_t len);
int  fat_close(int fd);
void fat_ls(void);
#endif
