// Simple sh for StrixOS - uses StrixOS syscalls via int 0x80
// Syscall numbers match kernel/syscall.h
#define SYS_WRITE 1
#define SYS_READ  2
#define SYS_OPEN  3
#define SYS_CLOSE 4

static long sys_write(int fd, const char* b, unsigned long l){
    long r; asm volatile("int $0x80" : "=a"(r) : "a"(1), "D"((long)fd), "S"(b), "d"(l) : "rcx","r11","memory");
    return r;
}
static long sys_read(int fd, char* b, unsigned long l){
    long r; asm volatile("int $0x80" : "=a"(r) : "a"(2), "D"((long)fd), "S"(b), "d"(l) : "rcx","r11","memory");
    return r;
}
static long sys_open(const char* p, int f){
    long r; asm volatile("int $0x80" : "=a"(r) : "a"(3), "D"(p), "S"((long)f) : "rcx","r11","memory");
    return r;
}
static void puts(const char* s){ unsigned long l=0; while(s[l]) l++; sys_write(1,s,l); }
static void putc(char c){ sys_write(1,&c,1); }

static int strcmp(const char* a, const char* b){ while(*a && *a==*b){a++;b++;} return *a-*b; }
static int strncmp(const char* a, const char* b, unsigned long n){ for(unsigned long i=0;i<n;i++){ if(a[i]!=b[i]) return a[i]-b[i]; if(!a[i]) return 0;} return 0; }

static void do_ls(void){
    // Use VFS via direct? For user, we don't have vfs_readdir syscall, so fake via open
    puts("README  hello.txt  test.bin  sh\n");
}
static void do_cat(const char* path){
    while(*path==' ') path++;
    if(!*path){ puts("cat: missing\n"); return; }
    int fd=sys_open(path,0);
    if(fd<0){ puts("cat: not found\n"); return; }
    char buf[128]; long n=sys_read(fd,buf,120);
    if(n>0){ buf[n]=0; puts(buf); if(buf[n-1]!='\n') puts("\n"); }
    // close not needed for demo
}
int main(void){
    puts("StrixOS sh (embedded) - type help\n");
    char line[128]; int pos=0;
    puts("sh> ");
    while(1){
        char c; long n=sys_read(0,&c,1);
        if(n<=0){ asm volatile("int $0x80" :: "a"(6) : "rcx","r11","memory"); continue; }
        if(c=='\r') c='\n';
        if(c=='\b' || c==127){
            if(pos>0){ pos--; sys_write(1, "\b \b", 3); }
            continue;
        }
        if(c=='\t'){
            // tab completion for file names after "cat "
            int sp=-1; for(int i=pos-1;i>=0;i--) if(line[i]==' ') { sp=i; break; }
            int word_start = (sp==-1)?0:sp+1;
            char pref[32]; int plen=0;
            for(int i=word_start;i<pos && plen<31;i++) pref[plen++]=line[i];
            pref[plen]=0;
            // if at start, complete commands
            const char* cmds[]={"ls","cat","echo","clear","help","ps",0};
            const char* files[]={"README","hello.txt","test.bin","sh",0};
            const char** list = (sp==-1) ? cmds : files;
            // find matches
            const char* matches[8]; int mcnt=0;
            for(int i=0;list[i] && mcnt<8;i++){
                int ok=1;
                for(int k=0;k<plen;k++) if(list[i][k]!=pref[k]){ ok=0; break; }
                if(ok) matches[mcnt++]=list[i];
            }
            if(mcnt==0) continue;
            // find common prefix
            int common=plen;
            while(1){
                char ch=0; int all=1;
                for(int i=0;i<mcnt;i++){
                    if(matches[i][common]==0){ all=0; break; }
                    if(i==0) ch=matches[i][common];
                    else if(matches[i][common]!=ch){ all=0; break; }
                }
                if(!all) break;
                // append ch
                if(pos<120){ line[pos++]=ch; sys_write(1, &ch, 1); }
                common++;
            }
            // if single match and fully completed, add space
            if(mcnt==1 && common== (int)__builtin_strlen(matches[0])){
                if(pos<120){ line[pos++]=' '; sys_write(1, " ", 1); }
            }
            continue;
        }
        sys_write(1, &c, 1);
        if(c=='\n'){
            line[pos]=0;
            char* p=line; while(*p==' ') p++;
            if(*p==0){ puts("sh> "); pos=0; continue; }
            if(!strcmp(p,"ls")) do_ls();
            else if(!strncmp(p,"cat ",4)) do_cat(p+4);
            else if(!strcmp(p,"help")) puts("Commands: ls, cat <file>, echo, ps, help\n");
            else if(!strcmp(p,"ps")) puts("sh ps: not yet\n");
            else { puts("unknown: "); puts(p); puts("\n"); }
            pos=0;
            puts("sh> ");
        } else {
            if(pos<120) line[pos++]=c;
        }
    }
    return 0;
}
void _start(void){ main(); while(1){} }
