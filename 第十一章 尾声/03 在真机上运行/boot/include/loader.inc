;=====================================一些常量设置====================
MessageLength equ 9     ;显示字符串的固定长度
BaseOfLoader equ 09000h     ;Loader.bin被加载的段地址
OffsetOfLoader equ 0100h    ;Loader.bin被加载的偏移地址
BaseOfLoaderPhyAddr equ BaseOfLoader * 10h      ;Loader.bin被加载的物理地址
BaseOfKernelFile equ 0x7000     ;Kernel.bin被加载的段地址
OffsetOfKernelFile equ 0    ;Kernel.bin被加载的偏移地址
BaseOfKernelFilePhyAddr equ BaseOfKernelFile * 10h + OffsetOfKernelFile
KernelEntryPointPhyAddr equ 0x1400 ;内核入口地址，必须与ld命令的-Ttext选项保持一致
KRNL_ENT_PT_PHY_ADDR   equ 0x1400

;;Macros below should corresponding with C source.
;内核信息相关
BOOT_PARAM_ADDR		equ	0x900
BOOT_PARAM_MAGIC	equ	0xB007

;根设备起始扇区号
ROOT_BASE equ 0x1800
;=====================================================================

;==============单独为硬盘引导设置的一些常量========================
;; corresponding with include/fs.h
SB_MAGIC_V1		equ	0x111
SB_MAGIC		equ	4 *  0
SB_NR_INODES		equ	4 *  1
SB_NR_SECTS		equ	4 *  2
SB_NR_IMAP_SECTS	equ	4 *  3
SB_NR_SMAP_SECTS	equ	4 *  4
SB_N_1ST_SECT		equ	4 *  5
SB_NR_INODE_SECTS	equ	4 *  6
SB_ROOT_INODE		equ	4 *  7
SB_INODE_SIZE		equ	4 *  8
SB_INODE_ISIZE_OFF	equ	4 *  9
SB_INODE_START_OFF	equ	4 * 10
SB_DIR_ENT_SIZE		equ	4 * 11
SB_DIR_ENT_INODE_OFF	equ	4 * 12
SB_DIR_ENT_FNAME_OFF	equ	4 * 13

;; super block will be stored at: [0x700,0x900)
SUPER_BLK_SEG		equ	0x70

LOADER_SEG		equ	0x9000
LOADER_OFF		equ	0x100
LOADER_PHY_ADDR		equ	LOADER_SEG * 0x10

;; where kernel file is loaded
KERNEL_FILE_SEG		equ	0x7000
KERNEL_FILE_OFF		equ	0
KERNEL_FILE_PHY_ADDR	equ	KERNEL_FILE_SEG * 0x10
