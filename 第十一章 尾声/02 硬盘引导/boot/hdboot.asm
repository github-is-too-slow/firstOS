; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               hdboot.asm
;从硬盘的引导扇区加载loader，并将控制权交给loader
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
org  0x7c00			; bios always loads boot sector to 0000:7C00
                    ;即：当boot代码开始执行时，cs=0,eip=7c00

	jmp	boot_start

%include		"loader.inc"

STACK_BASE		equ	0x7C00	; base address of stack when booting
TRANS_SECT_NR		equ	2
SECT_BUF_SIZE		equ	TRANS_SECT_NR * 512

;读取硬盘所需要的数据结构
disk_address_packet:	db	0x10		; [ 0] Packet size in bytes.数据结构的大小
			db	0		; [ 1] Reserved, must be 0.
			db	TRANS_SECT_NR	; [ 2] Nr of blocks to transfer.每次传输的扇区数
			db	0		; [ 3] Reserved, must be 0.
			dw	0		; [ 4] Addr of transfer - Offset 段内偏移
			dw	SUPER_BLK_SEG	; [ 6] buffer.          - Seg 段地址
			dd	0		; [ 8] LBA. Low  32-bits. 逻辑块地址
			dd	0		; [12] LBA. High 32-bits.


err:
	mov	dh, 3			; "Error 0  "
	call	disp_str		; display the string
	jmp	$

boot_start:
	mov	ax, cs  ;初始cs=0
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	sp, STACK_BASE ;栈顶指针

	call	clear_screen   ;清屏

	mov	dh, 0			; "Booting  "，开始引导
	call	disp_str		; display the string

	;; read the super block to SUPER_BLK_SEG::0
	mov	dword [disk_address_packet +  8], ROOT_BASE + 1
	call	read_sector
	mov	ax, SUPER_BLK_SEG
	mov	fs, ax   ;以后fs指向超级块缓冲区
    ;以后再读取硬盘都存入到loader缓冲区中
	mov	dword [disk_address_packet +  4], LOADER_OFF
	mov	dword [disk_address_packet +  6], LOADER_SEG

	;; get the sector nr of `/' (ROOT_INODE), it'll be stored in eax
    ;根inode号
	mov	eax, [fs:SB_ROOT_INODE]
	call	get_inode
    ;目前eax中存放根目录的起始扇区号，ecx存放根目录大小(字节单位)
	;; read `/' into ex:bx
	mov	dword [disk_address_packet +  8], eax
	call	read_sector
    ;目前loader缓冲区中存放根目录的前两个扇区，es:bx执行该缓冲区
	;; let's search `/' for the loader
	mov	si, LoaderFileName
	push	bx		; <- save，以后bx每次指向一个根目录项的起始位置
.str_cmp:
	;; before comparation:
	;;     es:bx -> dir_entry @ disk
	;;     ds:si -> filename we want
    ;令bx指向该目录项的文件名
	add	bx, [fs:SB_DIR_ENT_FNAME_OFF]
.1:
	lodsb				; ds:si -> al
	cmp	al, byte [es:bx]
	jz	.2   ;字符相同
	jmp	.different		; oops 字符不相同
.2:					; so far so good
	cmp	al, 0			; both arrive at a '\0', match成功匹配到末尾了
	jz	.found
	inc	bx			; next char @ disk
	jmp	.1			; on and on
.different:
	pop	bx		; -> restore
	add	bx, [fs:SB_DIR_ENT_SIZE]  ;指向下一个目录项
	sub	ecx, [fs:SB_DIR_ENT_SIZE]  ;判断根目录是否循环完毕
	jz	.not_found

	mov	dx, SECT_BUF_SIZE  ;判断读取的两个扇区是否循环完毕
	cmp	bx, dx
	jge	.not_found
    ;初始化下一个目录项的比较
	push	bx
	mov	si, LoaderFileName
	jmp	.str_cmp
.not_found:
	mov	dh, 2   ;No LOADER
	call	disp_str
	jmp	$
.found:  ;目前已经找到了loader文件的目录项，bx指向它
	pop	bx
	add	bx, [fs:SB_DIR_ENT_INODE_OFF]
	mov	eax, [es:bx]		; eax <- inode nr of loader，loader对应的inode号
	call	get_inode		; eax <- start sector nr of loader
    ;目前eax中存放loader文件的起始扇区号，ecx存放大小
	mov	dword [disk_address_packet +  8], eax
load_loader:
	call	read_sector
	cmp	ecx, SECT_BUF_SIZE  ;判断剩余字节是否小于已读取的两个扇区
	jl	.done
	sub	ecx, SECT_BUF_SIZE	; bytes_left -= SECT_BUF_SIZE
	add	word  [disk_address_packet + 4], SECT_BUF_SIZE ; transfer buffer，缓冲区位置移动两个扇区的大小
	jc	err  ;此时如果loader文件大小超过了64KB，报错
	add	dword [disk_address_packet + 8], TRANS_SECT_NR ; LBA，逻辑块地址加2
	jmp	load_loader
.done:
	mov	dh, 1
	call	disp_str   ;HD Booted,加载完毕
	jmp	LOADER_SEG:LOADER_OFF   ;将控制权交给loader
	jmp	$


;============================================================================
;字符串
;----------------------------------------------------------------------------
LoaderFileName		db	"hdldr.bin", 0	; LOADER 之文件名
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength		equ	9
BootMessage:		db	"Booting  "; 9字节, 不够则用空格补齐. 序号 0
Message1		db	"HD Booted"; 9字节, 不够则用空格补齐. 序号 1
Message2		db	"No LOADER"; 9字节, 不够则用空格补齐. 序号 2
Message3		db	"Error 0  "; 9字节, 不够则用空格补齐. 序号 3
;============================================================================

clear_screen:
	mov	ax, 0x600		; AH = 6,  AL = 0代表清除
	mov	bx, 0x700		; 背景黑底前景白字(BH = 0x7)
	mov	cx, 0			; 左上角: (0, 0)
	mov	dx, 0x184f		; 右下角: (24, 79)
	int	0x10			; int 0x10
	ret

;----------------------------------------------------------------------------
; 函数名: disp_str
;----------------------------------------------------------------------------
; 作用:
;	显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based)
disp_str:
	mov	ax, MessageLength
	mul	dh
	add	ax, BootMessage
	mov	bp, ax			; `.
	mov	ax, ds			;  | ES:BP = 串地址
	mov	es, ax			; /
	mov	cx, MessageLength	; CX = 串长度
	mov	ax, 0x1301		; AH = 0x13写字符串模式,  AL = 0x1写模式
	mov	bx, 0x7			; 页号为0(BH = 0) 黑底白字(BL = 0x7)
	mov	dl, 0           ;DH代表行，DL代表列
	int	0x10			; int 0x10
	ret

;----------------------------------------------------------------------------
; read_sector
;----------------------------------------------------------------------------
; Entry:
;     - fields disk_address_packet should have been filled
;       before invoking the routine
; Exit:
;     - es:bx -> data read
; registers changed:
;     - eax, ebx, dl, si, es
read_sector:
	xor	ebx, ebx

	mov	ah, 0x42
	mov	dl, 0x80    ;最高位为1代表读取硬盘，其余代表读取第一块硬盘
	mov	si, disk_address_packet  ;ds:si指向包地址
	int	0x13

	mov	ax, [disk_address_packet + 6]
	mov	es, ax
	mov	bx, [disk_address_packet + 4]

	ret

;----------------------------------------------------------------------------
; get_inode
;----------------------------------------------------------------------------
; Entry:
;     - eax    : inode nr.
; Exit:
;     - eax    : sector nr.
;     - ecx    : the_inode.i_size
;     - es:ebx : inodes sector buffer
; registers changed:
;     - eax, ebx, ecx, edx
get_inode:
	dec	eax			; eax <-  inode_nr -1  在inode_arr中的索引下标
	mov	bl, [fs:SB_INODE_SIZE]
	mul	bl			; eax <- (inode_nr - 1) * INODE_SIZE，inode相对于数组的偏移字节
	mov	edx, SECT_BUF_SIZE
	sub	edx, dword [fs:SB_INODE_SIZE]  ;缓冲区中最后一个inode的起始地址
	cmp	eax, edx
	jg	err
	push	eax

	mov	ebx, [fs:SB_NR_IMAP_SECTS]
	mov	edx, [fs:SB_NR_SMAP_SECTS]
	lea	eax, [ebx+edx+ROOT_BASE+2]   ;inode_arr数组的起始扇区号
	mov	dword [disk_address_packet +  8], eax
	call	read_sector
    ;目前loader缓冲区中存放inode数组的前两个扇区，bx目前指向缓冲区
	pop	eax			; [es:ebx+eax] -> the inode

	mov	edx, dword [fs:SB_INODE_ISIZE_OFF] ;文件大小相对于inode的偏移
	add	edx, ebx
	add	edx, eax		; [es:edx] -> the_inode.i_size
	mov	ecx, [es:edx]		; ecx <- the_inode.i_size 取文件大小ecx

	; es:[ebx+eax] -> the_inode.i_start_sect
	add	ax, word [fs:SB_INODE_START_OFF]

	add	bx, ax
	mov	eax, [es:bx]
	add	eax, ROOT_BASE		; eax <- the_inode.i_start_sect，文件起始扇区号eax
	ret


times 	510-($-$$) db 0 ; 填充剩下的空间，使生成的二进制代码恰好为512字节
dw 	0xaa55		; 结束标志