;========================
;测试使用段描述符，对应Orange's 3.1节
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
LABEL_DESC_CODE32:
    Descriptor  0,              SegCode32Len - 1,   DA_C + DA_32    ;32位非一致代码段描述符
LABEL_DESC_DISP:
    Descriptor  0B8000h,        0ffffh,             DA_DRW          ;显存段描述符
LABEL_DESC_SHOWMSG:
    Descriptor  0,             ShowMsgLen - 1,      DA_DR           ;显示字符段描述符

GdtLen equ $ - LABEL_GDT    ;GDT全局描述符表长度
GdtPtr dw GdtLen - 1        ;GDT界限Limit
       dd 0                 ;存储着GDT基地址32位，后面会重新初始化

;GDT中全局描述符对应的选择子
SelectorCode32 equ LABEL_DESC_CODE32 - LABEL_GDT
SelectorDisp equ LABEL_DESC_DISP - LABEL_GDT
SelectorShowMsg equ LABEL_DESC_SHOWMSG - LABEL_GDT
;===============end of section .gdt==========================

;============begin of section .s16=======================
[section .s16]
bits 16
LABEL_BEGIN:
    mov ax, cs         ;初始化段寄存器
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0100h

    ;初始化32位非一致代码段描述符，因为显存段基地址确定，无需再初始化
    xor eax, eax
    mov ax, cs
    shl eax, 4           ;相当于代码段寄存器左移4位，求出代码段基地址
    add eax, LABEL_SEG_CODE32   ;加上偏移地址求出32位代码段的真正物理地址
    mov word [LABEL_DESC_CODE32 + 2], ax   ;将基地址1存在偏移地址2处
    shr eax, 16
    mov byte [LABEL_DESC_CODE32 + 4], al    ;将基地址2存在偏移地址4处
    mov byte [LABEL_DESC_CODE32 + 7], ah    ;将基地址3存在偏移地址7处

    ;初始化待显示字符串段描述符
    xor eax, eax
    mov ax, ds
    shl eax, 4           ;相当于数据段寄存器左移4位，求出数据段基地址
    add eax, LABEL_SEG_SHOWMSG   ;加上偏移地址求出字符串段的真正物理地址
    mov word [LABEL_DESC_SHOWMSG + 2], ax   ;将基地址1存在偏移地址2处
    shr eax, 16
    mov byte [LABEL_DESC_SHOWMSG + 4], al    ;将基地址2存在偏移地址4处
    mov byte [LABEL_DESC_SHOWMSG + 7], ah    ;将基地址3存在偏移地址7处

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
;==========end of section .s16==================================

;============begin of section .s32================================
[section .s32]
bits 32
LABEL_SEG_CODE32:
    mov ax, SelectorShowMsg
    mov ds, ax     ;源：字符串段选择子
    mov ax, SelectorDisp
    mov es, ax       ;目的：显存段(数据段)选择子
    xor esi, esi
    mov edi, (80 * 12 + 30) * 2      ;屏幕(25行*80列)中间位置
    mov ah, 0Ch     ;显存属性：黑底红字
    cld             ;清除方向位
LOOP_START:
    lodsb   ;将[ds: esi]字节单元中的字符送入al寄存器中
    test al, al
    jz LOOP_END
    mov [es:edi], al  ;将字符送入显存单元中
    inc edi
    mov [es:edi], ah  ;设置属性字节
    inc edi
    jmp LOOP_START
LOOP_END:
    jmp $   ;一直在此处循环

SegCode32Len equ $ - LABEL_SEG_CODE32
;==============end of section .s32========================

;============begin of section .msg=============================
[section .msg]
LABEL_SEG_SHOWMSG:
    db 'hello world, hahaha!', 0
ShowMsgLen equ $ - LABEL_SEG_SHOWMSG
;=============end of section .msg================================