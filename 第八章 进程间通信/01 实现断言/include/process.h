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
    int nr_tty;  //进程在哪个终端启动的
    //用于IPC新增加的
    int p_flags;    //进程是否可运行标志，取值：0、SENDING、RECEIVING
    MESSAGE *p_msg;     //进程消息队列
    int p_recvfrom;     //进程等待接收谁的消息
    int p_sendto;       //进程要将消息发给谁
    int has_int_msg;    //是否有中断消息处理，非0表示当中断发生该任务却没有准备好处理它
    PROCESS *q_sending;  //发消息到这个进程的进程队列，q_sending指向A,A的next_sending指向B..
    PROCESS *next_sending;  //在q_sending发送队列中的下一个进程，
}PROCESS;

/*任务数量和用户进程数量*/
#define NR_TASKS 1
#define NR_PROCS 3

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