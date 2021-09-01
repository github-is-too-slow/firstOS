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

/*判断p_con是否是当前控制台*/
PUBLIC int is_current_console(CONSOLE *p_con){
    return (p_con == &console_table[nr_current_console]);
}

/*设置光标到指定位置*/
PRIVATE void set_cursor(unsigned int pos){
    disable_int();
    out_byte(CRTC_ADDR_REG, CURSOR_H);
    out_byte(CRTC_DATA_REG, (pos >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, CURSOR_L);
    out_byte(CRTC_DATA_REG, pos & 0xFF);
    enable_int();
}

/*设置显示器显示的起始位置*/
PRIVATE void set_video_start_addr(u32 addr){
    disable_int();
    out_byte(CRTC_ADDR_REG, START_ADDR_H);
    out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, START_ADDR_L);
    out_byte(CRTC_DATA_REG, addr & 0xFF);
    enable_int();
}

/*同时设置光标和起始位置*/
PRIVATE void flush(CONSOLE *p_con){
    set_cursor(p_con->cursor);
    set_video_start_addr(p_con->current_start_addr);
}

/*在指定控制台中显示字符*/
PUBLIC void out_char(CONSOLE *p_con, char ch){
    //获取显存指针,
    //并非调用汇编写的disp_str写入汇编，而是直接用c编译的代码写入
    //这样做的前提是当前的ds指向的段基址为0
    //在当前光标处显示
    //缓冲区中此时有两种可打印字符：
    //一种是普通可打印字符，一种是enter、backspace需要特殊处理的可打印字符
    u8 *p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
    switch (ch)
    {
    case '\n'://在in_process函数中放入了缓冲区,\n控制的是光标位置，跟翻屏不一样
        if(p_con->cursor < p_con->original_addr + p_con->v_mem_limit - SCREEN_WIDTH){
            p_con->cursor = p_con->original_addr + SCREEN_WIDTH * ((p_con->cursor - p_con->original_addr) / SCREEN_WIDTH + 1);
        }
        break;
    case '\b'://退格控制的同样是光标位置
        if(p_con->cursor > p_con->original_addr){
            p_con->cursor--;
            *(p_vmem - 2) = ' ';
            *(p_vmem - 1) = DEFAULT_CHAR_COLOR;
        }
        break;
    default://其余普通可打印字符
        if(p_con->cursor < p_con->original_addr + p_con->v_mem_limit - 1){
            *p_vmem++ = ch;
            *p_vmem++ = DEFAULT_CHAR_COLOR;
            //移动光标
            p_con->cursor++;
        }
        break;
    }
    //当光标移除了屏幕，触发屏幕滚动
    while(p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE){
        scroll_screen(p_con, SCR_DN);
    }
    //设置光标和起始位置
    flush(p_con);
}

/**
 * 初始化tty的控制台
 **/
PUBLIC void init_screen(TTY *p_tty){
    int nr_tty = p_tty - tty_table;
    p_tty->p_console = console_table + nr_tty;
    int v_mem_size = V_MEM_SIZE >> 1; //这里以容纳的字符数为准，因此除以2
    int con_v_mem_size = v_mem_size / NR_CONSOLES;  //一个tty占用的显存大小
    //设置起始位置，当前起始位置，所占显存大小
    p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
    p_tty->p_console->v_mem_limit = con_v_mem_size;
    p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;
    //默认光标位置
    p_tty->p_console->cursor = p_tty->p_console->original_addr;
    if(nr_tty == 0){
        //第一个tty沿用原来光标
        p_tty->p_console->cursor = disp_pos / 2;
        disp_pos = 0;       //重置显示位置从显存开始处
    }else{
        out_char(p_tty->p_console, nr_tty + '0');
        out_char(p_tty->p_console, '#');
    }
    set_cursor(p_tty->p_console->cursor);
}

/**
 * 滚屏效果
 **/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction){
    if(direction == SCR_UP){
        if(p_con->current_start_addr > p_con->original_addr){
            p_con->current_start_addr -= SCREEN_WIDTH;
        }
    }else if(direction = SCR_DN){
        if(p_con->current_start_addr + SCREEN_SIZE < p_con->original_addr + p_con->v_mem_limit){
            p_con->current_start_addr += SCREEN_WIDTH;
        }
    }else {

    }
    //设置光标和起始位置
    set_video_start_addr(p_con->current_start_addr);
    set_cursor(p_con->cursor);
}

/**
 * 切换控制台,控制台编号由组合按键alt+Fn产生
 **/
PUBLIC void select_console(int nr_console){
    if((nr_console < 0) || (nr_console >= NR_CONSOLES)){
        return;
    }
    nr_current_console = nr_console;
    //设置光标
    set_cursor(console_table[nr_console].cursor);
    //设置显示器显示的起始位置
    set_video_start_addr(console_table[nr_console].current_start_addr);
}