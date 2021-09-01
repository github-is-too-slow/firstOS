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
PRIVATE int code_with_E0 = 0;
PRIVATE int shift_l;         //左shift状态
PRIVATE int shift_r;
PRIVATE int alt_l;
PRIVATE int alt_r;
PRIVATE int ctrl_l;
PRIVATE int ctrl_r;
PRIVATE int caps_lock;      //大写键
PRIVATE int num_lock;       //数字键
PRIVATE int scroll_lock;    //滚动键，在Excel中可控制上下键是滚动光标还是页面
PRIVATE int column;

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
    shift_l	= shift_r = 0;
	alt_l	= alt_r   = 0;
	ctrl_l	= ctrl_r  = 0;
    put_irq_handler(KEYBOARD_IRQ, keyboard_handler);
    enable_irq(KEYBOARD_IRQ);
}

//从键盘缓冲区中读取一个扫描码,并显示
PUBLIC void keyboard_read(){
    u8 scan_code;
    char output[2];
    int make;       //true: make code;    false: break code
    u32 key = 0; //表示按下的键值，单字节扫码就是本身，EO开头的双字节扫描码是第二个字节，其他的是自定义值
    u32 *keyrow;        //指向keymap[]的某一行
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
            //多字节扫描码，暂时不作处理，如PrintScreen占6字节
            //pass
        }else if(scan_code == 0xE0){
            //多字节扫描码，如Pause占4字节，其余占双字节
            code_with_E0 = 1;
        }else {//以下均是单字节扫描码,目前仅处理可打印字符
            //首先判断是make code 还是break code
            //FLAG_BREAK = 0x80,make code与操作为假
            make = (scan_code & FLAG_BREAK ? FALSE: TRUE);

            //先定位到keymap中的行
            //与0x7F进行与操作，避免数组越界，也可以将break code键变为make code
            keyrow = &keymap[(scan_code & 0x7F) * MAP_COLS];

            column = 0;
            if(shift_l || shift_r){//倘若按下了shift，定位到第二列
                column = 1;
            }
            if(code_with_E0){//倘若以E0开头，定位到第三列，如L Ctrl(1D) 和R Ctrl(E0, 1D),
                             //前者扫描码和后者第二字节相同，对应到同一行的不同列
                column = 2;
                code_with_E0 = 0;
            }
            //取出对应的键，单字节扫码就是本身，EO开头的双字节扫描码是第二个字节，其他的是自定义值
            key = keyrow[column];

            switch (key)
            {
            case SHIFT_L:
                shift_l = make;   //按下时置1，弹起时置0
                key = 0;
                break;
            case SHIFT_R:
                shift_r = make;
                key = 0;
                break;
            case CTRL_L:
                ctrl_l = make;   //按下时置1，弹起时置0
                key = 0;
                break;
            case CTRL_R:
                ctrl_r = make;
                key = 0;
                break;
            case ALT_L:
                alt_l = make;   //按下时置1，弹起时置0
                key = 0;
                break;
            case ALT_R:
                alt_r = make;
                key = 0;
                break;
            default:
                if(!make){//非特殊键的break code，同样不处理
                    key = 0;
                }
                break;
            }
            //如果是make code，就打印，特殊键以及非特殊键的break code先不做处理
            if(key){
                output[0] = key;
                disp_str(output);
            }
        }
        //disp_int(scan_code);
    }
}