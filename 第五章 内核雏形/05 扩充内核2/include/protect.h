#ifndef _ORANGES_PROTECT_H_
#define _ORANGES_PROTECT_H_

/* 中断向量 */
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

#define	DA_386IGate		0x8E	// 386 中断门类型值

#define	SELECTOR_FLAT_C		0x08
#define	SELECTOR_KERNEL_CS	SELECTOR_FLAT_C

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
#endif