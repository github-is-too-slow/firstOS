%include "sconst.inc"

_NR_printx              equ     0           ;printx()子函数号
_NR_sendrec             equ     1           ;sendrec()子函数号
INT_VECTOR_SYS_CALL     equ     0x90        ;系统调用号

global printx
global sendrec

[section .text]
[bits 32]
; ====================================================================================
;                         原型：PUBLIC void printx(char* s);
; ====================================================================================
printx:
	mov	eax, _NR_printx
	mov	edx, [esp + 4]
	int	INT_VECTOR_SYS_CALL
	ret

;原型：PUBLIC int sendrec(int function, int src_dest, MESSAGE *p_msg);
sendrec:
    mov eax, _NR_sendrec
    mov ebx, [esp + 4]  ;function
    mov ecx, [esp + 8]  ;src_dest
    mov edx, [esp + 12]  ;p_msg
    int INT_VECTOR_SYS_CALL
    ret
