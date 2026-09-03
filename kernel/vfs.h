#ifndef VFS_H
#define VFS_H
#include <stdint.h>
#include <stddef.h>

#define VFS_MAX_FD 16
#define VFS_NAME_LEN 32

struct vfs_file {
    int used;
    char name[VFS_NAME_LEN];
    uint8_t* data;
    size_t size;
    size_t offset;
    int flags;
};

void vfs_init(void);
int  vfs_open(const char* path, int flags);
int  vfs_close(int fd);
long vfs_read(int fd, void* buf, size_t len);
long vfs_write(int fd, const void* buf, size_t len);
int  vfs_readdir(int idx, char* out);
void vfs_dump(void);
int  vfs_save(const char* path, const void* data, size_t len);
int  vfs_remove(const char* path);

#endif
