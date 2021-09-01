#include "type.h"
#include "const.h"
#include "proto.h"
#include "protect.h"
#include "process.h"
#include "global.h"

/*进程切换逻辑*/
PUBLIC void clock_handler(int irq){
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