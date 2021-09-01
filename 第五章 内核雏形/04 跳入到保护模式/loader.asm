;=====================================================================
;将Kernel加载到内存,并跳转到保护模式,重新安排内核位置,并向内核交出控制权
;对应 5.4节
;=======================================================================
org 0100h       ;被boot.bin引导扇区代码加载到 09000:0100h 处

    jmp LABEL_START           ;start to load

;引入软盘头部信息以及相关常量
%include "fat12hdr.inc"
;引入内存加载位置常量
%include "loader.inc"

;=====================================一些常量设置====================
BaseOfStack equ 0100h
;=====================================================================
;==============一些变量设置:必须设置在磁盘头部信息之后=======================
wRootDirSizeForLoop dw RootDirSectors
wSectorNo  dw 0     ;要读取的扇区号
bOdd db 0   ;扇区号是奇数还是偶数
dwKernelSize dd 0       ;Kernel.bin文件大小
;==================================一些显示字符串===============
KernelFileName db 'KERNEL  BIN', 0     ;KERNEL.BIN文件名,固定11个字符
LoadMessage db '         '  ;9字节，序号为0
Message2    db '         '  ;9字节，序号为1
Message3    db 'Loading  '  ;9字节，序号为2
Message4    db 'Ready.   '  ;9字节，序号为3
Message5    db 'No Kernel'  ;9字节，序号为4
;===========================================================================
;====================一些用于获取内存地址相关变量================
;实模式下使用的符号
_szMemChkTitle:
    db 'BaseAddrL BaseAddrH LengthLow LengthHigh    Type', 0Ah, 0
_dwMCRNumber: dd 0   ;Address Range Descriptor Structure地址范围描述符结构的个数
_MemChkBuf:
    times 256 db 0     ;用于存储地址描述符结构体(20byte)的缓冲区(可以存12个)
_dwMemSize: dd 0
_dwDispPos: dd (80 * 5 + 0) * 2
_szRAMSize: db 'RAM size:', 0
_szReturn: db 0Ah, 0
_ARDStruct:
    _dwBaseAddrLow: dd 0
    _dwBaseAddrHigh: dd 0
    _dwLengthLow: dd 0
    _dwLengthHigh: dd 0
    _dwType: dd 0
_PageTableNumber: dd 0      ;页表个数
;保护模式下使用的符号
szMemChkTitle equ _szMemChkTitle + BaseOfLoaderPhyAddr
dwMCRNumber equ _dwMCRNumber + BaseOfLoaderPhyAddr
MemChkBuf equ _MemChkBuf + BaseOfLoaderPhyAddr
dwMemSize equ _dwMemSize + BaseOfLoaderPhyAddr
dwDispPos equ _dwDispPos + BaseOfLoaderPhyAddr
szRAMSize equ _szRAMSize + BaseOfLoaderPhyAddr
szReturn equ _szReturn + BaseOfLoaderPhyAddr
ARDSTruct equ _ARDStruct + BaseOfLoaderPhyAddr
    dwBaseAddrLow equ _dwBaseAddrLow + BaseOfLoaderPhyAddr
    dwBaseAddrHigh equ _dwBaseAddrHigh + BaseOfLoaderPhyAddr
    dwLengthLow equ _dwLengthLow + BaseOfLoaderPhyAddr
    dwLengthHigh equ _dwLengthHigh + BaseOfLoaderPhyAddr
    dwType equ _dwType + BaseOfLoaderPhyAddr
PageTableNumber equ _PageTableNumber + BaseOfLoaderPhyAddr
;============================================================

LABEL_START:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, BaseOfStack     ;设置栈基址

    mov dh, 2
    call DispStrRealMode    ;"Loading  ", 开始加载内核了

    ;软驱复位
    xor ah, ah
    xor dl, dl
    int 13h
    ;在软盘A的根目录寻找KERNEL.BIN
    mov word [wSectorNo], SectorNoOfRootDir  ;将根目录起始扇区号赋给要读取的扇区号
LABEL_SEARCH_IN_ROOT_DIR_BEGIN:
    cmp word [wRootDirSizeForLoop], 0 ;判断根目录是否读取完毕
    jz LABEL_NO_KERNELBIN   ;读取完毕还是没有找kernel.bin文件
    dec word [wRootDirSizeForLoop]
    mov ax, BaseOfKernelFile
    mov es, ax
    mov bx, OffsetOfKernelFile   ;暂时使用kernel.bin加载的目标地址
    mov ax, [wSectorNo]     ;要读取的根目录占用的某个扇区号
    mov cl, 1
    call ReadSector ;此时指定扇区内容在es:bx中
    mov si, KernelFileName ;ds:si -> "KERNEL  BIN"
    mov di, OffsetOfKernelFile  ;es:di -> BaseOfKernelFile:0
    cld
    mov dx, 10h  ;32字节表示一个根目录项,一个扇区中共有16个
LABEL_SEARCH_FOR_KERNELBIN:
    cmp dx, 0
    jz LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR   ;一个扇区16个根目录项比较完毕
    dec dx
    mov cx, 11  ;比较11个字符是否相等
LABEL_CMP_FILENAME:
    cmp cx, 0
    jz LABEL_FILENAME_FOUND ;比较了11个字符都没有跳转，表明找到了
    dec cx
    lodsb   ;ds:si -> al, si++
    cmp al, byte [es:di]
    jz LABEL_GO_ON
    jmp LABEL_DIFFERETN ;因为文件名刚好是从每个32字节目录项的第一个字节开始，
LABEL_GO_ON:            ;只要发现不一样的字符,可以只比较开头11个文件名字符即可
    inc di
    jmp LABEL_CMP_FILENAME
LABEL_DIFFERETN:
    and di, 0FFE0h  ;di &= E0, 清空后五位，指向根目录项开始
    add di, 20h     ;加上32字节，指向下一个目录项
    mov si, KernelFileName  ;
    jmp LABEL_SEARCH_FOR_KERNELBIN
LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR:
    add word [wSectorNo], 1
    jmp LABEL_SEARCH_IN_ROOT_DIR_BEGIN
LABEL_NO_KERNELBIN:
    mov dh, 4           ;"No Kernel"
    call DispStrRealMode        ;显示字符串
%ifdef _BOOT_DEBUG_
    mov ax, 4c00h
    int 21h         ;调试时返回dos系统
%else
    jmp $       ;没有找到一直在此处循环
%endif
LABEL_FILENAME_FOUND:  ;此时已经找到了kernel.bin所对应的根目录项
    mov ax, RootDirSectors
    and di, 0FFE0h  ;使es:di指向根目录项开始
    push eax
    mov eax, [es:di + 01Ch]
    mov dword [dwKernelSize], eax       ;kernel.bin文件大小
    pop eax
    add di, 01Ah    ;使es:di指向该文件起始簇号的字节
    mov cx, word [es:di]    ;取起始簇号
    push cx
    add cx, ax
    add cx, DeltaSectorNo   ;得到了起始簇号对应的扇区号
    mov ax, BaseOfKernelFile
    mov es, ax
    mov bx, OffsetOfKernelFile      ;es:bx指向存储kernel.bin的起始位置
    mov ax, cx              ;ax扇区号
LABEL_LOADERING_FILE:
    push ax
    push bx
    mov ah, 0Eh
    mov al, '.'
    mov bl, 0Fh
    int 10h         ;每将读取一个kernel.bin的扇区，就打印一个.
    pop bx          ;形成Loading  .......的效果
    pop ax          ;电传打字机输出	AH=0EH	AL=字符，BH=页码，BL=颜色（只适用于图形模式）
    mov cl, 1       ;读取一个扇区
    call ReadSector
    pop ax          ;取出该扇区对应的簇号
    call GetFATEntry    ;返回的ax中存着下一个扇区对应的簇号
    cmp ax, 0FFFh       ;假如下一个簇号=0FFFh，表明文件结束
    jz LABEL_FILE_LOADED
    push ax     ;下一个簇号入栈
    mov dx, RootDirSectors
    add ax, dx
    add ax, DeltaSectorNo   ;得到下一个扇区号
    add bx, [BPB_BytesPerSec]   ;存储位置偏移512字节
    jmp LABEL_LOADERING_FILE
LABEL_FILE_LOADED:
    call KillMotor      ;关闭软驱马达
    mov dh, 3
    call DispStrRealMode        ;"Ready.   " 表示引导已经完毕，kernel.bin已经加载进入了内存

    ;通过int 15h BIOS系统调用得到内存信息
    mov ebx, 0  ;连续值，通常用于是否检测到最后一个内存块
    mov di, _MemChkBuf
.loop_mem:
    mov eax, 0E820h
    mov ecx, 20
    mov edx, 0534D4150h
    int 15h
    jc LABEL_MEM_CHK_FAIL   ;若CF=1,表示检测有错误发生
    add di, 20  ;指向下一个地址结构体
    inc dword [_dwMCRNumber]  ;地址结构体个数加1
    cmp ebx, 0   ;测试是否到最后一个内存块
    jne .loop_mem
    jmp LABEL_MEM_CHK_OK
LABEL_MEM_CHK_FAIL:
    mov dword [_dwMCRNumber], 0
LABEL_MEM_CHK_OK:
    ;下面从实模式跳入保护模式下
    ;加载GDTR
    lgdt [GdtPtr]
    ;关中断
    cli
    ;打开A20地址线
    in al, 92h
    or al, 00000010b
    out 92h, al
    ;切换到保护模式
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    ;跳转到保护模式下32位代码段
    jmp dword SelectorFlatC: (BaseOfLoaderPhyAddr + LABEL_PM_START)

;功能：关闭软驱马达
KillMotor:
    push dx
    mov dx, 03F2h
    mov al, 0
    out dx, al
    pop dx
    ret

;功能：显示一个字符串
;默认：显示的字符串默认9字节，字符串序号存在dh中
DispStrRealMode:
    mov ax, MessageLength
    mul dh
    add ax, LoadMessage
    mov bp, ax
    mov ax, ds
    mov es, ax  ;es:bp起始段地址
    mov cx, MessageLength  ;串长度
    mov ax, 01301h  ;AH=13h,AL=01h
    mov bx, 0007h   ;页号为0(BH=0)，黑底白字高亮(BL=07h)
    mov dl, 0
    int 10h  ;AH=13H，AL= 显示输出方式，BH=页码，BL=颜色，CX=字符串长度，DH=行，DL=列，ES:BP=字符串偏移量
    ret

;功能：读取一定软盘扇区数到指定内存处
;默认：从第ax个Sector扇区开始(索引从0开始)，将cl个Sector读取es:bx中
ReadSector:
    push bp
    mov bp, sp
    sub esp, 2  ;用两个字节栈空间存储要读的扇区数
    mov byte [bp - 2], cl   ;读取扇区数入栈
    push bx
    mov bl, [BPB_SecPerTrk]     ;除数：每个磁道的扇区数
    div bl   ;扇区号/每个磁道的扇区数，商在al,余数在ah中
    inc ah
    mov cl, ah      ;cl中存读取的起始扇区号
    mov dh, al
    shr al, 1
    mov ch, al          ;ch中存柱面号
    and dh, 1           ;dh中存磁头号
    pop bx              ;bx中存着目标偏移地址
    mov dl, [BS_DrvNum]    ;dl中存驱动器号，0表示软盘A
.GoOnReading:
    mov ah, 2       ;int 13h子程序号
    mov al, byte [bp - 2]   ;al中存读取的扇区数
    int 13h
    jc .GoOnReading     ;若读取错误会将CF位置1，不停地读，直至成功
    add esp, 2
    pop bp
    ret

;功能：寻找指定簇号(FAT表索引)在FAT中的表项值
;默认：ax存储着簇号，计算得到的表项值存储在ax中
GetFATEntry:
    push es
    push bx
    push ax     ;此时ax中存储着待查找的簇号
    mov ax, BaseOfKernelFile
    sub ax, 0100h       ;注意这是段地址减去了0100h
    mov es, ax      ;在BaseOfKernelFile的4k字节存放FAT扇区
    pop ax
    mov byte [bOdd], 0
    mov bx, 3
    mul bx
    mov bx, 2
    div bx      ;ax乘以3除以2后得到簇号对应的偏移字节(相对于FAT表)
    cmp dx, 0   ;通过余数判断簇号对应的12bit是占偏移2字节的高位还是低位
    jz LABEL_EVEN
    mov byte [bOdd], 1
LABEL_EVEN:
    ;现在ax中存着簇号对应FATEntry的偏移量，接下来计算
    xor dx, dx
    mov bx, [BPB_BytesPerSec]
    div bx      ;通过除以512字节，ax中存着相对于FAT表的偏移扇区数，dx存着在扇区中的偏移量
    push dx
    mov bx, 0   ;读取FATEntry对应的两字节进入es:bx
    add ax, SectorNoOfFAT1  ;FAT表的第一扇区号
    mov cl, 2   ;读取两个两个扇区，因为FATEntry有可能跨越两个扇区
    call ReadSector ;读取到了es:bx中
    pop dx
    add bx, dx  ;得到在扇区中的偏移量
    mov ax, [es:bx]
    cmp byte [bOdd], 1      ;假如对应是偶数，占低12bit，反之占高12bit
    jnz LABEL_EVEN2
    shr ax, 4
LABEL_EVEN2:
    and ax, 0FFFh
    pop bx
    pop es
    ret ;最终ax中存着下一个簇号


;==============================GDT全局描述符表========================
;引入保护模式相关定义和常量
%include "pm.inc"
LABEL_GDT:      ;段基址Base     ,段界限Limit        ,属性Attr
    Descriptor  0,              0,                  0               ;空描述符
LABEL_DESC_FLAT_C:
    Descriptor  0,              0fffffh,   DA_CR | DA_32 | DA_LIMIT_4K    ;0-4G代码段
LABEL_DESC_FLAT_RW:
    Descriptor  0,              0fffffh,  DA_DRW | DA_32 | DA_LIMIT_4K     ;0-4G数据段
LABEL_DESC_VIDEO:
    Descriptor  0B8000h,        0ffffh,             DA_DRW | DA_DPL3          ;显存段描述符

GdtLen equ $ - LABEL_GDT    ;GDT全局描述符表长度
GdtPtr dw GdtLen - 1        ;GDT界限Limit
       dd BaseOfLoaderPhyAddr + LABEL_GDT  ;GDT基地址32位

;GDT中全局描述符对应的选择子
SelectorFlatC equ LABEL_DESC_FLAT_C - LABEL_GDT
SelectorFlatRW equ LABEL_DESC_FLAT_RW - LABEL_GDT
SelectorVideo equ LABEL_DESC_VIDEO - LABEL_GDT + SA_RPL3
;=====================================================================
;=========================保护模式下堆栈==============================
StackSpace: times 1024 db 0
TopOfStack equ BaseOfLoaderPhyAddr + $  ;栈顶指针
;=====================================================================

[section .s32]
align 32
[bits 32]   ;32位段
LABEL_PM_START:
    ;初始化各个寄存器的值
    mov ax, SelectorFlatRW
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax
    mov esp, TopOfStack
    mov ax, SelectorVideo
    mov gs, ax

    ;显示表头信息
    push szMemChkTitle
    call DispStr
    add esp, 4
    ;显示内存信息
    call DispMemSize
    ;启动分页机制
    call SetupPaging
    ;重新安放内核的内存位置
    call InitKernel
    jmp SelectorFlatC: KernelEntryPointPhyAddr  ;向内核交出控制权

DispMemSize:    ;显示内存信息
    push esi
    push edi
    push ecx
    mov esi, MemChkBuf
    mov ecx, [dwMCRNumber]   ;内存地址块ARDS个数
.LOOP_ARDS:
    mov edx, 5         ;每一个ARDS有五个子项
    mov edi, ARDSTruct
.LOOP_ARDS_ITEM1:
    push dword [esi]
    call DispInt        ;显示栈顶元素,对应一个子项
    pop eax
    stosd          ;将对应子项放入ARDS中
    add esi, 4
    dec edx
    cmp edx, 0
    jnz .LOOP_ARDS_ITEM1
    call DispReturn     ;一个ARDS显示完毕，换行
    cmp dword [dwType], 1
    jne .LOOP_ARDS_ITEM2        ;内存块Type != AddressRangeMemory
    mov eax, [dwBaseAddrLow]
    add eax, [dwLengthLow]
    cmp eax, [dwMemSize]
    jb .LOOP_ARDS_ITEM2         ;BaseAddrLow + LengthLow <= MemSize
    mov [dwMemSize], eax
.LOOP_ARDS_ITEM2:
    loop .LOOP_ARDS
    call DispReturn     ;所有ARDS显示完毕，换行显示最大内存地址
    push szRAMSize
    call DispStr
    add esp, 4
    push dword [dwMemSize]
    call DispInt
    add esp, 4
    pop ecx
    pop edi
    pop esi
    ret
;内存信息显示完毕

SetupPaging:    ;设置分页机制(最简单的线性地址和物理地址对等映射的页目录表和页表)
    ;根据内存大小计算应该初始化多少PDE和多少个PTE
    xor edx, edx
    mov eax, [dwMemSize]
    mov ebx, 400000h        ;400000h = 4M = 4096 * 1024 一个PDE对应一个页表，对应的内存大小
    div ebx
    mov ecx, eax
    test edx, edx
    jz .no_remainder
    inc ecx     ;共ecx个PDE表项
.no_remainder:
    mov [PageTableNumber], ecx    ;暂存页表个数，留到页表初始化时使用
    ;初始化页目录表
    mov ax, SelectorFlatRW
    mov es, ax
    mov edi, PageDirBase
    xor eax, eax
    mov eax, PageTableBase | PG_P | PG_USU | PG_RWW  ;存在的可读写用户级页表
.1:
    stosd           ;edi每次自增4
    add eax, 4096 ;下一个页表地址
    loop .1

    ;初始化所有页表(1024个页表，4M内存空间)
    mov eax, [PageTableNumber]     ;获取页表个数
    mov ebx, 1024       ;一个页表个数1024个PTE
    mul ebx
    mov ecx, eax     ;共1M个页表项，即1M个物理页
    mov edi, PageTableBase
    xor eax, eax
    mov eax, PG_P | PG_USU | PG_RWW
.2:
    stosd
    add eax, 4096   ;每页指向4K空间
    loop .2

    mov eax, PageDirBase
    mov cr3, eax        ;初始化页目录基址寄存器
    mov eax, cr0
    or eax, 80000000h
    mov cr0, eax          ;设置cr0寄存器最高位PG位，开启了分页机制
    jmp short .3
.3:
    nop
    ret
;分页机制启动完毕

;初始化内核，按照ELF文件的program header重新安排内核在内存中的存放位置
InitKernel:
    xor esi, esi
    mov cx, word [BaseOfKernelFilePhyAddr + 2Ch]   ;获取e_phnum程序头数量
    movzx ecx, cx
    mov esi, [BaseOfKernelFilePhyAddr + 1Ch]     ;获取e_phoff程序头偏移地址
    add esi, BaseOfKernelFilePhyAddr    ;获取程序头绝对物理地址
.Begin:
    mov eax, [esi + 0]  ;获取p_type段类型
    cmp eax, 0
    jz .NoAction
    push dword [esi + 010h]   ;获取p_filesz段在文件中的长度,并压栈
    mov eax, [esi + 04h]        ;获取p_offset段在文件中的偏移地址
    add eax, BaseOfKernelFilePhyAddr
    push eax    ;源地址压栈
    push dword [esi + 08h]      ;目标地址压栈,获取p_vaddr段在内存中的虚拟线性地址(线性地址与物理地址对等映射)
    call memcpy
    add esp, 12
.NoAction:
    add esi, 020h   ;读取下一个程序头
    dec ecx
    jnz .Begin
    ret
%include "lib.inc"  ;导入库函数