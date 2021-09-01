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
EXTERN  PROCESS     proc_table[NR_TASKS];   //进程控制块数组
EXTERN  char        task_stack[STACK_SIZE_TOTAL];   //进程栈
EXTERN  u32         k_reenter;           //重入的中断数量