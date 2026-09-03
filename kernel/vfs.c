#include "vfs.h"
#include "initrd.h"
#include "heap.h"
#include "io.h"

extern void vga_puts(const char* s);
extern void serial_puts(const char* s);

static struct vfs_file fds[VFS_MAX_FD];

void vfs_init(void){
    for(int i=0;i<VFS_MAX_FD;i++) fds[i].used=0;
    // reserve 0,1,2 as stdin/out/err
    fds[0].used=1; fds[0].flags=0;
    fds[1].used=1; fds[1].flags=1;
    fds[2].used=1; fds[2].flags=1;
    initrd_init();
    vga_puts("[VFS] initrd 5 files ready\n");
    serial_puts("[VFS] ready\n");
}

static int kstrcmp(const char* a, const char* b){
    while(*a && *a==*b){ a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}
static void kstrncpy(char* d, const char* s, size_t n){
    size_t i=0; for(;i<n && s[i];i++) d[i]=s[i];
    for(;i<n;i++) d[i]=0;
}
int vfs_open(const char* path, int flags){
    if(path[0]=='/') path++;
    for(int i=0; initrd_files[i].name[0];i++){
        if(0==kstrcmp(path, initrd_files[i].name)){
            for(int fd=3; fd<VFS_MAX_FD; fd++) if(!fds[fd].used){
                fds[fd].used=1;
                kstrncpy(fds[fd].name, path, VFS_NAME_LEN-1);
                fds[fd].name[VFS_NAME_LEN-1]=0;
                fds[fd].data = initrd_files[i].data;
                fds[fd].size = initrd_files[i].size;
                fds[fd].offset=0;
                fds[fd].flags=flags;
                return fd;
            }
            return -1;
        }
    }
    return -1;
}
int vfs_close(int fd){
    if(fd<0||fd>=VFS_MAX_FD||!fds[fd].used) return -1;
    if(fd<3) return -1;
    fds[fd].used=0;
    return 0;
}
long vfs_read(int fd, void* buf, size_t len){
    if(fd<0||fd>=VFS_MAX_FD||!fds[fd].used) return -1;
    if(fd==0) return 0;
    struct vfs_file* f=&fds[fd];
    if(f->offset >= f->size) return 0;
    size_t to_read = len;
    if(f->offset + to_read > f->size) to_read = f->size - f->offset;
    for(size_t i=0;i<to_read;i++) ((uint8_t*)buf)[i]= f->data[f->offset+i];
    f->offset += to_read;
    return to_read;
}
long vfs_write(int fd, const void* buf, size_t len){
    (void)buf;
    if(fd==1||fd==2){
        return len;
    }
    if(fd<0||fd>=VFS_MAX_FD||!fds[fd].used) return -1;
    return -1;
}
int vfs_readdir(int idx, char* out){
    if(idx<0||idx>=initrd_count) return -1;
    kstrncpy(out, initrd_files[idx].name, VFS_NAME_LEN);
    return 0;
}
void vfs_dump(void){
    vga_puts("[VFS] files: ");
    for(int i=0;i<initrd_count;i++){ vga_puts(initrd_files[i].name); vga_puts(" "); }
    vga_puts("\n");
}
int vfs_save(const char* path, const void* data, size_t len){
    return initrd_save(path, (const uint8_t*)data, len);
}
int vfs_remove(const char* path){
    return initrd_remove(path);
}
