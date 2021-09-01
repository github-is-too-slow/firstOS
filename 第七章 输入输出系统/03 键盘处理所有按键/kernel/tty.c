#include "type.h"
#include "const.h"
#include "string.h"
#include "keyboard.h"
#include "proto.h"
#include "protect.h"
#include "process.h"
#include "global.h"

PUBLIC void task_tty(){
    while(1){//新进程的任务就是不停地从缓冲区读取扫描码
        keyboard_read();
    }
}

PUBLIC void in_process(u32 key){
    char output[2] = {'\0', '\0'};
    if(!(key & FLAG_EXT)){
        output[0] = key & 0xFF;
        disp_color_str(output, 0x8D);
    }
}