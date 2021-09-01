#ifndef _ORANGES_PROCESS_H_
#define _ORANGES_PROCESS_H_

//动态分配进程空间
#define	PROCS_BASE		0x800000 /* 8 MB以上的给用户进程使用 */
#define	PROC_IMAGE_SIZE_DEFAULT	0x100000 /*  1 MB的用户进程空间 */
#define	PROC_ORIGIN_STACK	0x400    /*  1 KB用户进程栈 */

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

/**
 * MESSAGE mechanism消息机制
 */
struct mess1 {
	int m1i1;   //消息体1中的整数1
	int m1i2;
	int m1i3;
	int m1i4;
};
struct mess2 {
	void* m2p1;   //消息体2中的指针2
	void* m2p2;
	void* m2p3;
	void* m2p4;
};
struct mess3 {
	int	m3i1;       //消息体3中的整数1,msg.u.m3.m3i1
	int	m3i2;
	int	m3i3;
	int	m3i4;
	u64	m3l1;       //消息体1中的长整数1
	u64	m3l2;
	void*	m3p1;   //消息体1中的指针1
	void*	m3p2;
};
typedef struct {
	int source;         //消息源
	MsgType type;           //消息类型
	union {
		struct mess1 m1;
		struct mess2 m2;
		struct mess3 m3;
	} u;                //消息共用体
} MESSAGE;

/*PCB进程控制表*/
typedef struct s_proc
{
    STACK_FRAME regs;           //各种寄存器值
    u16 ldt_sel;                //一个进程独有的局部描述符表在GDT中的描述符对应的选择子
    DESCRIPTOR ldts[LDT_SIZE];  //局部描述符表
    int ticks;                  //剩余的时钟中断数
    int priority;               //ticks初始值，也代表了优先级
    u32 pid;
    char p_name[16];
    u32 p_parent;       //父进程pid
    int nr_tty;  //进程在哪个终端启动的
    //用于IPC新增加的
    int p_flags;    //进程是否可运行标志，取值：0、SENDING、RECEIVING
    MESSAGE *p_msg;     //指向消息体
    int p_recvfrom;     //进程等待接收谁的消息
    int p_sendto;       //进程要将消息发给谁
    int has_int_msg;    //是否有中断消息处理，非0表示当中断发生该任务却没有准备好处理它
    struct s_proc *q_sending;  //发消息到这个进程的进程队列，q_sending指向A,A的next_sending指向B..
    struct s_proc *next_sending;  //在q_sending发送队列中的下一个进程，
    //用于文件系统或磁盘I/O
    struct file_desc *filp[NR_FILES];       //文件描述符指针数组
    //用于内存管理
    int exit_status;
}PROCESS;

/*进程A、B堆栈大小*/
#define	STACK_SIZE_DEFAULT	0x4000 /* 16 KB */
#define STACK_SIZE_TTY		STACK_SIZE_DEFAULT
#define STACK_SIZE_SYS		STACK_SIZE_DEFAULT
#define STACK_SIZE_HD		STACK_SIZE_DEFAULT
#define STACK_SIZE_FS		STACK_SIZE_DEFAULT
#define STACK_SIZE_MM		STACK_SIZE_DEFAULT
#define STACK_SIZE_INIT		STACK_SIZE_DEFAULT
#define STACK_SIZE_TESTA	STACK_SIZE_DEFAULT
#define STACK_SIZE_TESTB	STACK_SIZE_DEFAULT
#define STACK_SIZE_TESTC	STACK_SIZE_DEFAULT
/*进程栈总大小*/
#define STACK_SIZE_TOTAL	(STACK_SIZE_TTY + \
				STACK_SIZE_SYS + \
				STACK_SIZE_HD + \
				STACK_SIZE_FS + \
				STACK_SIZE_MM + \
				STACK_SIZE_INIT + \
				STACK_SIZE_TESTA + \
				STACK_SIZE_TESTB + \
				STACK_SIZE_TESTC)

/*一个进程只需要一个进程体和堆栈就可以运行了*/
typedef struct s_task
{
    task_f initial_eip; //进程体开始执行地址
    int stacksize;      //进程栈大小
    char name[32];      //进程名
}TASK;
#endif