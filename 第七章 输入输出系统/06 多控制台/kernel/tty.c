#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "string.h"
#include "keyboard.h"
#include "proto.h"
#include "protect.h"
#include "process.h"
#include "global.h"

#define TTY_FIRST   (tty_table)      //指向第一个TTY任务
#define TTY_END     (tty_table + NR_CONSOLES)

/*对tty缓冲区和控制台指针进行初始化*/
PRIVATE void init_tty(TTY *p_tty){
    p_tty->inbuf_count = 0;
    p_tty->p_inbuf_header = p_tty->p_inbuf_tail = p_tty->in_buf;
    init_screen(p_tty);
}

/*封装的tty读取键盘缓冲区操作，具体操作还是由keyboard_read来做*/
PRIVATE void tty_do_read(TTY *p_tty){
    if(is_current_console(p_tty->p_console)){
        //从键盘缓冲区中依次读取一个字节
        //并将解析到的四字节结果key放入tty缓冲区中
        keyboard_read(p_tty);
    }
}

/**
 * 从tty缓冲区中取出字符打印，注意此时缓冲区中的key只有可打印字符，
 * 第9bit位为0
**/
PRIVATE void tty_do_write(TTY *p_tty){
    if(p_tty->inbuf_count){//只要缓冲区中有数据就打印，不管是不是当前控制台
        char ch = *(p_tty->p_inbuf_tail); //只需要截取低8位打印即可
        p_tty->p_inbuf_tail++;
        if(p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES){
            p_tty->p_inbuf_tail = p_tty->in_buf;
        }
        p_tty->inbuf_count--;
        //调用out_char()在控制台中显示并更新光标位置
        out_char(p_tty->p_console, ch);
    }
}

PUBLIC void task_tty(){
    TTY *p_tty;
    //对键盘缓冲区和键盘中断又一次初始化
    init_keyboard();
    for(p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++){
        //对每一个tty进行初始化
        init_tty(p_tty);
    }
    //首次选中当前控制台为第一个tty
    select_console(0);
    while(1){//新进程的任务就是不停地从缓冲区读取扫描码
        //该进程在TTY数组中的TTY任务中不断循环执行它们的读写任务
        for(p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++){
            //只需要在tty_do_read中判断是否是当前控制台，
            //而在tty_do_write中判断只要对应缓冲区不为空，就做处理
            tty_do_read(p_tty);
            tty_do_write(p_tty);
        }
    }
}

/*仅将可打印字符写入tty缓冲区中*/
PUBLIC void in_process(TTY *p_tty, u32 key){
    char output[2] = {'\0', '\0'};
    //可打印字符
    if(!(key & FLAG_EXT)){
        if(p_tty->inbuf_count < TTY_IN_BYTES){
            //tty缓冲区未满
            *(p_tty->p_inbuf_header) = key;
            p_tty->p_inbuf_header++;
            if(p_tty->p_inbuf_header == p_tty->in_buf + TTY_IN_BYTES){
                p_tty->p_inbuf_header = p_tty->in_buf;
            }
            p_tty->inbuf_count++;
        }
    }else {
        //得到低9bit的原生信息,特殊键在第9位为1
        int raw_code = key & MASK_RAW;
        switch (raw_code)
        {
        case UP:
            if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)){//shift + up键
                scroll_screen(p_tty->p_console, SCR_UP);
            }
            break;
        case DOWN:
            if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)){//shift + down键
                scroll_screen(p_tty->p_console, SCR_DN);
            }
            break;
        case F1:
        case F2:
        case F3:
        case F4:
        case F5:
        case F6:
        case F7:
        case F8:
        case F9:
        case F10:
        case F11:
        case F12:
            if((key & FLAG_ALT_L) || (key & FLAG_ALT_R)){
                //根据raw_code - F1产生tty编号
                select_console(raw_code - F1);
            }
            break;
        default:
            break;
        }
    }
}