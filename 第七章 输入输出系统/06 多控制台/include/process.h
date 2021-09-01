#ifndef _ORANGES_PROCESS_H_
#define _ORANGES_PROCESS_H_
/*栈帧结构，用于保存各种寄存器的值，在PCB最顶层*/
typedef struct s_stackframe
{
    u32     gs;
    u32     fs;
    u32     es;
    u32     ds;
    u32     edi;
    u32     esi;
    u32     ebp;
    u32     kernel_esp;     //内核栈，当popad时会自动忽略，仅仅预留一个位置
    u32     ebx;
    u32     edx;
    u32     ecx;
    u32     eax;
    u32     retaddr;        //返回到kernel.asm的地址，仍然作为预留位置
    u32     eip;            //以下五个寄存器由CPU中断隐指令自动保存
    u32     cs;
    u32     eflags;
    u32     esp;
    u32     ss;
}STACK_FRAME;

/*PCB进程控制表*/
typedef struct s_proc
{
    STACK_FRAME regs;           //各种寄存器值
    u16 ldt_sel;                //一个进程独有的局部描述符表在GDT中的描述符对应的选择子
    DESCRIPTOR ldts[LDT_SIZE];  //局部描述符表
    int ticks;     //剩余的时钟中断数
    int priority;   //ticks初始值，也代表了优先级
    u32 pid;
    char p_name[16];
}PROCESS;

/*最大允许进程数*/
#define NR_TASKS 4
/*进程A、B堆栈大小*/
#define STACK_SIZE_TESTA 0x8000     //32k大小
#define STACK_SIZE_TESTB 0x8000
#define STACK_SIZE_TESTC 0x8000
#define STACK_SIZE_TASK_TTY 0x8000
/*进程栈总大小*/
#define STACK_SIZE_TOTAL (STACK_SIZE_TESTA + STACK_SIZE_TESTB + STACK_SIZE_TESTC + STACK_SIZE_TASK_TTY)

/*一个进程只需要一个进程体和堆栈就可以运行了*/
typedef struct s_task
{
    task_f initial_eip; //进程体开始执行地址
    int stacksize;      //进程栈大小
    char name[32];      //进程名
}TASK;

#endif