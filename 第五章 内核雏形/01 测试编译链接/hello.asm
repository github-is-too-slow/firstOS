;====================================================
;测试编译链接方法,对应5.1
;=======================================================
;初始化数据区
[section .data]
StrHello: db "hello, world!", 0Ah
StrLen equ $ - StrHello

;代码区
[section .text]
global _start
_start:
    mov edx, StrLen
    mov ecx, StrHello
    mov ebx, 1      ;返回值
    mov eax, 4      ;sys_write
    int 0x80        ;系统调用
    mov ebx, 0
    mov eax, 1      ;sys_exit
    int 0x80