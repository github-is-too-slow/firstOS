#ifndef _ORANGES_TYPE_H_
#define _ORANGES_TYPE_H_
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef void (*int_handler)();  //异常处理函数指针类型
typedef void (*task_f)();        //进程体函数指针
typedef void (*irq_handler)(int irq);  //具体的中断处理逻辑函数指针，与中断处理例程不同
#endif