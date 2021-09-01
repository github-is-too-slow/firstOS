#ifndef _ORANGES_PROTECT_H_
#define _ORANGES_PROTECT_H_

#define	reassembly(high, high_shift, mid, mid_shift, low)	\
	(((high) << (high_shift)) +				\
	 ((mid)  << (mid_shift)) +				\
	 (low))


/* 异常中断向量 */
#define	INT_VECTOR_DIVIDE		0x0
#define	INT_VECTOR_DEBUG		0x1
#define	INT_VECTOR_NMI			0x2
#define	INT_VECTOR_BREAKPOINT		0x3
#define	INT_VECTOR_OVERFLOW		0x4
#define	INT_VECTOR_BOUNDS		0x5
#define	INT_VECTOR_INVAL_OP		0x6
#define	INT_VECTOR_COPROC_NOT		0x7
#define	INT_VECTOR_DOUBLE_FAULT		0x8
#define	INT_VECTOR_COPROC_SEG		0x9
#define	INT_VECTOR_INVAL_TSS		0xA
#define	INT_VECTOR_SEG_NOT		0xB
#define	INT_VECTOR_STACK_FAULT		0xC
#define	INT_VECTOR_PROTECTION		0xD
#define	INT_VECTOR_PAGE_FAULT		0xE
#define	INT_VECTOR_COPROC_ERR		0x10

/*系统调用中断号*/
#define INT_VECTOR_SYS_CALL 0x90

/*中断向量号*/
#define INT_VECTOR_IRQ0     0x20
#define INT_VECTOR_IRQ8     0x28

/*PIT 8253可编程间隔计数器相关宏定义*/
#define TIMER0          0x40        //Counter0计数器对应的端口号
#define TIMER_MODE      0x43        //模式控制寄存器
#define RATE_GENERATOR  0x34        //00-11-010-0
                                    //Counter0-LSB then MSB-rate generator-binary
#define TIMER_FREQ      1193182L    //时钟芯片的时钟频率 1193182/HZ
#define INT_FREQ        100         //时钟中断频率  100/HZ

/*描述符对应的索引*/
#define INDEX_DUMMY 0
#define INDEX_FLAT_C 1
#define INDEX_FLAT_RW 2
#define INDEX_VIDEO 3
#define INDEX_TSS 4
#define INDEX_LDT_FIRST 5

/* descriptor indices in LDT */
#define INDEX_LDT_C             0
#define INDEX_LDT_RW            1

/*一些选择子*/
#define	SELECTOR_FLAT_C		0x08
#define SELECTOR_FLAT_RW    0x10
#define SELECTOR_VIDEO      0x18 + 3   //RPL=3
#define SELECTOR_TSS        0x20        //TSS选择子
#define SELECTOR_LDT_FIRST  0x28

#define	SELECTOR_KERNEL_CS	SELECTOR_FLAT_C
#define SELECTOR_KERNEL_DS  SELECTOR_FLAT_RW
#define SELECTOR_KERNEL_GS  SELECTOR_VIDEO

/*选择子属性值SA_*/
#define SA_RPL_MASK 0xFFFC
#define SA_RPL0 0
#define SA_RPL1 1
#define SA_RPL2 2
#define SA_RPL3 3
#define SA_TI_MASK 0xFFFB
#define SA_TIG 0        //选择域
#define SA_TIL 4

//描述符属性
#define	DA_32			0x4000	/* 32 位段				*/
#define	DA_LIMIT_4K		0x8000	/* 段界限粒度为 4K 字节			*/
#define	LIMIT_4K_SHIFT		  12
#define	DA_DPL0			0x00	/* DPL = 0				*/
#define	DA_DPL1			0x20	/* DPL = 1				*/
#define	DA_DPL2			0x40	/* DPL = 2				*/
#define	DA_DPL3			0x60	/* DPL = 3				*/
/* 存储段描述符类型值说明 */
#define	DA_DR			0x90	/* 存在的只读数据段类型值		*/
#define	DA_DRW			0x92	/* 存在的可读写数据段属性值		*/
#define	DA_DRWA			0x93	/* 存在的已访问可读写数据段类型值	*/
#define	DA_C			0x98	/* 存在的只执行代码段属性值		*/
#define	DA_CR			0x9A	/* 存在的可执行可读代码段属性值		*/
#define	DA_CCO			0x9C	/* 存在的只执行一致代码段属性值		*/
#define	DA_CCOR			0x9E	/* 存在的可执行可读一致代码段属性值	*/
#define	DA_386IGate		0x8E	// 386 中断门类型值
#define	DA_386TSS		0x89	//可用 386 任务状态段类型值
#define	DA_LDT			0x82	//局部描述符表段类型值

/*GDT和IDT描述符个数*/
#define GDT_SIZE 128
#define IDT_SIZE 256
#define LDT_SIZE 2

/* 描述符权限 */
#define	PRIVILEGE_KRNL	0
#define	PRIVILEGE_TASK	1
#define	PRIVILEGE_USER	3
/* 选择子权限*/
#define	RPL_KRNL	SA_RPL0
#define	RPL_TASK	SA_RPL1
#define	RPL_USER	SA_RPL3

/*8259A中断控制器的端口号*/
#define INT_M_CTL 0x20
#define INT_M_CTLMASK 0x21
#define INT_S_CTL 0xA0
#define INT_S_CTLMASK 0xA1

/*由段基址和偏移地址求物理地址*/
#define vir2phys(seg_base, vir) ((u32)(((u32)(seg_base)) + ((u32)(vir))))

/*数据段或代码段/系统段描述符，用于描述一个段*/
typedef struct s_descriptor{       //共8个字节
    u16 limit_low;      //段限制的低16位
    u16 base_low;       //段基址的低16位
    u8  base_mid;       //段基址的中8位
    u8  attr1;          //P(1)/DPL(2)/DT(1)/TYPE(4)
    u8  limit_high_attr2;   //G(1)/D(1)/O(1)/AVL(1)/LimitHigh(4)
    u8  base_high;      //段基址的高8位
}DESCRIPTOR;

/*门描述符，用于描述一个段中特定的执行代码,可以实现真正的特权级切换*/
typedef struct s_gate
{
    u16 offset_low;     //offset偏移地址低16位
    u16 selector;       //GDT选择子Selector
    u8 dcount;          //该字段只在调用门描述符中有效
    u8 attr;            //P(1)/DPL(2)/DT(1)/TYPE(4)
    u16 offset_high;    //offset偏移地址高16位
}GATE;

typedef struct s_tss
{
    u32 backlink;
    u32 esp0;
    u32 ss0;
    u32 esp1;
    u32 ss1;
    u32 esp2;
    u32 ss2;
    u32 cr3;
    u32 eip;
    u32 eflags;
    u32 eax;
    u32 ecx;
    u32 edx;
    u32 ebx;
    u32 esp;
    u32 ebp;
    u32 esi;
    u32 edi;
    u32 es;
    u32 cs;
    u32 ss;
    u32 ds;
    u32 fs;
    u32 gs;
    u32 ldt;
    u16 trap;
    u16 iobase;   //I/O位图基址大于或等于TSS段界限，就表示没有I/O许可位图
}TSS;
#endif