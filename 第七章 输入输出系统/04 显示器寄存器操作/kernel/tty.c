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
        disp_str(output);
        //显示完毕后更新光标位置,
        //disp_pos代表了下一个字符的相对位置(相对于显存基址)
        disable_int();
        out_byte(CRTC_ADDR_REG, CURSOR_H);
        out_byte(CRTC_DATA_REG, ((disp_pos / 2) >> 8) & 0xFF);
        out_byte(CRTC_ADDR_REG, CURSOR_L);
        out_byte(CRTC_DATA_REG, (disp_pos / 2) & 0xFF);
        enable_int();
    }else {
        //得到低9bit的原生信息,特殊键在第9位为1
        int raw_code = key & MASK_RAW;
        switch (raw_code)
        {
        case UP:
            if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)){//shift + up键
                disable_int();
                out_byte(CRTC_ADDR_REG, START_ADDR_H);
                out_byte(CRTC_DATA_REG, ((80 * 15) >> 8) & 0xFF);
                out_byte(CRTC_ADDR_REG, START_ADDR_L);
                out_byte(CRTC_DATA_REG, (80 * 15) & 0xFF);
                enable_int();
            }
            break;
        case DOWN:
            if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)){//shift + down键
                //暂且不处理
            }
            break;
        default:
            break;
        }
    }
}