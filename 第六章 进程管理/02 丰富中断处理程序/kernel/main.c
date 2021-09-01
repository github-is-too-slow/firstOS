#include "type.h"
#include "const.h"
#include "string.h"
#include "proto.h"
#include "protect.h"
#include "process.h"
#include "global.h"

PUBLIC void restart();

PUBLIC void TestA(){
    int i = 0;
    while(1){
        disp_str("A");
        disp_int(i++);
        disp_str(".");
        delay(1);
    }
}

PUBLIC int kernel_main(){
    disp_str("-----\"kernel_main\" begins-----\n");

    //初始化进程表，为进程第一次执行做准备
    PROCESS *p_proc = proc_table;
    //设置LDT在GDT中描述符对应的选择子
    proc_table->ldt_sel = SELECTOR_LDT_FIRST;
    //设置在进程表中的LDT，包含两个局部描述符
    memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR));
    p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;     //改变DPL特权级
    memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
    p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
    p_proc->regs.cs = (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | PRIVILEGE_TASK;
    p_proc->regs.ds = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | PRIVILEGE_TASK;
    p_proc->regs.es = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | PRIVILEGE_TASK;
    p_proc->regs.fs = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | PRIVILEGE_TASK;
    p_proc->regs.ss = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | PRIVILEGE_TASK;
    p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | PRIVILEGE_TASK;
    p_proc->regs.eip = (u32)TestA;
    p_proc->regs.esp = (u32)task_stack + STACK_SIZE_TOTAL;
    p_proc->regs.eflags = 0x1202;   //IF=1, IOPL=1,当主动执行时钟中断iretd后中断打开

    p_proc_ready = proc_table;
    restart();
    while(1){}
}