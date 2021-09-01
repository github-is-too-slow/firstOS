#include "type.h"
#include "const.h"
#include "string.h"
#include "keyboard.h"
#include "keymap.h"
#include "proto.h"
#include "protect.h"
#include "process.h"
#include "global.h"

//私有键盘缓冲区
PRIVATE KB_INPUT kb_in;

/*该键盘中断处理逻辑负责从键盘端口中读取数据，并写入缓冲区中*/
PUBLIC void keyboard_handler(int irq){
    //键盘控制器(接口)8042只有将数据缓冲器的数据清空后(读取)
    //才能接受下一个输入扫描码
    u8 scan_code = in_byte(KB_DATA);
    if(kb_in.count < KB_IN_BYTES){//缓冲区未满
        *(kb_in.p_header) = scan_code;
        kb_in.p_header++; //移动到下一个空闲位置
        if(kb_in.p_header == kb_in.buf + KB_IN_BYTES){
            //当p_header移动到缓冲区末尾，将其移至开头
            kb_in.p_header = kb_in.buf;
        }
        kb_in.count++;
    }
}

//对键盘缓冲区和键盘中断作初始化
PUBLIC void init_keyboard(){
    kb_in.count = 0;
    kb_in.p_header = kb_in.p_tail = kb_in.buf;
    put_irq_handler(KEYBOARD_IRQ, keyboard_handler);
    enable_irq(KEYBOARD_IRQ);
}

//从键盘缓冲区中读取一个扫描码,并显示
PUBLIC void keyboard_read(){
    u8 scan_code;
    char output[2];
    int make;       //true: make code;    false: break code

    memset(output, 0, 2);   //output作为输出字符串，末尾0结束

    if(kb_in.count > 0){//当键盘缓冲区为空时，直接返回
        disable_int();  //对键盘缓冲区数据结构的操作应该是原子操作
        scan_code = *(kb_in.p_tail);
        kb_in.p_tail++;
        if(kb_in.p_tail == kb_in.buf + KB_IN_BYTES){
            //将p_tail移至缓冲区头
            kb_in.p_tail = kb_in.buf;
        }
        kb_in.count--;
        enable_int();   //开中断

        //下面开始解析扫描码
        if(scan_code == 0xE1){
            //双字节扫描码，暂时不作处理
            //pass
        }else if(scan_code == 0xE0){
            //双字节扫描码，暂时不作处理
            //pass
        }else {//以下均是单字节扫描码,目前仅处理可打印字符
            //首先判断是make code 还是break code
            //FLAG_BREAK = 0x80,make code与操作为假
            make = (scan_code & FLAG_BREAK ? FALSE: TRUE);
            //如果是make code，就打印，break code先不做处理
            if(make){//与0x7F进行与操作，避免数组越界，也可以将break code键变为make code
                output[0] = keymap[(scan_code & 0x7F) * MAP_COLS];
                disp_str(output);
            }
        }
        //disp_int(scan_code);
    }
}