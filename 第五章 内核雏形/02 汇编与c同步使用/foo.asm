;=========================================
;汇编与c语言同时使用，汇编代码是程序入口
;二者相互调用的关键：global:导出、extern:导入
;============================================

extern choose  ;引用c文件中的int choose(int a, int b);

[section .data]
num1    dd  3
num2    dd 4

[section .text]
global _start       ;导出_start,告诉链接器ld，程序入口在此汇编程序中,类似于C语言中的main
global myprint      ;导出myprint,以便可以在c程序中使用
_start:
    push dword [num2]
    push dword [num1]       ;实参从右到左依次入栈
    call choose      ;将eip压栈，函数choose则从esp + 4的位置开始取实参
    add esp, 8      ;将实参弹出栈顶
    mov ebx, 0
    mov eax, 1  ;sys_exit
    int 0x80

;自定义参数void myprint(char* msg, int len)
;c调用约定(C Callint Convention):参数从右到左依次入栈，并右调用者Caller清理堆栈
;当在C语言中调用myprint函数时，编译器首先将实参从右到左压栈，然后call myprint
;因此eip在栈顶前4个字节，实参在esp + 4的位置
myprint:
    mov edx, [esp + 8]      ;len
    mov ecx, [esp + 4]      ;msg
    mov ebx, 1
    mov eax, 4          ;sys_write
    int 0x80
    ret