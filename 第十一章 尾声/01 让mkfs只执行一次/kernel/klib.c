#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "string.h"
#include "protect.h"
#include "fs.h"
#include "stdio.h"
#include "process.h"
#include "hd.h"
#include "keyboard.h"
#include "global.h"
#include "elf.h"
#include "proto.h"

/*****************************************************************************
 *                                get_boot_params
 * 获取loader.asm中保存的内核相关信息，在0x900地址处
 *****************************************************************************/
/**
 * <Ring 0~1> The boot parameters have been saved by LOADER.
 *            We just read them out.
 *
 * @param pbp  Ptr to the boot params structure
 *****************************************************************************/
PUBLIC void get_boot_params(struct boot_params *pbp)
{
	/**
	 * Boot params should have been saved at BOOT_PARAM_ADDR.
	 */
	int *p = (int*)BOOT_PARAM_ADDR;
	assert(p[BI_MAG] == BOOT_PARAM_MAGIC);

	pbp->mem_size = p[BI_MEM_SIZE];
	pbp->kernel_file = (unsigned char *)(p[BI_KERNEL_FILE]);
}

/*****************************************************************************
 *                                get_kernel_map
 * 获取内核镜像的基址和范围
 *****************************************************************************/
/**
 * <Ring 0~1> Parse the kernel file, get the memory range of the kernel image.
 *
 * - The meaning of `base':	base => first_valid_byte
 * - The meaning of `limit':	base + limit => last_valid_byte
 *
 * @param b   Memory base of kernel.
 * @param l   Memory limit of kernel.
 *****************************************************************************/
PUBLIC int get_kernel_map(unsigned int * b, unsigned int * l)
{
	struct boot_params bp;
	get_boot_params(&bp);

	Elf32_Ehdr* elf_header = (Elf32_Ehdr*)(bp.kernel_file);

	*b = ~0;
	unsigned int t = 0;
	int i;
	for (i = 0; i < elf_header->e_shnum; i++) {
		Elf32_Shdr* section_header =
			(Elf32_Shdr*)(bp.kernel_file +
				      elf_header->e_shoff +
				      i * elf_header->e_shentsize);

		if (section_header->sh_flags & SHF_ALLOC) {//此节区在进程执行过程中占用内存
			int bottom = section_header->sh_addr;
			int top = section_header->sh_addr +
				section_header->sh_size;

			if (*b > bottom)
				*b = bottom;
			if (t < top)
				t = top;
		}
	}
	assert(*b < t);
	*l = t - *b - 1;

	return 0;
}

/**
 * 整形转化为字符串,将32位整数用16进制方式显示出来
 * 数字前面的0不被显示出来
 **/
PUBLIC char *itoa(char *str, int num){
    char *p = str;
    char ch;
    int i;
    int flag = 0;
    *p++ = '0';
    *p++ = 'x';
    if(num == 0){
        *p++ = '0';
    }else{
        for(i = 28; i >= 0; i -= 4){
            ch = (num >> i) & 0xF;
            if(flag || (ch > 0)){
                flag = 1;   //遇到第一个非0位
                ch += '0';
                if(ch > '9'){
                    ch += 7;
                }
                *p++ = ch;
            }
        }
    }
    *p = 0;
    return str;
}

/**
 * disp_int
 **/
PUBLIC void disp_int(int input){
    char output[16];
    itoa(output, input);
    disp_str(output);
}

/**
 * 延迟函数
 **/
PUBLIC void delay(int time){
    int i, j, k;
    for(k = 0; k < time; k++){
        for(i = 0; i < 10; i++){
            for(j = 0; j < 10000; j++){
            }
        }
    }
}

/**
 * 清屏操作
 **/
PUBLIC void clearDisplay(){
    disp_pos = 0;
    for(int i = 0; i < 80 * 25; i++){
        disp_str(" ");
    }
    disp_pos = 0;
}
