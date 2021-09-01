#include "type.h"
#include "const.h"
#include "proto.h"
#include "protect.h"
#include "process.h"
#include "global.h"

PUBLIC void task_tty(){
    while(1){//新进程的任务就是不停地从缓冲区读取扫描码
        keyboard_read();
    }
}