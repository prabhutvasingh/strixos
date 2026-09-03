#include "initrd.h"
#include "heap.h"

static uint8_t readme_data[] = "MyOS initrd README\nVersion 0.1 - Unix-like teaching kernel\nFiles: README, hello.txt, test.bin, file1.txt, file2.txt\n";
static uint8_t hello_data[] = "Hello from initrd! This is a test file via VFS.\nSecond line here.\n";
static uint8_t test_data[] = "0123456789ABCDEF - test.bin content for read testing.\n";
static uint8_t file1_data[] = "content of file1\n";
static uint8_t file2_data[] = "content of file2\n";

struct initrd_entry initrd_files[INITRD_MAX_FILES] = {
    {"README",    readme_data, sizeof(readme_data)-1},
    {"hello.txt", hello_data,  sizeof(hello_data)-1},
    {"test.bin",  test_data,   sizeof(test_data)-1},
    {"file1.txt", file1_data,  sizeof(file1_data)-1},
    {"file2.txt", file2_data,  sizeof(file2_data)-1},
    {"",0,0}
};
int initrd_count = 5;

void initrd_init(void){}

static int kstrcmp_i(const char* a,const char* b){ while(*a&&*a==*b){a++;b++;} return (unsigned char)*a-(unsigned char)*b; }
static void kstrcpy(char* d,const char* s){ size_t i=0; while(s[i]&&i<31){d[i]=s[i];i++;} d[i]=0; }

int initrd_save(const char* name, const uint8_t* data, size_t size){
    if(!name||!name[0]) return -1;
    if(name[0]=='/') name++;
    // find existing
    for(int i=0;i<initrd_count;i++){
        if(0==kstrcmp_i(initrd_files[i].name, name)){
            uint8_t* copy = kmalloc(size?size:1);
            if(!copy) return -1;
            for(size_t k=0;k<size;k++) copy[k]=data[k];
            // free old if it was heap allocated and not static
            // we can't know, but if old data != original static buffers we free
            // simple: if old data outside static area, free. For now leak to avoid freeing static.
            // Check if old data is one of the 5 static buffers -> don't free
            int is_static=0;
            if(initrd_files[i].data==readme_data||initrd_files[i].data==hello_data||initrd_files[i].data==test_data||initrd_files[i].data==file1_data||initrd_files[i].data==file2_data) is_static=1;
            if(!is_static) kfree(initrd_files[i].data);
            initrd_files[i].data=copy;
            initrd_files[i].size=size;
            return 0;
        }
    }
    if(initrd_count>=INITRD_MAX_FILES-1) return -1;
    uint8_t* copy = kmalloc(size?size:1);
    if(!copy) return -1;
    for(size_t k=0;k<size;k++) copy[k]=data[k];
    kstrcpy(initrd_files[initrd_count].name, name);
    initrd_files[initrd_count].data=copy;
    initrd_files[initrd_count].size=size;
    initrd_count++;
    initrd_files[initrd_count].name[0]=0;
    initrd_files[initrd_count].data=0;
    initrd_files[initrd_count].size=0;
    return 0;
}
int initrd_remove(const char* name){
    if(!name) return -1;
    if(name[0]=='/') name++;
    for(int i=0;i<initrd_count;i++){
        if(0==kstrcmp_i(initrd_files[i].name, name)){
            int is_static=0;
            if(initrd_files[i].data==readme_data||initrd_files[i].data==hello_data||initrd_files[i].data==test_data||initrd_files[i].data==file1_data||initrd_files[i].data==file2_data) is_static=1;
            if(!is_static) kfree(initrd_files[i].data);
            for(int j=i;j<initrd_count-1;j++) initrd_files[j]=initrd_files[j+1];
            initrd_count--;
            initrd_files[initrd_count].name[0]=0;
            initrd_files[initrd_count].data=0;
            initrd_files[initrd_count].size=0;
            return 0;
        }
    }
    return -1;
}
