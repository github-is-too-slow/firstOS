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

/*在指定控制台中显示字符,暂时这样实现*/
PUBLIC void out_char(CONSOLE *p_con, char ch){
    //获取显存指针,
    //并非调用汇编写的disp_str写入汇编，而是直接用c编译的代码写入
    //这样做的前提是当前的ds指向的段基址为0
    u8 *p_vmem = (u8*)(V_MEM_BASE + disp_pos);
    *p_vmem++ = ch;
    *p_vmem++ = DEFAULT_CHAR_COLOR;
    disp_pos += 2;
    //显示完毕后设置光标到下一个位置
    set_cursor(disp_pos / 2);
}