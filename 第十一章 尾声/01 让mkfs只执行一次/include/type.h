#ifndef _ORANGES_TYPE_H_
#define _ORANGES_TYPE_H_
typedef	unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef void (*int_handler)();  //异常处理函数指针类型
typedef void (*task_f)();        //进程体函数指针
typedef void (*irq_handler)(int irq);  //具体的中断处理逻辑函数指针，与中断处理例程不同
typedef void *p_sys_call;          //任意类型的系统调用函数指针
typedef char *va_list;              //不定参数在栈中的指针类型
#endif