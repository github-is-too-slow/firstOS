%include "sconst.inc"

;导入外部函数，用于堆栈和GDT切换
extern cstart
extern exception_handle
extern spurious_irq
extern kernel_main
extern disp_color_str
extern disp_str
extern delay
;导入全局变量
extern gdt_ptr
extern idt_ptr
extern tss      ;任务状态栈
extern p_proc_ready ;指向进程的进程控制块的起始位置
extern disp_pos
extern k_reenter

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
;导出重启函数
global restart

;未初始化kernel内核堆栈
[section .bss]
StackSpace:  resb 2 * 1024
StackTop:   ;栈顶

[section .data]
clock_int_msg: db '^', 0

[section .text]
_start:
    mov esp, StackTop
    mov dword [disp_pos], 0
    sgdt [gdt_ptr]  ;保存GDTR寄存器的值
    call cstart     ;复制GDT，并令gdt_ptr指向新的GDT,设置LDT/TSS在gdt中的描述符
    lgdt [gdt_ptr]  ;加载GDT
    lidt [idt_ptr]  ;加载IDT
    jmp SELECTOR_KERNEL_CS:CSInit
CSInit:
    xor eax, eax
    mov ax, SELECTOR_TSS
    ltr ax      ;加载任务状态栈TSS
    jmp kernel_main

;当调度程序选择了一个进程执行，便开始恢复各寄存器的值
;并设置TSS中的esp0,以便下次中断到来时CPU可以自动切换堆栈到进程表中
restart:
    mov esp, [p_proc_ready]     ;令栈顶指针指向进程的PCB
    lldt [esp + P_LDT_SEL]      ;加载指定进程的局部描述表
    lea eax, [esp + P_STACKTOP]
    mov dword [tss + TSS3_S_SP0], eax      ;设置TSS中esp0的值
    ;恢复进程状态
    pop gs
    pop fs
    pop es
    pop ds
    popad    ;恢复10个通用寄存器的值
    add esp, 4  ;过滤掉retaddr
    iretd  ;会修改eflags=0x1202,打开中断

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
    ;中断门和陷阱门的区别在于由中断门向量引发的中断，在进入中断程序前关闭中断
    ;从中断程序返回前打开中断
    ;保存现场
    sub esp, 4      ;跳过retaddr
    pushad
    push ds
    push es
    push fs
    push gs
    ;由于涉及特权级变化，目前ss0/esp0均是从tss中取到的值
    ;令ds/es/ss指向相同的段
    mov dx, ss
    mov ds, dx
    mov es, dx

    ;测试逻辑
    inc byte [gs:0]

    ;中断结束标志,提前结束中断
    mov al, EOI
    out INT_M_CTL, al           ;向主i8259A芯片发送中断结束标志

    ;检测是否是新的重入中断
    inc dword [k_reenter]
    cmp dword [k_reenter], 0
    jne .re_enter

    ;只有第一个中断才能切换堆栈和开中断
    ;在进入中断处理逻辑前，esp从进程表切换到内核栈，并且开中断
    mov esp, StackTop
    sti

    push dword 0x72         ;灰底蓝字
    push clock_int_msg
    call disp_color_str
    add esp, 8

    ;延迟一段时间，确保有新中断重入
    ;push 1
    ;call delay
    ;add esp, 4

    ;在处理逻辑完毕后关中断
    cli

    ;中断逻辑处理完毕后，esp切换到进程表,
    ;在C语言中一个指针变量名实际上是存在地址的内存单元的标号
    mov esp, [p_proc_ready]
    ;在中断返回前必须设置tss中esp0的值，
    ;以便等下次从ring1->ring0切换时CPU可以将进程状态保存到对应的进程表中
    lea eax, [esp + P_STACKTOP]
    mov dword [tss + TSS3_S_SP0], eax

.re_enter:   ;重入中断直接恢复线程并返回
    dec dword [k_reenter]       ;中断数量减1
    ;恢复现场
    pop gs
    pop fs
    pop es
    pop ds
    popad
    add esp, 4      ;跳过retaddr

    iretd           ;0号时钟中断，从ring0 -> ring1返回前的栈状态是：eip/cs/eflags/esp/ss
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