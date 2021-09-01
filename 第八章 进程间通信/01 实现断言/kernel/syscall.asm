%include "sconst.inc"

_NR_get_ticks           equ     0           ;get_ticks()子函数号
_NR_write               equ     1           ;write()子函数号
_NR_printx              equ     2           ;printx()子函数号
INT_VECTOR_SYS_CALL     equ     0x90        ;系统调用号

global get_ticks
global write
global printx

[section .text]
[bits 32]
;原型：PUBLIC int get_ticks();
get_ticks:
    mov eax, _NR_get_ticks      ;ax中默认保存系统调用子程序号
    int INT_VECTOR_SYS_CALL     ;当由ring0系统调用程序返回时，eax中存放着结果
    ret

;原型：PUBLIC void write(char *buf, int len);
write:
    mov eax, _NR_write     ;系统调用子程序号
    mov ebx, [esp + 4]     ;buf待写入字符串首地址
    mov ecx, [esp + 8]     ;字符串长度
    int INT_VECTOR_SYS_CALL  ;此时切换堆栈，但ebx/ecx寄存器的值不会变化
    ret

; ====================================================================================
;                          void printx(char* s);
; ====================================================================================
printx:
	mov	eax, _NR_printx
	mov	edx, [esp + 4]
	int	INT_VECTOR_SYS_CALL
	ret

