#ifndef _ORANGES_CONST_H_
#define _ORANGES_CONST_H_

/*函数类型*/
#define PUBLIC
#define PRIVATE static  //限制全局变量或函数的作用域

/*引用变量*/
#define EXTERN extern

/* 权限 */
#define	PRIVILEGE_KRNL	0
#define	PRIVILEGE_TASK	1
#define	PRIVILEGE_USER	3

/*GDT和IDT描述符个数*/
#define GDT_SIZE 128
#define IDT_SIZE 256

/*8259A中断控制器的端口号*/
#define INT_M_CTL 0x20
#define INT_M_CTLMASK 0x21
#define INT_S_CTL 0xA0
#define INT_S_CTLMASK 0xA1

/*中断向量号*/
#define INT_VECTOR_IRQ0     0x20
#define INT_VECTOR_IRQ8     0x28
#endif