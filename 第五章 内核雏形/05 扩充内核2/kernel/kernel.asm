SELECTOR_KERNEL_CS equ 8        ;其实指向了LABEL_DESC_FLAT_C描述符
INT_VECTOR_IRQ0 equ 0x20        ;中断向量号
INT_VECTOR_IRQ8 equ 0x28

;导入外部函数，用于堆栈和GDT切换
extern cstart
extern exception_handle
extern spurious_irq
;导入全局变量
extern gdt_ptr
extern idt_ptr
extern disp_pos

;导出入口函数
global _start
;导出异常处理函数
global divide_error
global single_step_exception
global nmi
global breakpoint_exception
global overflow
global bounds_check
global inval_opcode
global copr_not_available
global double_fault
global copr_seg_overrun
global inval_tss
global segment_not_present
global stack_exception
global general_protection
global page_fault
global copr_error
;导出中断处理函数
global hwint00
global hwint01
global hwint02
global hwint03
global hwint04
global hwint05
global hwint06
global hwint07
global hwint08
global hwint09
global hwint10
global hwint11
global hwint12
global hwint13
global hwint14
global hwint15

;未初始化kernel堆栈
[section .bss]
StackSpace:  resb 2 * 1024
StackTop:   ;栈顶

[section .text]
_start:
    mov esp, StackTop
    mov dword [disp_pos], 0
    sgdt [gdt_ptr]  ;保存GDTR寄存器的值
    call cstart     ;复制GDT，并令gdt_ptr指向新的GDT
    lgdt [gdt_ptr]
    lidt [idt_ptr]
    jmp SELECTOR_KERNEL_CS:CSInit
CSInit:
    ;push 0
    ;popfd       ;更新eflags标志寄存器的值
    ;ud2         ;Intel提供的产生#UD INVALID OPCODE异常的指令
    ;jmp 0x40:0
    sti   ;开中断，置IF=1
    hlt         ;使程序停止运行，处理器进入暂停状态，不执行任何操作，不影响标志。
                ;当复位（外语：RESET）线上有复位信号、CPU响应非屏蔽中断、CPU响应可屏蔽中断
                ;3种情况之一时，CPU脱离暂停状态，执行HLT的下一条指令。
;0除法异常
divide_error:
    push 0xFFFFFFFF     ;没有错误码
    push 0              ;中断向量号0
    jmp exception
;1单步中断
single_step_exception:
    push 0xFFFFFFFF     ;没有错误码
    push 1              ;中断向量号1
    jmp exception
;2非屏蔽中断
nmi:
    push 0xFFFFFFFF     ;没有错误码
    push 2              ;中断向量号2
    jmp exception
;3断点调试
breakpoint_exception:
    push 0xFFFFFFFF     ;没有错误码
    push 3              ;中断向量号3
    jmp exception
;4溢出
overflow:
    push 0xFFFFFFFF     ;没有错误码
    push 4              ;中断向量号4
    jmp exception
;5
bounds_check:
    push 0xFFFFFFFF     ;没有错误码
    push 5              ;中断向量号5
    jmp exception
;6
inval_opcode:
    push 0xFFFFFFFF     ;没有错误码
    push 6              ;中断向量号6
    jmp exception
;7
copr_not_available:
    push 0xFFFFFFFF     ;没有错误码
    push 7              ;中断向量号7
    jmp exception
;8
double_fault:
    ;有错误码
    push 8              ;中断向量号8
    jmp exception
;9
copr_seg_overrun:
    push 0xFFFFFFFF     ;没有错误码
    push 9              ;中断向量号9
    jmp exception
;10无效tss
inval_tss:
    ;有错误码
    push 10              ;中断向量号10
    jmp exception
;11
segment_not_present:
    ;有错误码
    push 11              ;中断向量号11
    jmp exception
;12
stack_exception:
    ;有错误码
    push 12              ;中断向量号12
    jmp exception
;13常规保护错误
general_protection:
    ;有错误码
    push 13              ;中断向量号13
    jmp exception
;14
page_fault:
    ;有错误码
    push 14              ;中断向量号14
    jmp exception
;16
copr_error:
    push 0xFFFFFFFF      ;没有错误码
    push 16              ;中断向量号16
    jmp exception
exception:
    call exception_handle
    add esp, 8  ;栈顶指向eip/cs/eflags
    hlt

hwint00:
    push INT_VECTOR_IRQ0 + 0
    jmp interrupt
hwint01:
    push INT_VECTOR_IRQ0 + 1
    jmp interrupt
hwint02:
    push INT_VECTOR_IRQ0 + 2
    jmp interrupt
hwint03:
    push INT_VECTOR_IRQ0 + 3
    jmp interrupt
hwint04:
    push INT_VECTOR_IRQ0 + 4
    jmp interrupt
hwint05:
    push INT_VECTOR_IRQ0 + 5
    jmp interrupt
hwint06:
    push INT_VECTOR_IRQ0 + 6
    jmp interrupt
hwint07:
    push INT_VECTOR_IRQ0 + 7
    jmp interrupt
hwint08:
    push INT_VECTOR_IRQ8 + 0
    jmp interrupt
hwint09:
    push INT_VECTOR_IRQ8 + 1
    jmp interrupt
hwint10:
    push INT_VECTOR_IRQ8 + 2
    jmp interrupt
hwint11:
    push INT_VECTOR_IRQ8 + 3
    jmp interrupt
hwint12:
    push INT_VECTOR_IRQ8 + 4
    jmp interrupt
hwint13:
    push INT_VECTOR_IRQ8 + 5
    jmp interrupt
hwint14:
    push INT_VECTOR_IRQ8 + 6
    jmp interrupt
hwint15:
    push INT_VECTOR_IRQ8 + 7
    jmp interrupt
interrupt:
    call spurious_irq
    add esp, 4
    hlt