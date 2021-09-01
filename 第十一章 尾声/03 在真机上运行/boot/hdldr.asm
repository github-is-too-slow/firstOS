; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               loader.asm
;hdboot会将该文件加载到0x9000:0x100,
;该文件负责将kernel加载到内存，并且切换到保护模式后将控制权交给内核
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


org  0100h

	jmp	LABEL_START		; Start

%include	"loader.inc"
%include	"pm.inc"


TRANS_SECT_NR		equ	2
SECT_BUF_SIZE		equ	TRANS_SECT_NR * 512

disk_address_packet:	db	0x10		; [ 0] Packet size in bytes. Must be 0x10 or greater.
			db	0		; [ 1] Reserved, must be 0.
sect_cnt:		db	TRANS_SECT_NR	; [ 2] Number of blocks to transfer.
			db	0		; [ 3] Reserved, must be 0.
			dw	KERNEL_FILE_OFF	; [ 4] Address of transfer buffer. Offset
			dw	KERNEL_FILE_SEG	; [ 6]                             Seg
lba_addr:		dd	0		; [ 8] Starting LBA address. Low  32-bits.
			dd	0		; [12] Starting LBA address. High 32-bits.


; GDT ------------------------------------------------------------------------------------------------
;                                                段基址            段界限     , 属性
LABEL_GDT:			Descriptor             0,                    0, 0						; 空描述符
LABEL_DESC_FLAT_C:		Descriptor             0,              0fffffh, DA_CR  | DA_32 | DA_LIMIT_4K			; 0 ~ 4G可执行可读代码段
LABEL_DESC_FLAT_RW:		Descriptor             0,              0fffffh, DA_DRW | DA_32 | DA_LIMIT_4K			; 0 ~ 4G可读写数据段
LABEL_DESC_VIDEO:		Descriptor	 0B8000h,               0ffffh, DA_DRW                         | DA_DPL3	; 显存首地址64KB
; GDT ---------------------------------------------------------------------------------------------------

GdtLen		equ	$ - LABEL_GDT
GdtPtr		dw	GdtLen - 1				; 段界限
		dd	LOADER_PHY_ADDR + LABEL_GDT		; 基地址 (让基地址八字节对齐将起到优化速度之效果，目前懒得改)
; The GDT is not a segment itself; instead, it is a data structure in linear address space.
; The base linear address and limit of the GDT must be loaded into the GDTR register. -- IA-32 Software Developer’s Manual, Vol.3A


; GDT 选择子 ----------------------------------------------------------------------------------
SelectorFlatC		equ	LABEL_DESC_FLAT_C	- LABEL_GDT  ;选择子权限为0
SelectorFlatRW		equ	LABEL_DESC_FLAT_RW	- LABEL_GDT
SelectorVideo		equ	LABEL_DESC_VIDEO	- LABEL_GDT + SA_RPL3
; GDT 选择子 ----------------------------------------------------------------------------------


BaseOfStack	equ	0100h

err:
	mov	dh, 5			; "Error 0  "
	call	real_mode_disp_str	; display the string
	jmp	$

;目前cs:LOADER_SEG=0x9000
;eip:LOADER_OFF=0x100
LABEL_START:
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	sp, BaseOfStack

	mov	dh, 2			; "Loading  "开始加载内核
	call	real_mode_disp_str	; 显示字符串

	; 得到内存数
	mov	ebx, 0			; ebx = 后续值, 开始时需为 0
	mov	di, _MemChkBuf		; es:di 指向一个地址范围描述符结构（Address Range Descriptor Structure）
.MemChkLoop:
	mov	eax, 0E820h		; eax = 0000E820h
	mov	ecx, 20			; ecx = 地址范围描述符结构的大小
	mov	edx, 0534D4150h		; edx = 'SMAP'
	int	15h			; int 15h
	jc	.MemChkFail
	add	di, 20
	inc	dword [_dwMCRNumber]	; dwMCRNumber = ARDS 的个数
	cmp	ebx, 0
	jne	.MemChkLoop
	jmp	.MemChkOK
.MemChkFail:
	mov	dword [_dwMCRNumber], 0
.MemChkOK:

	;; get the sector nr of `/' (ROOT_INODE), it'll be stored in eax
    ;目前fs指向超级块的缓冲区
	mov	eax, [fs:SB_ROOT_INODE] ; fs -> super_block (see hdboot.asm)
	call	get_inode
    ;目前eax中存放根目录的起始扇区号，ecx存放根目录大小(字节单位)
	;; read `/' into es:bx
	mov	dword [disk_address_packet +  8], eax
	call	read_sector
    ;目前kernel缓冲区中存放根目录的前两个扇区，es:bx执行该缓冲区
	;; let's search `/' for the kernel
	mov	si, KernelFileName
	push	bx		; <- save
    ;以下是匹配根目录区的文件名，直到找到位置
.str_cmp:
	;; before comparation:
	;;     es:bx -> dir_entry @ disk
	;;     ds:si -> filename we want
	add	bx, [fs:SB_DIR_ENT_FNAME_OFF]
.str_cmp1:
	lodsb				; ds:si -> al
	cmp	al, byte [es:bx]
	jz	.str_cmp2
	jmp	.different		; oops
.str_cmp2:					; so far so good
	cmp	al, 0			; both arrive at a '\0', match
	jz	.found
	inc	bx			; next char @ disk
	jmp	.str_cmp1			; on and on
.different:
	pop	bx		; -> restore
	add	bx, [fs:SB_DIR_ENT_SIZE]
	sub	ecx, [fs:SB_DIR_ENT_SIZE]
	jz	.not_found
    mov	dx, SECT_BUF_SIZE  ;判断读取的两个扇区是否循环完毕
	cmp	bx, dx
	jge	.not_found
    ;初始化下一个目录项的比较
	push	bx
	mov	si, KernelFileName
	jmp	.str_cmp
.not_found:
	mov	dh, 4
	call real_mode_disp_str
	jmp	$
.found:  ;目前已经找到了kernel文件的目录项，bx指向它
	pop	bx
	add	bx, [fs:SB_DIR_ENT_INODE_OFF]
	mov	eax, [es:bx]		; eax <- inode nr of kernel得到inode号
	call	get_inode		; eax <- start sector起始扇区号 nr of kernel和大小
	mov	dword [disk_address_packet +  8], eax
load_kernel:
	call	read_sector
	cmp	ecx, SECT_BUF_SIZE
	jl	.done
	sub	ecx, SECT_BUF_SIZE	; bytes_left -= SECT_BUF_SIZE
	add	word  [disk_address_packet + 4], SECT_BUF_SIZE ; transfer buffer
	jc	.load_kernel1  ;判断是否已经超过了64KB，当超过时，转到.1去改变段基址，而偏移地址刚好为0
	jmp	.load_kernel2
.load_kernel1:
	add	word  [disk_address_packet + 6], 1000h
.load_kernel2:
	add	dword [disk_address_packet + 8], TRANS_SECT_NR ; LBA，转向下下一个扇区读取
	jmp	load_kernel
.done:
	mov	dh, 3
	call	real_mode_disp_str


; 下面准备跳入保护模式 -------------------------------------------

; 加载 GDTR
	lgdt	[GdtPtr]

; 关中断
	cli

; 打开地址线A20
	in	al, 92h
	or	al, 00000010b
	out	92h, al

; 准备切换到保护模式
	mov	eax, cr0
	or	eax, 1
	mov	cr0, eax

; 真正进入保护模式,此时必须使用保护模式的段机制
	jmp	dword SelectorFlatC:(LOADER_PHY_ADDR + LABEL_PM_START)


	jmp	$		; never arrive here

;============================================================================
;变量
;----------------------------------------------------------------------------
wSectorNo		dw	0		; 要读取的扇区号
bOdd			db	0		; 奇数还是偶数
dwKernelSize		dd	0		; KERNEL.BIN 文件大小

;============================================================================
;字符串
;----------------------------------------------------------------------------
KernelFileName		db	"kernel.bin", 0	; KERNEL.BIN 之文件名
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength		equ	9
LoadMessage db '         '  ;9字节，序号为0
Message2    db '         '  ;9字节，序号为1
Message3    db 'Loading  '  ;9字节，序号为2
Message4    db 'HD Loaded'  ;9字节，序号为3
Message5    db 'No Kernel'  ;9字节，序号为4
Message6	db 'Error 0  '  ;9字节，序号为5
;============================================================================

;============================================================================

clear_screen:
	mov	ax, 0x600		; AH = 6,  AL = 0
	mov	bx, 0x700		; 黑底白字(BL = 0x7)
	mov	cx, 0			; 左上角: (0, 0)
	mov	dx, 0x184f		; 右下角: (24, 79)
	int	0x10			; int 0x10
	ret

; 作用:
;	显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based)
real_mode_disp_str:
	mov	ax, MessageLength
	mul	dh
	add	ax, LoadMessage
	mov	bp, ax			; ┓
	mov	ax, ds			; ┣ ES:BP = 串地址
	mov	es, ax			; ┛
	mov	cx, MessageLength	; CX = 串长度
	mov	ax, 0x1301		; AH = 0x13,  AL = 0x1
	mov	bx, 0x7			; 页号为0(BH = 0) 黑底白字(BL = 0x7)
	mov	dl, 0
	int	0x10			; int 0x10
	ret

;----------------------------------------------------------------------------
; read_sector
;----------------------------------------------------------------------------
; before:
;     - fields disk_address_packet should have been filled
;       before invoking the routine
; after:
;     - es:bx -> data read
; registers changed:
;     - eax, ebx, dl, si, es
read_sector:
	xor	ebx, ebx

	;mov	dword [disk_address_packet +  8], eax
    ;LBA高32位置0
	mov	dword [disk_address_packet + 12], 0

	mov	ah, 0x42
	mov	dl, 0x80
	mov	si, disk_address_packet
	int	0x13

	mov	ax, [disk_address_packet + 6]
	mov	es, ax
	mov	bx, [disk_address_packet + 4]

	ret

;----------------------------------------------------------------------------
; get_inode
;----------------------------------------------------------------------------
; before:
;     - eax    : inode nr.
; after:
;     - eax    : sector nr.
;     - ecx    : the_inode.i_size
;     - es:ebx : inodes sector buffer
; registers changed:
;     - eax, ebx, ecx, edx
get_inode:
	dec	eax				; eax <-  inode_nr -1
	mov	bl, [fs:SB_INODE_SIZE]
	mul	bl				; eax <- (inode_nr - 1) * INODE_SIZE
	mov	edx, SECT_BUF_SIZE
	sub	edx, dword [fs:SB_INODE_SIZE]
	cmp	eax, edx
	jg	err
	push	eax

	mov	ebx, [fs:SB_NR_IMAP_SECTS]
	mov	edx, [fs:SB_NR_SMAP_SECTS]
	lea	eax, [ebx+edx+ROOT_BASE+2]
	mov	dword [disk_address_packet +  8], eax
	call	read_sector

	pop	eax				; [es:ebx+eax] -> the inode

	mov	edx, dword [fs:SB_INODE_ISIZE_OFF]
	add	edx, ebx
	add	edx, eax			; [es:edx] -> the_inode.i_size
	mov	ecx, [es:edx]			; ecx <- the_inode.i_size

	add	ax, word [fs:SB_INODE_START_OFF]; es:[ebx+eax] -> the_inode.i_start_sect

	add	bx, ax
	mov	eax, [es:bx]
	add	eax, ROOT_BASE			; eax <- the_inode.i_start_sect
	ret

; 从此以后的代码在保护模式下执行 ----------------------------------------------------
; 32 位代码段. 由实模式跳入 ---------------------------------------------------------
[section .s32]
ALIGN	32
[bits	32]

LABEL_PM_START:
    ;目前cs=SelectorFlatC
	mov	ax, SelectorVideo
	mov	gs, ax  ;显存段
	mov	ax, SelectorFlatRW
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	ss, ax
	mov	esp, TopOfStack
    ;显示内存信息
	call	DispMemInfo
    ;开启页基址，建立的线性地址空间和物理地址空间的对等映射，
    ;因此之后就使用一套页表，不用再重新加载页表了
	call	SetupPaging
    ;重新安排内核位置
	call	InitKernel

	;jmp	$存储一些内核相关的信息，如内核大小，供初始话Init进程所用
	mov	dword [BOOT_PARAM_ADDR], BOOT_PARAM_MAGIC	; BootParam[0] = BootParamMagic;
	mov	eax, [dwMemSize]				;
	mov	[BOOT_PARAM_ADDR + 4], eax			; BootParam[1] = MemSize;
	mov	eax, KERNEL_FILE_SEG
	shl	eax, 4
	add	eax, KERNEL_FILE_OFF
	mov	[BOOT_PARAM_ADDR + 8], eax			; BootParam[2] = KernelFilePhyAddr;

	;***************************************************************
	jmp	SelectorFlatC:KRNL_ENT_PT_PHY_ADDR	; 正式进入内核 *
	;***************************************************************
	; 内存看上去是这样的：
	;              ┃                                    ┃
	;              ┃                 .                  ┃
	;              ┃                 .                  ┃
	;              ┃                 .                  ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■┃
	;              ┃■■■■■■Page  Tables■■■■■■┃
	;              ┃■■■■■(大小由LOADER决定)■■■■┃
	;    00101000h ┃■■■■■■■■■■■■■■■■■■┃ PAGE_TBL_BASE
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■┃
	;    00100000h ┃■■■■Page Directory Table■■■■┃ PAGE_DIR_BASE  <- 1M
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□┃
	;       F0000h ┃□□□□□□□System ROM□□□□□□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□┃
	;       E0000h ┃□□□□Expansion of system ROM □□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□┃
	;       C0000h ┃□□□Reserved for ROM expansion□□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□┃ B8000h ← gs
	;       A0000h ┃□□□Display adapter reserved□□□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□┃
	;       9FC00h ┃□□extended BIOS data area (EBDA)□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■┃
	;       90000h ┃■■■■■■■LOADER.BIN■■■■■■┃ somewhere in LOADER ← esp
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■┃
	;       80000h ┃■■■■■■■KERNEL.BIN■■■■■■┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■┃
	;       30000h ┃■■■■■■■■KERNEL■■■■■■■┃ 30400h ← KERNEL 入口 (KRNL_ENT_PT_PHY_ADDR)
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃                                    ┃
	;        7E00h ┃              F  R  E  E            ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■┃
	;        7C00h ┃■■■■■■BOOT  SECTOR■■■■■■┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃                                    ┃
	;         500h ┃              F  R  E  E            ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□┃
	;         400h ┃□□□□ROM BIOS parameter area □□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇┃
	;           0h ┃◇◇◇◇◇◇Int  Vectors◇◇◇◇◇◇┃
	;              ┗━━━━━━━━━━━━━━━━━━┛ ← cs, ds, es, fs, ss
	;
	;
	;		┏━━━┓		┏━━━┓
	;		┃■■■┃ 我们使用 	┃□□□┃ 不能使用的内存
	;		┗━━━┛		┗━━━┛
	;		┏━━━┓		┏━━━┓
	;		┃      ┃ 未使用空间	┃◇◇◇┃ 可以覆盖的内存
	;		┗━━━┛		┗━━━┛
	;
	; 注：KERNEL 的位置实际上是很灵活的，可以通过同时改变 LOAD.INC 中的 KRNL_ENT_PT_PHY_ADDR 和 MAKEFILE 中参数 -Ttext 的值来改变。
	;     比如，如果把 KRNL_ENT_PT_PHY_ADDR 和 -Ttext 的值都改为 0x400400，则 KERNEL 就会被加载到内存 0x400000(4M) 处，入口在 0x400400。
	;




; ------------------------------------------------------------------------
; 显示 AL 中的数字
; 位置由dwDispPos决定
; ------------------------------------------------------------------------
DispAL:
	push	ecx
	push	edx
	push	edi

	mov	edi, [dwDispPos]

	mov	ah, 0Fh			; 0000b: 黑底    1111b: 白字
	mov	dl, al
	shr	al, 4
	mov	ecx, 2
.DispALbegin:
	and	al, 01111b
	cmp	al, 9
	ja	.DispAL1
	add	al, '0'
	jmp	.DispAL2
.DispAL1:
	sub	al, 0Ah
	add	al, 'A'
.DispAL2:
	mov	[gs:edi], ax
	add	edi, 2

	mov	al, dl
	loop	.DispALbegin
	;add	edi, 2

	mov	[dwDispPos], edi

	pop	edi
	pop	edx
	pop	ecx

	ret
; DispAL 结束-------------------------------------------------------------

; ------------------------------------------------------------------------
; 显示一个整形数
; ------------------------------------------------------------------------
DispInt:
	mov	eax, [esp + 4]
	shr	eax, 24
	call	DispAL

	mov	eax, [esp + 4]
	shr	eax, 16
	call	DispAL

	mov	eax, [esp + 4]
	shr	eax, 8
	call	DispAL

	mov	eax, [esp + 4]
	call	DispAL

	mov	ah, 07h			; 0000b: 黑底    0111b: 灰字
	mov	al, 'h'
	push	edi
	mov	edi, [dwDispPos]
	mov	[gs:edi], ax
	add	edi, 4
	mov	[dwDispPos], edi
	pop	edi

	ret
; DispInt 结束------------------------------------------------------------

; ------------------------------------------------------------------------
; 显示一个字符串
; ------------------------------------------------------------------------
DispStr:
	push	ebp
	mov	ebp, esp
	push	ebx
	push	esi
	push	edi

	mov	esi, [ebp + 8]	; pszInfo
	mov	edi, [dwDispPos]
	mov	ah, 0Fh
.DispStr1:
	lodsb
	test	al, al
	jz	.DispStr2
	cmp	al, 0Ah	; 是回车吗?
	jnz	.DispStr3
	push	eax
	mov	eax, edi
	mov	bl, 160
	div	bl
	and	eax, 0FFh
	inc	eax
	mov	bl, 160
	mul	bl
	mov	edi, eax
	pop	eax
	jmp	.DispStr1
.DispStr3:
	mov	[gs:edi], ax
	add	edi, 2
	jmp	.DispStr1

.DispStr2:
	mov	[dwDispPos], edi

	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret
; DispStr 结束------------------------------------------------------------

; ------------------------------------------------------------------------
; 换行
; ------------------------------------------------------------------------
DispReturn:
	push	szReturn
	call	DispStr			;printf("\n");
	add	esp, 4

	ret
; DispReturn 结束---------------------------------------------------------

; ------------------------------------------------------------------------
; 内存拷贝，仿 memcpy
; ------------------------------------------------------------------------
; void* MemCpy(void* es:pDest, void* ds:pSrc, int iSize);
; ------------------------------------------------------------------------
MemCpy:
	push	ebp
	mov	ebp, esp

	push	esi
	push	edi
	push	ecx

	mov	edi, [ebp + 8]	; Destination
	mov	esi, [ebp + 12]	; Source
	mov	ecx, [ebp + 16]	; Counter
.MemCpy1:
	cmp	ecx, 0		; 判断计数器
	jz	.MemCpy2		; 计数器为零时跳出

	mov	al, [ds:esi]		; ┓
	inc	esi			; ┃
					; ┣ 逐字节移动
	mov	byte [es:edi], al	; ┃
	inc	edi			; ┛

	dec	ecx		; 计数器减一
	jmp	.MemCpy1		; 循环
.MemCpy2:
	mov	eax, [ebp + 8]	; 返回值

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; 函数结束，返回
; MemCpy 结束-------------------------------------------------------------

; 显示内存信息 --------------------------------------------------------------
DispMemInfo:
	push	esi
	push	edi
	push	ecx

	push	szMemChkTitle
	call	DispStr
	add	esp, 4

	mov	esi, MemChkBuf
	mov	ecx, [dwMCRNumber]	;for(int i=0;i<[MCRNumber];i++) // 每次得到一个ARDS(Address Range Descriptor Structure)结构
.DispMemInfoloop:					;{
	mov	edx, 5			;	for(int j=0;j<5;j++)	// 每次得到一个ARDS中的成员，共5个成员
	mov	edi, ARDStruct		;	{			// 依次显示：BaseAddrLow，BaseAddrHigh，LengthLow，LengthHigh，Type
.DispMemInfo1:					;
	push	dword [esi]		;
	call	DispInt			;		DispInt(MemChkBuf[j*4]); // 显示一个成员
	pop	eax			;
	stosd				;		ARDStruct[j*4] = MemChkBuf[j*4];
	add	esi, 4			;
	dec	edx			;
	cmp	edx, 0			;
	jnz	.DispMemInfo1			;	}
	call	DispReturn		;	printf("\n");
	cmp	dword [dwType], 1	;	if(Type == AddressRangeMemory) // AddressRangeMemory : 1, AddressRangeReserved : 2
	jne	.DispMemInfo2			;	{
	mov	eax, [dwBaseAddrLow]	;
	add	eax, [dwLengthLow]	;
	cmp	eax, [dwMemSize]	;		if(BaseAddrLow + LengthLow > MemSize)
	jb	.DispMemInfo2			;
	mov	[dwMemSize], eax	;			MemSize = BaseAddrLow + LengthLow;
.DispMemInfo2:					;	}
	loop	.DispMemInfoloop			;}
					;
	call	DispReturn		;printf("\n");
	push	szRAMSize		;
	call	DispStr			;printf("RAM size:");
	add	esp, 4			;
					;
	push	dword [dwMemSize]	;
	call	DispInt			;DispInt(MemSize);
	add	esp, 4			;

	pop	ecx
	pop	edi
	pop	esi
	ret
; ---------------------------------------------------------------------------

; 启动分页机制 --------------------------------------------------------------
SetupPaging:
	; 根据内存大小计算应初始化多少PDE以及多少页表
	xor	edx, edx
	mov	eax, [dwMemSize]
	mov	ebx, 400000h	; 400000h = 4M = 4096 * 1024, 一个页表对应的内存大小
	div	ebx
	mov	ecx, eax	; 此时 ecx 为页表的个数，也即 PDE 应该的个数
	test	edx, edx
	jz	.no_remainder
	inc	ecx		; 如果余数不为 0 就需增加一个页表
.no_remainder:
	push	ecx		; 暂存页表个数

	; 为简化处理, 所有线性地址对应相等的物理地址. 并且不考虑内存空洞.

	; 首先初始化页目录
	mov	ax, SelectorFlatRW
	mov	es, ax
	mov	edi, PAGE_DIR_BASE	; 此段首地址为 PAGE_DIR_BASE
	xor	eax, eax
	mov	eax, PAGE_TBL_BASE | PG_P  | PG_USU | PG_RWW
.SetupPaging1:
	stosd
	add	eax, 4096		; 为了简化, 所有页表在内存中是连续的.
	loop	.SetupPaging1

	; 再初始化所有页表
	pop	eax			; 页表个数
	mov	ebx, 1024		; 每个页表 1024 个 PTE
	mul	ebx
	mov	ecx, eax		; PTE个数 = 页表个数 * 1024
	mov	edi, PAGE_TBL_BASE	; 此段首地址为 PAGE_TBL_BASE
	xor	eax, eax
	mov	eax, PG_P  | PG_USU | PG_RWW
.SetupPaging2:
	stosd
	add	eax, 4096		; 每一页指向 4K 的空间
	loop	.SetupPaging2

	mov	eax, PAGE_DIR_BASE
	mov	cr3, eax
	mov	eax, cr0
	or	eax, 80000000h
	mov	cr0, eax
	jmp	short .SetupPaging3
.SetupPaging3:
	nop

	ret
; 分页机制启动完毕 ----------------------------------------------------------



; InitKernel ---------------------------------------------------------------------------------
; 将 KERNEL.BIN 的内容经过整理对齐后放到新的位置
; --------------------------------------------------------------------------------------------
InitKernel:	; 遍历每一个 Program Header，根据 Program Header 中的信息来确定把什么放进内存，放到什么位置，以及放多少。
	xor	esi, esi
	mov	cx, word [KERNEL_FILE_PHY_ADDR + 2Ch]	; ┓ ecx <- pELFHdr->e_phnum
	movzx	ecx, cx					; ┛
	mov	esi, [KERNEL_FILE_PHY_ADDR + 1Ch]	; esi <- pELFHdr->e_phoff
	add	esi, KERNEL_FILE_PHY_ADDR		; esi <- OffsetOfKernel + pELFHdr->e_phoff
.InitKernelBegin:
	mov	eax, [esi + 0]
	cmp	eax, 0				; PT_NULL
	jz	.NoAction
	push	dword [esi + 010h]		; size	┓
	mov	eax, [esi + 04h]		;	┃
	add	eax, KERNEL_FILE_PHY_ADDR	;	┣ ::memcpy(	(void*)(pPHdr->p_vaddr),
	push	eax				; src	┃		uchCode + pPHdr->p_offset,
	push	dword [esi + 08h]		; dst	┃		pPHdr->p_filesz;
	call	MemCpy				;	┃
	add	esp, 12				;	┛
.NoAction:
	add	esi, 020h			; esi += pELFHdr->e_phentsize
	dec	ecx
	jnz	.InitKernelBegin

	ret
; InitKernel ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


; SECTION .data1 之开始 -----------------------
[section .data1]
ALIGN	32
LABEL_DATA:
; 实模式下使用这些符号，标号是相对于程序空间的偏移地址
; 字符串
_szMemChkTitle:			db	"BaseAddrL BaseAddrH LengthLow LengthHigh   Type", 0Ah, 0
_szRAMSize:			db	"RAM size: ", 0
_szReturn:			db	0Ah, 0
_dwMCRNumber:			dd	0	; Memory Check Result
_dwDispPos:			dd	(80 * 5 + 0) * 2	; 屏幕第 5 行, 第 0 列
_dwMemSize:			dd	0
_ARDStruct:			; Address Range Descriptor Structure
	_dwBaseAddrLow:		dd	0
	_dwBaseAddrHigh:	dd	0
	_dwLengthLow:		dd	0
	_dwLengthHigh:		dd	0
	_dwType:		dd	0
_MemChkBuf:	times	256	db	0
;
;; 保护模式下使用这些符号，段基址为0
szMemChkTitle		equ	LOADER_PHY_ADDR + _szMemChkTitle
szRAMSize		equ	LOADER_PHY_ADDR + _szRAMSize
szReturn		equ	LOADER_PHY_ADDR + _szReturn
dwMCRNumber		equ	LOADER_PHY_ADDR + _dwMCRNumber
dwDispPos		equ	LOADER_PHY_ADDR + _dwDispPos
dwMemSize		equ	LOADER_PHY_ADDR + _dwMemSize
ARDStruct		equ	LOADER_PHY_ADDR + _ARDStruct
	dwBaseAddrLow	equ	LOADER_PHY_ADDR + _dwBaseAddrLow
	dwBaseAddrHigh	equ	LOADER_PHY_ADDR + _dwBaseAddrHigh
	dwLengthLow	equ	LOADER_PHY_ADDR + _dwLengthLow
	dwLengthHigh	equ	LOADER_PHY_ADDR + _dwLengthHigh
	dwType		equ	LOADER_PHY_ADDR + _dwType
MemChkBuf		equ	LOADER_PHY_ADDR + _MemChkBuf


; 堆栈就在数据段的末尾
StackSpace:	times	1000h	db	0
TopOfStack	equ	LOADER_PHY_ADDR + $	; 栈顶
; SECTION .data1 之结束 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
