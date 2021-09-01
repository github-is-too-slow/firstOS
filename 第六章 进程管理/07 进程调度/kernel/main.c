/**
 * 进程涉及的4个方面：
 * 1. 进程体：如何加载到内存中
 * 2. 进程表：保存进程的状态信息：寄存器、ldt选择子、ldt表
 * 3. GDT：进程ldt表在GDT中的描述符
 * 4. TSS：一个进程任务对应一个TSS，保存任务切换时ss0/esp0堆栈指针
 **/

#include "type.h"
#include "const.h"
#include "string.h"
#include "proto.h"
#include "protect.h"
#include "process.h"
#include "global.h"

PUBLIC void TestA(){
    int i = 0;
    while(1){
        disp_color_str("A.", BRIGHT | MAKE_COLOR(BLACK,RED));
        disp_int(get_ticks());
        milli_delay(200);  //20次时钟中断
    }
}

PUBLIC void TestB(){
    int i = 0x1000;
    while(1){
        disp_color_str("B.", BRIGHT | MAKE_COLOR(BLACK,RED));
        disp_int(get_ticks());
        milli_delay(200);  //20次时钟中断
    }
}

PUBLIC void TestC(){
    int i = 0x2000;
    while(1){
        disp_color_str("C.", BRIGHT | MAKE_COLOR(BLACK,RED));
        disp_int(get_ticks());
        milli_delay(200);  //20次时钟中断
    }
}

PUBLIC int kernel_main(){
    disp_str("-----\"kernel_main\" begins-----\n");

    //根据task_table中进程信息初始化进程表
    TASK        *p_task         = task_table;
    PROCESS     *p_proc         = proc_table;
    char        *p_task_stack   = task_stack + STACK_SIZE_TOTAL;
    u16         selector_ldt    = SELECTOR_LDT_FIRST;
    for(int i = 0; i < NR_TASKS; i++){
        //复制进程名
        strcpy(p_proc->p_name, p_task->name);
        //分配进程id
        p_proc->pid = i;
        //ldt选择子
        p_proc->ldt_sel = selector_ldt;
        //设置在进程表中的LDT，包含两个局部描述符
        memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR));
        p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;     //改变DPL特权级
        memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
        p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
        //初始化进程状态
        p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
        p_proc->regs.cs = (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | PRIVILEGE_TASK;
        p_proc->regs.ds = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | PRIVILEGE_TASK;
        p_proc->regs.es = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | PRIVILEGE_TASK;
        p_proc->regs.fs = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | PRIVILEGE_TASK;
        p_proc->regs.ss = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | PRIVILEGE_TASK;
        p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | PRIVILEGE_TASK;
        //指向对应进程体入口
        p_proc->regs.eip = (u32)p_task->initial_eip;
        p_proc->regs.esp = (u32)p_task_stack;
        p_proc->regs.eflags = 0x1202;   //IF=1, IOPL=1,当主动执行时钟中断iretd后中断打开
        //转向下一个进程
        p_task_stack -= p_task->stacksize;
        p_proc++;
        p_task++;
        selector_ldt += 1 << 3;     //选择子加8
    }
    //进程数暂时为3，初始化它们的优先级和经历的时钟中断数
    proc_table[0].ticks = proc_table[0].priority = 150;
    proc_table[1].ticks = proc_table[1].priority = 100;
    proc_table[2].ticks = proc_table[2].priority = 50;

    //初始化8253时钟芯片的时钟中断频率(通过counter0计数器)
    out_byte(TIMER_MODE, RATE_GENERATOR);
    //先设置低位8字节
    out_byte(TIMER0, (u8)(TIMER_FREQ / INT_FREQ));
    //再设置高位8字节
    out_byte(TIMER0, (u8)((TIMER_FREQ / INT_FREQ) >> 8));
    //设置i8259A中断芯片0号时钟中断处理逻辑，并打开时钟中断
    put_irq_handler(CLOCK_IRQ, clock_handler);
    enable_irq(CLOCK_IRQ);
    //初始化各种全局变量
    ticks = 0;
    k_reenter = 0;         //初始重入中断数量为0
    p_proc_ready = proc_table;
    //延迟5秒钟清屏，并进入进程调度，因为milli_delay会有时钟中断重入，
    //因此进程A第一次被过滤掉了
    milli_delay(5000);
    clearDisplay();
    restart();
    while(1){}
}