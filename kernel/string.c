#include <stddef.h>
void *memcpy(void *dst, const void *src, size_t n){
    char *d=dst; const char *s=src;
    for(size_t i=0;i<n;i++) d[i]=s[i];
    return dst;
}
void *memset(void *s, int c, size_t n){
    char *p=s;
    for(size_t i=0;i<n;i++) p[i]=c;
    return s;
}
int memcmp(const void *a, const void *b, size_t n){
    const char *p=a,*q=b;
    for(size_t i=0;i<n;i++) if(p[i]!=q[i]) return p[i]-q[i];
    return 0;
}
