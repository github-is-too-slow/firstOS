#ifdef GLOBAL_VARIABLES_HERE_
#undef EXTERN   //取消EXTERN的定义
#define EXTERN
#endif

//全局变量定义在此处
EXTERN  int         disp_pos;   //屏幕上显示位置
EXTERN  u8          gdt_ptr[6];  //GDT描述符指针
EXTERN  DESCRIPTOR  gdt[GDT_SIZE];  //GDT描述符表
EXTERN  u8          idt_ptr[6];    //IDT中断描述符表指针
EXTERN  GATE        idt[IDT_SIZE];  //IDT
EXTERN  PROCESS     *p_proc_ready;    //被调度的进程表指针
EXTERN  TSS         tss;   //任务状态栈
EXTERN  PROCESS     proc_table[NR_TASKS + NR_PROCS];   //进程控制块数组
EXTERN  char        task_stack[STACK_SIZE_TOTAL];   //进程栈
EXTERN  u32         k_reenter;           //重入的中断数量
EXTERN  int         ticks;              //记录时钟中断滴答数
EXTERN  int         nr_current_console; //当前控制台的下标
extern  TASK        task_table[];
extern  TASK        user_proc_table[];
extern  irq_handler irq_table[];
extern  p_sys_call  sys_call_table[];     //系统调用子程序表
extern  TTY         tty_table[];
extern  CONSOLE     console_table[];
extern  struct dev_drv_map dd_map[];        //主设备号到驱动映射表
extern  u8          *fsbuf;             //文件系统缓冲区
EXTERN	MESSAGE			fs_msg;         //文件系统消息缓冲区
EXTERN	PROCESS     *pcaller;    //请求文件系统服务的进程
EXTERN	struct file_desc	f_desc_table[NR_FILE_DESC];
EXTERN	struct inode *		root_inode;         //根目录iNode指针
EXTERN	struct file_desc	f_desc_table[NR_FILE_DESC];   //所有进程共享的文件描述符表
EXTERN	struct inode		inode_table[NR_INODE];
EXTERN	struct super_block	super_block[NR_SUPER_BLOCK];