#include "type.h"
#include "const.h"
#include "proto.h"
#include "protect.h"
#include "process.h"
#include "global.h"

/*进程切换逻辑*/
PUBLIC void clock_handler(int irq){
    ticks++;   //时钟中断次数加1
    disp_color_str("#", 0x74);

    if(k_reenter != 0){
        //重入中断
        disp_color_str("!", 0x72);
        return;
    }
    p_proc_ready++;
    if(p_proc_ready >= proc_table + NR_TASKS){
        p_proc_ready = proc_table;  //从第一个进程重新开始调度
    }
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