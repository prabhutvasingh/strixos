#ifndef ELF_H
#define ELF_H
#include <stdint.h>
#define ELF_MAGIC 0x464C457F
struct elf_hdr { uint32_t magic; uint8_t c[12]; uint16_t type, machine; uint32_t ver; uint64_t entry, phoff, shoff; uint32_t flags; uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx; };
struct elf_phdr { uint32_t type; uint32_t flags; uint64_t off, vaddr, paddr, filesz, memsz, align; };
int elf_check(void* data);
int elf_load(void* data, void (**entry)(void));
#endif
