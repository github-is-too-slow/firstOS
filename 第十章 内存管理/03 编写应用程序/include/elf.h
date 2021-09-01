#ifndef _ORANGES_ELF_H_
#define _ORANGES_ELF_H_

/*loader.asm相关常量*/
#define BOOT_PARAM_ADDR		0x900  //存放内核相关信息的起始地址
#define BOOT_PARAM_MAGIC 	0xB007
#define BI_MAG				0
#define BI_MEM_SIZE			1
#define BI_KERNEL_FILE      2
#define EI_NIDENT   16
#define SHF_ALLOC 0x2           //此节区在进程执行过程中占用内存

#define Elf32_Addr  u32
#define Elf32_Half u16
#define Elf32_Off  u32
#define Elf32_SWord int
#define Elf32_Word  u32

struct boot_params {
    int mem_size;
    unsigned char *kernel_file;
};

typedef struct
{
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half  e_type;
    Elf32_Half  e_machine;
    Elf32_Word  e_version;
    Elf32_Addr  e_entry;
    Elf32_Off   e_phoff;
    Elf32_Off   e_shoff;
    Elf32_Word  e_flags;
    Elf32_Half  e_ehsize;
    Elf32_Half  e_phentsize;
    Elf32_Half  e_phnum;
    Elf32_Half  e_shentsize;
    Elf32_Half  e_shnum;
    Elf32_Half  e_shstrndx;
}Elf32_Ehdr;

typedef struct {
    Elf32_Word    sh_name;    //name - index into section header string table
    Elf32_Word    sh_type;    /* type */
    Elf32_Word    sh_flags;    /* flags */
    Elf32_Addr    sh_addr;    /* address */
    Elf32_Off     sh_offset;    /* file offset */
    Elf32_Word    sh_size;    /* section size */
    Elf32_Word    sh_link;    /* section header table index link */
    Elf32_Word    sh_info;    /* extra information */
    Elf32_Word    sh_addralign;    /* address alignment */
    Elf32_Word    sh_entsize;    /* section entry size */
} Elf32_Shdr;

PUBLIC void get_boot_params(struct boot_params * pbp);
PUBLIC int get_kernel_map(unsigned int * b, unsigned int * l);
#endif