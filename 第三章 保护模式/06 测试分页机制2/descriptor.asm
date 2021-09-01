;========================
;检测内存可用大小，合理建立页表
;对应Orange's 3.3.6节
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
    Descriptor  0,              SegCode32Len - 1,   DA_C + DA_32    ;32位非一致代码段描述符
LABEL_DESC_CODE16:
    Descriptor  0,              0ffffh,   DA_C            ;16位非一致代码段描述符
LABEL_DESC_DATA:
    Descriptor  0,              DataLen - 1,        DA_DRW          ;标准数据段描述符
LABEL_DESC_STACK:
    Descriptor  0,              TopOfStack,         DA_DRWA+DA_32   ;32位栈段描述符
LABEL_DESC_DISP:
    Descriptor  0B8000h,        0ffffh,             DA_DRW          ;显存段描述符
LABEL_DESC_PAGE_DIR:
    Descriptor  PageDirBase,    4095,               DA_DRW          ;页目录表描述符
LABEL_DESC_PAGE_TABLE:
    Descriptor  PageTableBase,  4096 * 8 - 1,               DA_DRW ;页表描述符

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
SelectorPageDir equ LABEL_DESC_PAGE_DIR - LABEL_GDT
SelectorPageTable equ LABEL_DESC_PAGE_TABLE - LABEL_GDT
;===============end of section .gdt==========================

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

;保护模式下使用的符号
szPMMsg equ _szPMMsg - $$
szMemChkTitle equ _szMemChkTitle - $$
dwMCRNumber equ _dwMCRNumber - $$
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

    ;通过lgdt命令加载GDTR
    lgdt [GdtPtr]   ;将GdtPtr指向的内容加载到GDTR寄存器中

    ;清除中断标志位IF=0,不响应中断
    cli

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
    ;显示一个提示字符串"In Protect Mode now.^-^"
    push szPMMsg
    call DispStr
    add esp, 4
    ;显示表头
    push szMemChkTitle
    call DispStr
    add esp, 4

    call DispMemSize   ;显示内存信息
    call SetupPaging  ;启动分页

    ;跳转到16位代码段,进行保护模式向实模式的切换
    jmp SelectorCode16:0
;====================start 子函数定义===================
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
    push ecx    ;暂存页表个数，留到页表初始化时使用
    ;初始化页目录表
    mov ax, SelectorPageDir
    mov es, ax
    xor edi, edi
    xor eax, eax
    mov eax, PageTableBase | PG_P | PG_USU | PG_RWW  ;存在的可读写用户级页表
.1:
    stosd           ;edi每次自增4
    add eax, 4096 ;下一个页表地址
    loop .1

    ;初始化所有页表(1024个页表，4M内存空间)
    mov ax, SelectorPageTable
    mov es, ax
    pop eax ;弹出页表个数
    mov ebx, 1024       ;一个页表个数1024个PTE
    mul ebx
    mov ecx, eax     ;共1M个页表项，即1M个物理页
    xor edi, edi
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

;==============start of section .s16code========================

;==============start of section .s16code========================