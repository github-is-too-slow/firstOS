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

/*进程切换逻辑*/
/*此处采用轮询调度策略*/
PUBLIC void clock_handler(int irq){
    ticks++;   //时钟中断次数加1
    p_proc_ready->ticks--;   //当前进程的循环时钟中断减1

    if(k_reenter != 0){
        //重入时钟中断
        return;
    }
    //在一个进程的ticks还没有变成0之前，
    //其他进程不会有机会获得执行
    if(p_proc_ready->ticks > 0){
        return;
    }
    //根据优先级调度，进行进程切换
    schedule();
}

/*对8353 PIT芯片作初始化，设置时钟中断处理逻辑，并打开时钟中断*/
PUBLIC void init_clock(){
    //初始化8253时钟芯片的时钟中断频率(通过counter0计数器)
    out_byte(TIMER_MODE, RATE_GENERATOR);
    //先设置低位8字节
    out_byte(TIMER0, (u8)(TIMER_FREQ / INT_FREQ));
    //再设置高位8字节
    out_byte(TIMER0, (u8)((TIMER_FREQ / INT_FREQ) >> 8));
    put_irq_handler(CLOCK_IRQ, clock_handler);
    enable_irq(CLOCK_IRQ);
}

/**
 * milli_sec: 指定延迟的毫秒数
 * 不太精确的10毫秒级延迟函数
 **/
PUBLIC void milli_delay(int milli_sec){
    int t = get_ticks();  //获取处理时钟中断数
    //1000 / INT_FREQ 表示相邻两个时钟中断所占得毫秒数
    while(((get_ticks() -t) * 1000 / INT_FREQ) < milli_sec){}
}