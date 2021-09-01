/**
 * 进程涉及的4个方面：
 * 1. 进程体：如何加载到内存中
 * 2. 进程表：保存进程的状态信息：寄存器、ldt选择子、ldt表
 * 3. GDT：进程ldt表在GDT中的描述符
 * 4. TSS：一个进程任务对应一个TSS，保存任务切换时ss0/esp0堆栈指针
 **/

#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "string.h"
#include "protect.h"
#include "process.h"
#include "global.h"
#include "proto.h"

PUBLIC int get_ticks(){
    MESSAGE msg;
    reset_msg(&msg);
    msg.type = GET_TICKS;
    send_recv(BOTH, TASK_SYS, &msg);
    return msg.RETVAL;
}

PUBLIC void TestA(){
    int i = 10;
    // assert(1 + 1 != 2);
    while(1){
        // disp_color_str("A.", BRIGHT | MAKE_COLOR(BLACK,RED));
        // disp_int(get_ticks());
        //使用系统调用printf和get_ticks
        printf("<ticks: %d> \n", get_ticks());
        milli_delay(5000);  //20次时钟中断
        // i--;
    }
    while(1){}
}

PUBLIC void TestB(){
    int i = 10;
    while(i){
        // disp_color_str("B.", BRIGHT | MAKE_COLOR(BLACK,RED));
        // disp_int(get_ticks());
        printf("B ");
        milli_delay(1000);  //20次时钟中断
        i--;
    }
    while(1){}
}

PUBLIC void TestC(){
    int i = 10;
    while(i){
        // disp_color_str("C.", BRIGHT | MAKE_COLOR(BLACK,RED));
        // disp_int(get_ticks());
        printf("C ");
        milli_delay(1000);  //20次时钟中断
        i--;
    }
    while(1){}
}

PUBLIC int kernel_main(){
    disp_str("-----\"kernel_main\" begins-----\n");

    //根据task_table中进程信息初始化进程表
    TASK        *p_task;
    PROCESS     *p_proc         = proc_table;
    char        *p_task_stack   = task_stack + STACK_SIZE_TOTAL;
    u16         selector_ldt    = SELECTOR_LDT_FIRST;
    u8          privilege;
    u8          rpl;
    int         eflags;
    int prio;   //初始优先级
    for(int i = 0; i < NR_TASKS + NR_PROCS; i++){
        if(i < NR_TASKS){
            p_task = task_table + i;
            privilege = PRIVILEGE_TASK;
            rpl = RPL_TASK;
            eflags = 0x1202;  /*IF = 1  IOPL = 1, bit 2总是1*/
            prio = 15;
        }else {
            p_task = user_proc_table + (i - NR_TASKS);
            privilege = PRIVILEGE_USER;
            rpl = RPL_USER;
            eflags = 0x202;  /*IF = 1  IOPL = 0, bit 2总是1,用户进程没有io权限*/
            prio = 5;
        }
        //复制进程名
        strcpy(p_proc->p_name, p_task->name);
        //分配进程id
        p_proc->pid = i;
        //ldt选择子
        p_proc->ldt_sel = selector_ldt;
        //设置在进程表中的LDT，包含两个局部描述符
        memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR));
        p_proc->ldts[0].attr1 = DA_C | privilege << 5;     //改变DPL特权级
        memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
        p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
        //初始化进程状态
        p_proc->regs.cs = (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.ds = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.es = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.fs = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.ss = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
        //指向对应进程体入口
        p_proc->regs.eip = (u32)p_task->initial_eip;
        p_proc->regs.esp = (u32)p_task_stack;
        p_proc->regs.eflags = eflags;   //IF=1, IOPL=1,当主动执行时钟中断iretd后中断打开
        p_proc->nr_tty = 0;         //默认在0号终端启动进程

        p_proc->p_flags = 0;
        p_proc->p_msg = 0;
        p_proc->p_recvfrom = NO_TASK;
        p_proc->p_sendto = NO_TASK;
        p_proc->has_int_msg = 0;
        p_proc->q_sending = 0;
        p_proc->next_sending = 0;

        p_proc->ticks = p_proc->priority = prio;
        //转向下一个进程
        p_task_stack -= p_task->stacksize;
        p_proc++;
        p_task++;
        selector_ldt += 1 << 3;     //选择子加8
    }
    //改变用户进程的绑定终端
    proc_table[NR_TASKS + 0].nr_tty = 0;
    proc_table[NR_TASKS + 1].nr_tty = 1;
    proc_table[NR_TASKS + 2].nr_tty = 1;
    //初始化时钟中断芯片PIT、处理逻辑，并打开时钟中断
    init_clock();
    //设置键盘中断处理逻辑，并打开键盘中断
    init_keyboard();
    //初始化各种全局变量
    ticks = 0;
    k_reenter = 0;         //初始重入中断数量为0
    p_proc_ready = proc_table;
    restart();
    while(1){}
}

/*****************************************************************************
 *                                panic严重错误，直接使CPU停止运行
 * panic是一个函数形式存在，而assert是以一个宏形式存在
 * panic只会用在ring1任务或ring0中发生致命错误时才是用
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)(&fmt) + 4);

    //panic自身的不定参数先嵌入一次
	i = vsprintf(buf, fmt, arg);

    //printf不定参数再嵌入一次输出
	printf("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}