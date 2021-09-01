;========================
;测试中断机制：中断控制器的初始化，设置中断描述符表IDT
;对应Orange's 3.4.3节
;========================

%include "common.inc"
%define _DEBUG_
%ifdef _DEBUG_
    org 0100h   ;在Freedos系统下调试时加载到0100h偏移地址处
%else
    org 07c00h  ;在运行模型时加载到07c00h处
%endif

jmp LABEL_BEGIN     ;跳转到16位实模式程序处运行

;=============begin of section .gdt==============================
[section .gdt]    ;GDT全局描述符表段
LABEL_GDT:      ;段基址Base     ,段界限Limit        ,属性Attr
    Descriptor  0,              0,                  0               ;空描述符
LABEL_DESC_NORMAL:
    Descriptor  0,              0ffffh,              DA_DRW          ;Normal段描述符
LABEL_DESC_CODE32:
    Descriptor  0,              SegCode32Len - 1,   DA_CR | DA_32    ;32位非一致代码段描述符,这里代码段必须是可读可执行的DA_CR
LABEL_DESC_CODE16:
    Descriptor  0,              0ffffh,   DA_C            ;16位非一致代码段描述符
LABEL_DESC_DATA:
    Descriptor  0,              DataLen - 1,        DA_DRW          ;标准数据段描述符
LABEL_DESC_STACK:
    Descriptor  0,              TopOfStack,         DA_DRWA | DA_32   ;32位栈段描述符
LABEL_DESC_DISP:
    Descriptor  0B8000h,        0ffffh,             DA_DRW          ;显存段描述符
LABEL_DESC_FLAT_C:
    Descriptor  0,    0fffffh,               DA_CR | DA_32 | DA_LIMIT_4K          ;可执行代码段描述符
LABEL_DESC_FLAT_RW:
    Descriptor  0,  0fffffh,               DA_DRW | DA_LIMIT_4K ;可读写数据段描述符

GdtLen equ $ - LABEL_GDT    ;GDT全局描述符表长度
GdtPtr dw GdtLen - 1        ;GDT界限Limit
       dd 0                 ;存储着GDT基地址32位，后面会重新初始化

;GDT中全局描述符对应的选择子
SelectorNormal equ LABEL_DESC_NORMAL - LABEL_GDT
SelectorCode32 equ LABEL_DESC_CODE32 - LABEL_GDT
SelectorCode16 equ LABEL_DESC_CODE16 - LABEL_GDT
SelectorData equ LABEL_DESC_DATA - LABEL_GDT
SelectorStack equ LABEL_DESC_STACK - LABEL_GDT
SelectorDisp equ LABEL_DESC_DISP - LABEL_GDT
SelectorFlatC equ LABEL_DESC_FLAT_C - LABEL_GDT
SelectorFlatRW equ LABEL_DESC_FLAT_RW - LABEL_GDT
;===============end of section .gdt==========================

;==============start of section .idt========================
[section .idt]  ;中断描述符表
align 32
[bits 32]
LABEL_IDT:
%rep 255 ; 目标段选择子，       段内偏移，          Dcount，    属性
        Gate    SelectorCode32,     SpuriousHandler,    0,          DA_386IGate
%endrep
IdtLen equ $ - LABEL_IDT
IdtPtr  dw IdtLen - 1    ;段界限
        dd 0            ;基地址
;==============end of section .idt========================

;====================begin of section .data================
[section .data1]     ;标准数据段
align 32    ;对齐32字节
[bits 32]
LABEL_DATA:
;实模式下使用的符号
_SPValueInRealMode: dw 0
_szPMMsg:
    db 'In Protect Mode now.^-^', 0Ah, 0Ah, 0
_szMemChkTitle:
    db 'BaseAddrL BaseAddrH LengthLow LengthHigh    Type', 0Ah, 0
_dwMCRNumber: dd 0   ;Address Range Descriptor Structure地址范围描述符结构的个数
_PageTableNumber: dd 0      ;页表个数
_MemChkBuf:
    times 256 db 0     ;用于存储地址描述符结构体(20byte)的缓冲区(可以存12个)
_dwMemSize: dd 0
_dwDispPos: dd (80 * 2 + 0) * 2
_szRAMSize: db 'RAM size:', 0
_szReturn: db 0Ah, 0
_ARDStruct:
    _dwBaseAddrLow: dd 0
    _dwBaseAddrHigh: dd 0
    _dwLengthLow: dd 0
    _dwLengthHigh: dd 0
    _dwType: dd 0
_SavedIDTR: dd 0        ;保存IDTR
_SavedIMR: dd 0         ;保存中断屏蔽寄存器(主控制器21h端口)的值

;保护模式下使用的符号
szPMMsg equ _szPMMsg - $$
szMemChkTitle equ _szMemChkTitle - $$
dwMCRNumber equ _dwMCRNumber - $$
PageTableNumber equ _PageTableNumber - $$
MemChkBuf equ _MemChkBuf - $$
dwMemSize equ _dwMemSize - $$
dwDispPos equ _dwDispPos - $$
szRAMSize equ _szRAMSize - $$
szReturn equ _szReturn - $$
ARDSTruct equ _ARDStruct - $$
    dwBaseAddrLow equ _dwBaseAddrLow - $$
    dwBaseAddrHigh equ _dwBaseAddrHigh - $$
    dwLengthLow equ _dwLengthLow - $$
    dwLengthHigh equ _dwLengthHigh - $$
    dwType equ _dwType - $$
SavedIDTR equ _SavedIDTR - $$
SavedIMR equ _SavedIMR - $$
DataLen equ $ - LABEL_DATA
;====================begin of section .data================

;===============begin of sectin .gs=======================
[section .gs]       ;全局栈段
align 32
[bits 32]
LABEL_STACK:
    times 512 db 0
TopOfStack equ $ - LABEL_STACK - 1      ;栈顶指针的偏移量，要多减去一个1
;===============end of sectin .gs=======================

;============begin of section .s16=======================
[section .s16]
align 32
[bits 16]
LABEL_BEGIN:
    mov ax, cs         ;初始化段寄存器
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0100h
    mov word [LABEL_GO_BACK_TO_REAL + 3], ax ;设置实模式下的段地址
    mov word [_SPValueInRealMode], sp

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
    ;初始化32位非一致代码段描述符，因为显存段基地址确定，无需再初始化
    xor eax, eax
    mov ax, cs
    shl eax, 4           ;相当于代码段寄存器左移4位，求出代码段基地址
    add eax, LABEL_SEG_CODE32   ;加上偏移地址求出32位代码段的真正物理地址
    mov word [LABEL_DESC_CODE32 + 2], ax   ;将基地址1存在偏移地址2处
    shr eax, 16
    mov byte [LABEL_DESC_CODE32 + 4], al    ;将基地址2存在偏移地址4处
    mov byte [LABEL_DESC_CODE32 + 7], ah    ;将基地址3存在偏移地址7处

    ;初始化16位非一致代码段描述符
    xor eax, eax
    mov ax, cs
    shl eax, 4
    add eax, LABEL_SEG_CODE16
    mov word [LABEL_DESC_CODE16 + 2], ax
    shr eax, 16
    mov byte [LABEL_DESC_CODE16 + 4], al
    mov byte [LABEL_DESC_CODE16 + 7], ah

    ;初始化标准数据段描述符
    xor eax, eax
    mov ax, ds
    shl eax, 4
    add eax, LABEL_DATA
    mov word [LABEL_DESC_DATA + 2], ax
    shr eax, 16
    mov byte [LABEL_DESC_DATA + 4], al
    mov byte [LABEL_DESC_DATA + 7], ah

    ;初始化全局栈段描述符
    xor eax, eax
    mov ax, ss
    shl eax, 4
    add eax, LABEL_STACK
    mov word [LABEL_DESC_STACK + 2], ax
    shr eax, 16
    mov byte [LABEL_DESC_STACK + 4], al
    mov byte [LABEL_DESC_STACK + 7], ah

    ;CPU内部有一个GDTR寄存器，存储着GDT全局描述符表的基地址和界限
    ;为加载GDTR做准备
    xor eax, eax
    mov ax, ds
    shl eax, 4
    add eax, LABEL_GDT      ;求出全局描述符表GDT的基地址
    mov dword [GdtPtr + 2], eax         ;将GDT基地址存在GdtPtr数据结构中

    ;CPU内部有一个IDTR寄存器，存储着IDT中断描述表的基地址和界限
    xor eax, eax
    mov ax, ds
    shl eax, 4
    add eax, LABEL_IDT
    mov dword [IdtPtr + 2], eax

    ;保存实模式下的IDTR
    sidt [_SavedIDTR]
    ;保存中断屏蔽寄存器(IMR: interrupt mask register,对应主中断控制器21h端口)中的值
    in al, 21h
    mov [_SavedIMR], al

    ;通过lgdt命令加载GDTR
    lgdt [GdtPtr]   ;将GdtPtr指向的内容加载到GDTR寄存器中

    ;清除中断标志位IF=0,不响应中断
    cli

    ;在关中断后加载IDT
    lidt [IdtPtr]

    ;打开地址线A20,
    ;cpu通过检测A20地址线是否打开确定当地址超过1M时是否回卷地址
    in al, 92h
    or al, 00000010b
    out 92h, al

    ;最后一步，切换cr0寄存器最后一位PE(protection enable)保护模式位，
    ;从而真正从实模式切换到保护模式
    mov eax, cr0
    or eax, 1  ;将cr0寄存器PE位置1
    mov cr0, eax

    ;调转到32位保护模式程序
    ;此时已经进入保护模式下，根据SelectorCode32选择子选择描述符，得到32位段基址
    ;dword指明偏移地址0是32位
    jmp dword SelectorCode32:0

LABEL_REAL_ENTRY:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov word sp, [_SPValueInRealMode]

    lidt [_SavedIDTR]       ;恢复IDTR
    mov al, [_SavedIMR]
    out 21h, al ;恢复中断屏蔽寄存器的值

    ;关闭A20地址线
    in al, 92h
    and al, 11111101b
    out 92h, al

    ;开中断
    sti

    ;回到dos
    mov word ax, 4c00h
    int 21h
;==========end of section .s16==================================

;============begin of section .s32================================
[section .s32]
align 32
[bits 32]
LABEL_SEG_CODE32:
    mov ax, SelectorData
    mov ds, ax     ;源: 标准数据段选择子
    mov ax, SelectorData
    mov es, ax
    mov ax, SelectorDisp
    mov gs, ax      ;显存段选择子
    mov ax, SelectorStack
    mov ss, ax      ;堆栈段选择子
    mov esp, TopOfStack     ;设置栈顶指针

    call Init8259A      ;初始化8259A中断控制器
    int 080h        ;软中断，因为在实模式下cli关了中断，不会立即响应中断
    sti     ;开中断

    ;显示一个提示字符串"In Protect Mode now.^-^"
    push szPMMsg
    call DispStr
    add esp, 4
    ;显示表头
    push szMemChkTitle
    call DispStr
    add esp, 4

    call DispMemSize   ;显示内存信息
    call PagingDemo  ;测试分页机制，切换页目录表和页表

    ;跳转到16位代码段,进行保护模式向实模式的切换
    jmp SelectorCode16:0
;====================start 子函数定义===================
Init8259A:
    ;ICW: initialization command word
    mov al, 011h   ;需要ICW4，使用级联8259A
    out 020h, al    ;主8259A,ICW1
    call io_delay

    out 0A0h, al    ;从8259A,ICW1
    call io_delay

    mov al, 020h   ;IRQ0(时钟中断源)对应中断向量0x20
    out 021h, al   ;主8259A,ICW2
    call io_delay

    mov al, 028h  ;IRQ8(时钟中断源)对应中断向量0x28
    out 0A1h, al      ;从8259A,ICW2
    call io_delay

    mov al, 004h   ;IR2对应从8259A
    out 021h, al    ;主8259A,ICW3
    call io_delay

    mov al, 002h    ;对应主8259A的IR2
    out 0A1h, al    ;从8259A,ICW3
    call io_delay

    mov al, 001h  ;正常EOI，使用80X86模式
    out 021h, al    ;主8259A,ICW4
    call io_delay

    out 0A1h, al    ;从8259A,ICW4
    call io_delay

    ;OCW: operation command word
    mov al, 11111111b   ;屏蔽主8259A所有中断
    out 021h, al   ;主8259A,OCW1, 对应位为1，关闭中断
    call io_delay

    mov al, 11111111b       ;屏蔽从8259A所有中断
    out 0A1h, al    ;从8259A,OCW1
    call io_delay
    ret

io_delay: ;延迟函数
    nop
    nop
    nop
    nop
    ret

_SpuriousHandler:
SpuriousHandler equ _SpuriousHandler - $$
    mov ah, 0Ch         ;黑底红字
    mov al, '!'
    mov [gs:((80 * 0 + 75) * 2)], ax
    jmp $
    iretd

SetupPaging:    ;设置分页机制
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
    ;push ecx
    mov [PageTableNumber], ecx    ;暂存页表个数，留到页表初始化时使用
    ;初始化页目录表
    mov ax, SelectorFlatRW
    mov es, ax
    ;xor edi, edi
    mov edi, PageDirBase0
    xor eax, eax
    mov eax, PageTableBase0 | PG_P | PG_USU | PG_RWW  ;存在的可读写用户级页表
.1:
    stosd           ;edi每次自增4
    add eax, 4096 ;下一个页表地址
    loop .1

    ;初始化所有页表(1024个页表，4M内存空间)
    ;mov ax, SelectorPageTable
    ;mov es, ax
    ;pop eax
    mov eax, [PageTableNumber]     ;弹出页表个数
    mov ebx, 1024       ;一个页表个数1024个PTE
    mul ebx
    mov ecx, eax     ;共1M个页表项，即1M个物理页
    ;xor edi, edi
    mov edi, PageTableBase0
    xor eax, eax
    mov eax, PG_P | PG_USU | PG_RWW
.2:
    stosd
    add eax, 4096   ;每页指向4K空间
    loop .2

    mov eax, PageDirBase0
    mov cr3, eax        ;初始化页目录基址寄存器
    mov eax, cr0
    or eax, 80000000h
    mov cr0, eax          ;设置cr0寄存器最高位PG位，开启了分页机制
    jmp short .3
.3:
    nop
    ret
;分页机制启动完毕

PagingDemo:  ;演示分页机制：复制目标代码，切换页表
    mov ax, cs
    mov ds, ax
    mov ax, SelectorFlatRW   ;目标段
    mov es, ax

    push LenFoo  ;待复制代码段
    push OffsetFoo  ;待复制代码在本段内偏移
    push ProcFoo ;复制目标地址
    call MemCpy
    add esp, 12

    push LenBar
    push OffsetBar
    push ProcBar
    call MemCpy
    add esp, 12

    push LenPagingDemoAll
    push OffsetPagingDemoProc
    push ProcPagingDemo
    call MemCpy
    add esp, 12

    mov ax, SelectorData
    mov ds, ax
    mov es, ax      ;恢复ds、es

    call SetupPaging  ;启动分页，cr3页目录基址寄存器指向页目录表0

    call SelectorFlatC:ProcPagingDemo
    call PSwitch      ;切换页目录，改变地址映射关系，同一线性地址LinearAddr指向不同的代码
    call SelectorFlatC:ProcPagingDemo
    ret
;分页机制测试完毕

;真正实现切换页表的函数，改变线性地址到物理地址的映射关系
PSwitch:
    ;初始化页目录表
    mov ax, SelectorFlatRW
    mov es, ax
    ;xor edi, edi
    mov edi, PageDirBase1
    xor eax, eax
    mov eax, PageTableBase1 | PG_P | PG_USU | PG_RWW  ;存在的可读写用户级页表
    mov ecx, [PageTableNumber]      ;获取页表个数，即PDE项个数
    cld
.ps1:
    stosd           ;edi每次自增4
    add eax, 4096 ;下一个页表地址
    loop .ps1

    ;初始化所有页表(1024个页表，4M内存空间)
    ;mov ax, SelectorPageTable
    ;mov es, ax
    ;pop eax
    mov eax, [PageTableNumber]     ;获取页表个数
    mov ebx, 1024       ;一个页表个数1024个PTE
    mul ebx
    mov ecx, eax     ;共1M个页表项，即1M个物理页
    ;xor edi, edi
    mov edi, PageTableBase1
    xor eax, eax
    mov eax, PG_P | PG_USU | PG_RWW
.ps2:
    stosd
    add eax, 4096   ;每页指向4K空间
    loop .ps2

    ;此时改变LinearAddrDemo线性地址的映射关系，只需改变一个PTE指向的页物理地址即可，其余映射关系不变
    mov eax, LinearAddrDemo
    shr eax, 22     ;取页目录表项索引,正常应该去查找页表基址,但页表都是连续的，所以可以*4096直接获取
    mov ebx, 4096  ;页目录表项对应一个页(4k大小)
    mul ebx
    mov ecx, eax
    mov eax, LinearAddrDemo
    shr eax, 12
    and eax, 03FFh; 11111,11111b(10bit),仅保留页表项索引
    mov ebx, 4 ;页表项大小4byte
    mul ebx
    add eax, ecx
    add eax, PageTableBase1 ;加上页表的基址，得到对应的页表项的真正物理地址
    mov dword [es:eax], ProcBar | PG_P | PG_USU | PG_RWW    ;因为物理页是4K对齐的，所以可以直接将物理基址放入页表项中即可

    mov eax, PageDirBase1
    mov cr3, eax        ;切换了页目录基址寄存器中的页目录基址
    ;mov eax, cr0
    ;or eax, 80000000h
    ;mov cr0, eax          ;之前已经开启了分页机制
    jmp short .ps3
.ps3:
    nop
    ret


;被复制代码段1
PagingDemoProc:
OffsetPagingDemoProc equ PagingDemoProc - $$
    mov eax, LinearAddrDemo  ;复制后的代码此时通过call SelectorFlatC:ProcPagingDemo进入SelectorFlatC
    call eax  ;LinearAddrDemo是相对于SelectorFlatC的偏移地址
    retf  ;call是段间远调用
LenPagingDemoAll equ $ - PagingDemoProc

;被复制代码段2
foo:
OffsetFoo equ foo - $$
    mov ah, 0Ch     ;黑底红字
    mov al, 'F'
    mov [gs:((80 * 17 + 0) * 2)], ax
    mov al, 'o'
    mov [gs:((80 * 17 + 1) * 2)], ax
    mov [gs:((80 * 17 + 2) * 2)], ax  ;显示Foo
    ret
LenFoo equ $ - foo

;被复制代码段3
bar:
OffsetBar equ bar - $$
    mov ah, 0Ah         ;黑底绿字
    mov al, 'B'
    mov [gs:((80 * 18 + 0) * 2)], ax
    mov al, 'a'
    mov [gs:((80 * 18 + 1) * 2)], ax
    mov al, 'r'
    mov [gs:((80 * 18 + 2) * 2)], ax  ;显示Bar
    ret
LenBar equ $ - bar

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
;====================start 子函数定义===================
%include "lib.inc"  ;导入库函数
SegCode32Len equ $ - LABEL_SEG_CODE32
;==============end of section .s32========================

;==============start of section .s16code========================
[section .s16code]
align 32
[bits 16]
LABEL_SEG_CODE16:
    mov ax, SelectorNormal
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ;切换到实模式
    mov eax, cr0
    and eax, 7FFFFFFEh  ;PE=0, PG=0
    mov cr0, eax
LABEL_GO_BACK_TO_REAL:
    jmp 0:LABEL_REAL_ENTRY      ;段地址会在实模式程序开始处被设置成正确的值,这里计算物理地址是按实模式下进行的
SegCode16Len equ $ - LABEL_SEG_CODE16
;==============end of section .s16code========================
