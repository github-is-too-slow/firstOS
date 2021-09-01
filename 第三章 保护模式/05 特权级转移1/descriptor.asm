;========================
;测试retf指令实现从高特权级返回至低特权级
;对应Orange's 3.2.4节
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
    Descriptor  0,              TopOfStack,         DA_DRWA + DA_32   ;32位栈段描述符
LABEL_DESC_TEST:
    Descriptor  0500000h,       0ffffh,             DA_DRW          ;测试用的大内存地址段描述符
LABEL_DESC_DISP:
    Descriptor  0B8000h,        0ffffh,             DA_DRW + DA_DPL3         ;显存段描述符
LABEL_DESC_LDT:
    Descriptor  0,              LdtLen - 1,         DA_LDT          ;LDT局部描述符表描述符
LABEL_DESC_CODE_DEST:
    Descriptor  0,              SegCodeDestLen - 1, DA_C + DA_32    ;32位非一致代码描述符
LABEL_CALL_GATE_TEST:
    Gate        SelectorCodeDest, 0,     0,  DA_386CGate + DA_DPL0 ;存在的特权级为0的调用门
LABEL_DESC_CODE_RING3:
    Descriptor  0,              SegCodeRing3Len - 1, DA_C + DA_32 + DA_DPL3   ;存在的32位用户特权级代码段
LABEL_DESC_STACK3:
    Descriptor  0,              TopOfStack3,         DA_DRWA + DA_32 + DA_DPL3 ;存在的32位用户级可读写栈段

GdtLen equ $ - LABEL_GDT    ;GDT全局描述符表长度
GdtPtr dw GdtLen - 1        ;GDT界限Limit
       dd 0                 ;存储着GDT基地址32位，后面会重新初始化

;GDT中全局描述符对应的选择子
SelectorNormal equ LABEL_DESC_NORMAL - LABEL_GDT
SelectorCode32 equ LABEL_DESC_CODE32 - LABEL_GDT
SelectorCode16 equ LABEL_DESC_CODE16 - LABEL_GDT
SelectorData equ LABEL_DESC_DATA - LABEL_GDT
SelectorStack equ LABEL_DESC_STACK - LABEL_GDT
SelectorTest equ LABEL_DESC_TEST - LABEL_GDT
SelectorDisp equ LABEL_DESC_DISP - LABEL_GDT
SelectorLdt equ LABEL_DESC_LDT - LABEL_GDT   ;用于加载局部描述符到对应的寄存器中
SelectorCodeDest equ LABEL_DESC_CODE_DEST - LABEL_GDT
SelectorCallGateTest equ LABEL_CALL_GATE_TEST - LABEL_GDT
SelectorCodeRing3 equ LABEL_DESC_CODE_RING3 - LABEL_GDT + SA_RPL3 ;请求特权级跟DPL保持一致
SelectorStack3 equ LABEL_DESC_STACK3 - LABEL_GDT + SA_RPL3
;===============end of section .gdt==========================

;=============begin of section .ldt==============================
[section .ldt]    ;LDT局部描述符表段，通过LDT在GDT中描述符对应的选择子加载
align 32
LABEL_LDT:      ;段基址Base     ,段界限Limit        ,属性Attr
LABEL_LDT_DESC_CODEA:
    Descriptor  0,              SegCodeALen - 1,   DA_C + DA_32    ;32位非一致代码段局部描述符

LdtLen equ $ - LABEL_LDT    ;LDT局部描述符表长度

;LDT中局部描述符对应的选择子
SelectorCodeA equ LABEL_LDT_DESC_CODEA - LABEL_LDT + SA_TIL
;===============end of section .gdt==========================

;====================begin of section .data================
[section .data1]     ;标准数据段
align 32    ;对齐32字节
[bits 32]
LABEL_DATA:
SPValueInRealMode dw 0
MsgInRealMode:
    db 'In Protect Mode now.^-^', 0
OffsetOfMsg equ  MsgInRealMode - $$
StrTest:
    db 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 0
OffsetOfStrTest equ StrTest - $$
MsgTestLdt:
    db 'use local descriptor table.^-^', 0
OffsetOfMsgTestLdt equ MsgTestLdt - $$
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
    mov word [SPValueInRealMode], sp

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

    ;初始化全局描述符表中局部描述符表描述符
    xor eax, eax
    mov ax, ds
    shl eax, 4
    add eax, LABEL_LDT
    mov word [LABEL_DESC_LDT + 2], ax
    shr eax, 16
    mov byte [LABEL_DESC_LDT + 4], al
    mov byte [LABEL_DESC_LDT + 7], ah

    ;初始化局部描述符表中的描述符
    xor eax, eax
    mov ax, ds
    shl eax, 4
    add eax, LABEL_SEG_CODEA
    mov word [LABEL_LDT_DESC_CODEA + 2], ax
    shr eax, 16
    mov byte [LABEL_LDT_DESC_CODEA + 4], al
    mov byte [LABEL_LDT_DESC_CODEA + 7], ah

    ;初始化调用门的目标段代码段
    xor eax, eax
    mov ax, cs
    shl eax, 4
    add eax, LABEL_SEG_CODE_DEST
    mov word [LABEL_DESC_CODE_DEST + 2], ax
    shr eax, 16
    mov byte [LABEL_DESC_CODE_DEST + 4], al
    mov byte [LABEL_DESC_CODE_DEST + 7], ah

    ;初始化Ring3用户级代码段
    xor eax, eax
    mov ax, cs
    shl eax, 4
    add eax, LABEL_CODE_RING3
    mov word [LABEL_DESC_CODE_RING3 + 2], ax
    shr eax, 16
    mov byte [LABEL_DESC_CODE_RING3 + 4], al
    mov byte [LABEL_DESC_CODE_RING3 + 7], ah

    ;初始化Ring3用户级栈段
    xor eax, eax
    mov ax, cs
    shl eax, 4
    add eax, LABEL_STACK3
    mov word [LABEL_DESC_STACK3 + 2], ax
    shr eax, 16
    mov byte [LABEL_DESC_STACK3 + 4], al
    mov byte [LABEL_DESC_STACK3 + 7], ah

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
    mov word sp, [SPValueInRealMode]

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

;============begin of section .l32================================
[section .l32]
[bits 32]
LABEL_SEG_CODEA:
    mov ah, 0Ch
    mov esi, OffsetOfMsgTestLdt
    cld
LOOP_START_LDT:
    lodsb
    test al, al
    jz LOOP_END_LDT
    mov [gs:edi], ax
    add edi, 2
    jmp LOOP_START_LDT
LOOP_END_LDT:
    ;跳转到16位代码段，准备切换至实模式
    jmp SelectorCode16:0
SegCodeALen equ $ - LABEL_SEG_CODEA
;============end of section .l32================================

;============begin of section .s32================================
[section .s32]
[bits 32]
LABEL_SEG_CODE32:
    mov ax, SelectorData
    mov ds, ax     ;源: 标准数据段选择子
    mov ax, SelectorTest
    mov es, ax       ;目的：大内存地址测试段选择子
    mov ax, SelectorDisp
    mov gs, ax      ;显存段选择子
    mov ax, SelectorStack
    mov ss, ax      ;堆栈段选择子
    mov esp, TopOfStack     ;设置栈顶指针
    ;显示一个提示字符串"In Protect Mode now.^-^"
    mov ah, 0Ch         ;黑底红字
    xor esi, esi
    xor edi, edi
    mov esi, OffsetOfMsg    ;待显示字符串的偏移
    mov edi, (80 * 5 + 0) * 2  ;屏幕10行0列
    cld      ;重置方向位
LOOP_START:
    lodsb   ;将[ds: esi]字节单元中的字符送入al寄存器中
    test al, al
    jz LOOP_END
    mov [gs:edi], ax  ;将字符送入显存单元中
    add edi, 2
    jmp LOOP_START
LOOP_END: ;显示完毕
    ;测试使用retf指令从DPL0返回至DPL3
    ;retf:依次返回eip、cs、esp、ss
    push SelectorStack3
    push TopOfStack3
    push SelectorCodeRing3
    push 0
    retf

    call DispReturn     ;换行显示
    call TestRead       ;读取测试段的内容,并换行
    call TestWrite      ;写入测试段
    call TestRead       ;读取测试段内容，检测读写内容是否一致

    ;测试调用门(无特权级变换)
    call SelectorCallGateTest:0
    ;直接进行远跳转
    ;call SelectorCodeDest:0

    ;加载局部描述符表
    mov ax, SelectorLdt
    lldt ax
    ;跳转到局部描述符对应的局部代码段
    jmp SelectorCodeA:0
;====================start 子函数定义===================
TestWrite:
    push esi  ;因为设置了ss和esp指针，因此均存放在新设置的栈段中
    push edi
    xor esi, esi
    xor edi, edi
    mov esi, OffsetOfStrTest    ;源数据偏移
    cld
WRITE_BEGIN:
    lodsb
    test al, al
    jz WRITE_END
    mov [es:edi], al
    inc edi
    jmp WRITE_BEGIN
WRITE_END:
    pop edi
    pop esi
    ret         ;将cs/ip从栈中弹出，恢复到断点处执行
;TestWrite 函数结束

TestRead:
    xor esi, esi
    mov ecx, 26
READ_BEGIN:
    mov al, [es:esi]
    call DispAL         ;将AL中的值以16进制形式显示
    inc esi
    loop READ_BEGIN
    call DispReturn         ;换行显示
    ret
;TestRead 函数结束

;功能：16进制显示AL寄存器中的数字
;默认：数字已经在AL寄存器中；edi指向要显示的位置
DispAL:
    push ecx
    push edx
    mov ah, 0Ah     ;黑底绿字
    mov dl, al
    shr al, 4       ;保留高位
    mov ecx, 2
DISP_BEGIN:
    and al, 01111b
    cmp al, 9   ;判断待显示的4位二进制是介于0-9之间还是10-15之间
    ja CHAR     ;按字母ASCII处理
    add al, '0' ;按数字ASCII处理
    jmp DISPLAY
CHAR:
    sub al, 0Ah  ;先减去10
    add al, 'A'
DISPLAY:
    mov [gs: edi], ax
    add edi, 2
    mov al, dl
    loop DISP_BEGIN
    add edi, 2
    pop edx
    pop ecx
    ret
;DispAL 函数结束

DispReturn:  ;实现换行功能
    push eax
    push ebx
    mov eax, edi
    mov bl, 160     ;一行占的内存
    div bl          ;8位除数，被除数存在ax寄存器中，商存在al中，余数存在ah中
    and eax, 0ffffh     ;仅保留商
    inc eax
    mov bl, 160
    mul bl          ;偏移若干行
    mov edi, eax
    pop ebx
    pop eax
    ret
;DispReturn 函数结束
;====================start 子函数定义===================
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
    and al, 11111110b
    mov cr0, eax
LABEL_GO_BACK_TO_REAL:
    jmp 0:LABEL_REAL_ENTRY      ;段地址会在实模式程序开始处被设置成正确的值,这里计算物理地址是按实模式下进行的
SegCode16Len equ $ - LABEL_SEG_CODE16
;==============end of section .s16code========================

;==============start of section .sdest========================
[section .call] ;调用门目标段
[bits 32]
LABEL_SEG_CODE_DEST:
    mov ax, SelectorDisp
    mov gs, ax
    mov edi, (80 * 10 + 0) * 2
    mov ah, 0Ah
    mov al, 'C'
    mov [gs:edi], ax
    add edi, 2
    retf
SegCodeDestLen equ $ - LABEL_SEG_CODE_DEST
;==============end of section .sdest========================

;==============start of section .s3========================
[section .s3]  ;DPL3堆栈段
align 32
[bits 32]
LABEL_STACK3:
    times 521 db 0
TopOfStack3 equ $ - LABEL_STACK3 - 1
;==============end of section .s3========================

;==============start of section .ring3========================
[section .ring3]  ;用户级代码段
align 32
[bits 32]
LABEL_CODE_RING3:
    mov ax, SelectorDisp
    mov gs, ax
    mov edi, (80 * 8 + 0) * 2
    mov ah, 0Ch
    mov al, 'R'
    mov [gs:edi], ax
    jmp $
SegCodeRing3Len equ $ - LABEL_CODE_RING3
;==============start of section .ring3========================