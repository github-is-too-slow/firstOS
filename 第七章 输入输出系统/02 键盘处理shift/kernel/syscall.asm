%include "sconst.inc"

_NR_get_ticks           equ     0           ;get_ticks()子函数号
INT_VECTOR_SYS_CALL     equ     0x90        ;系统调用号

global get_ticks

[section .text]
[bits 32]
get_ticks:
    mov eax, _NR_get_ticks      ;ax中默认保存系统调用子程序号
    int INT_VECTOR_SYS_CALL     ;当由ring0系统调用程序返回时，eax中存放着结果
    ret

