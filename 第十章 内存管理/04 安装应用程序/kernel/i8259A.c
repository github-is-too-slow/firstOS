#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "string.h"
#include "protect.h"
#include "fs.h"
#include "stdio.h"
#include "process.h"
#include "hd.h"
#include "keyboard.h"
#include "global.h"
#include "proto.h"

/**
 * i8259A中断
 **/
PUBLIC void spurious_irq(int irq){
    disp_str("spurious_irq: ");
    disp_int(irq);
    disp_str("\n");
}

/**
 * 设置具体的中断处理逻辑
 **/
PUBLIC void put_irq_handler(int irq, irq_handler handler){
    //先关闭对应中断源
    disable_irq(irq);
    //然后设置指定的具体的中断处理逻辑
    irq_table[irq] = handler;
}

/**
 * 初始化8259A中断控制器
 **/
PUBLIC void init_8259A(){
    //master 8259A, ICW1
    out_byte(INT_M_CTL, 0x11);
    //slave 8259A, ICW1
    out_byte(INT_S_CTL, 0x11);

    //master 8259A, ICW2,设置主8259A的中断向量号为0x20
    out_byte(INT_M_CTLMASK, INT_VECTOR_IRQ0);
    //slave 8259A, ICW2,设置从8259A的中断向量号为0x28
    out_byte(INT_S_CTLMASK, INT_VECTOR_IRQ8);

    //master 8259A, ICW3,IR2对应"从8259A"
    out_byte(INT_M_CTLMASK, 0x4);
    //slave 8259A, ICW3,对应主8259A的IR2
    out_byte(INT_S_CTLMASK, 0x2);

    //master 8259A, ICW4
    out_byte(INT_M_CTLMASK, 0x1);
    //slave 8259A, ICW4
    out_byte(INT_S_CTLMASK, 0x1);

    //master 8259A, OCW1
    out_byte(INT_M_CTLMASK, 0xFF);
    //slave 8259A, OCW1
    out_byte(INT_S_CTLMASK, 0xFF);

    //初始化irq_table具体中断处理逻辑函数指针数组
    for(int i = 0; i < NR_IRQ; i++){
        irq_table[i] = spurious_irq;
    }
}