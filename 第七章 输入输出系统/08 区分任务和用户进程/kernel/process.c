#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "protect.h"
#include "proto.h"
#include "process.h"
#include "global.h"

/*返回时钟中断数*/
PUBLIC int sys_get_ticks(){
    return ticks;
}

/*进程调度算法*/
PUBLIC void schedule(){
    PROCESS *p;
    int greatest_ticks = 0;
    while(!greatest_ticks){
        for(p = proc_table; p < proc_table + NR_TASKS + NR_PROCS; p++){
            if(p->ticks > greatest_ticks){
                greatest_ticks = p->ticks;
                //切换进程
                p_proc_ready = p;
            }
        }
        if(!greatest_ticks){
            for(p = proc_table; p < proc_table + NR_TASKS; p++){
                p->ticks = p->priority;
            }
        }
    }
}