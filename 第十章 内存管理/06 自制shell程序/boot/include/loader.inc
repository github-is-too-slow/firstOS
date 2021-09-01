;=====================================一些常量设置====================
MessageLength equ 9     ;显示字符串的固定长度
BaseOfLoader equ 09000h     ;Loader.bin被加载的段地址
OffsetOfLoader equ 0100h    ;Loader.bin被加载的偏移地址
BaseOfLoaderPhyAddr equ BaseOfLoader * 10h      ;Loader.bin被加载的物理地址
BaseOfKernelFile equ 0x7000     ;Kernel.bin被加载的段地址
OffsetOfKernelFile equ 0    ;Kernel.bin被加载的偏移地址
BaseOfKernelFilePhyAddr equ BaseOfKernelFile * 10h + OffsetOfKernelFile
KernelEntryPointPhyAddr equ 0x1400 ;内核入口地址，必须与ld命令的-Ttext选项保持一致

;;Macros below should corresponding with C source.
;内核信息相关
BOOT_PARAM_ADDR		equ	0x900
BOOT_PARAM_MAGIC	equ	0xB007

;根设备起始扇区号
ROOT_BASE equ 0x5EBC
;=====================================================================