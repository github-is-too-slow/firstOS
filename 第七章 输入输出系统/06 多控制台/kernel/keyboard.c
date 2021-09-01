#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "string.h"
#include "keyboard.h"
#include "keymap.h"
#include "proto.h"
#include "protect.h"
#include "process.h"
#include "global.h"

//私有键盘缓冲区
PRIVATE KB_INPUT kb_in;
PRIVATE int code_with_E0;
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

PRIVATE u8 get_byte_from_kbuf();

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

/**
 * 该函数的职责只是从键盘缓冲区中读取一个扫描码
 * 如何处理交给上层软件
 **/
PUBLIC void keyboard_read(TTY *p_tty){
    u8 scan_code;
    char output[2];
    int make;       //true: make code;    false: break code
    u32 key = 0; //表示按下的键值，单字节扫码就是本身，EO开头的双字节扫描码是第二个字节，其他的是自定义值
    u32 *keyrow;        //指向keymap[]的某一行
    memset(output, 0, 2);   //output作为输出字符串，末尾0结束

    if(kb_in.count > 0){//当键盘缓冲区为空时，直接返回
        code_with_E0 = 0;       //每一个相对完整的读入，均置0
        //在一个原子操作中从缓冲区中读入一个字符
        scan_code = get_byte_from_kbuf();

        //下面开始解析扫描码
        if(scan_code == 0xE1){
            //多字节扫描码，如pause占6字节
            int i;
            u8 pause_scode[] = {0xE1, 0x1D, 0x45,
                                0xE1, 0x9D, 0xC5};
            int is_pause = 1;   //默认是pause键
            for(i = 1; i < 6; i++){
                if(get_byte_from_kbuf() != pause_scode[i]){
                    is_pause = 0;
                    break;
                }
            }
            if(is_pause){//按键是pause暂停键，只有make code
                key = PAUSEBREAK;
            }
        }else if(scan_code == 0xE0){
            //多字节扫描码，如PrintScreen占4字节，其余占双字节
            scan_code = get_byte_from_kbuf(); //获取0xE0后的下一个字节
            //PrintScreen按下
            if(scan_code == 0x2A){
                if(get_byte_from_kbuf() == 0xE0){
                    if(get_byte_from_kbuf() == 0x37){
                        key = PRINTSCREEN;
                        make = 1;       //按下状态
                    }
                }
            }
            //PrintScreen松开
            if(scan_code == 0xB7){
                if(get_byte_from_kbuf() == 0xE0){
                    if(get_byte_from_kbuf() == 0xAA){
                        key = PRINTSCREEN;
                        make = 0;       //松开状态
                    }
                }
            }
            //其余以0xE0开头的双字节
            if(key == 0){
                code_with_E0 = 1;
            }
        }
        //非pause或printscreen的双字节(scan_code目前指向第二个字节)或单字节扫描码
        if((key != PAUSEBREAK) && (key != PRINTSCREEN)){
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
                break;
            case SHIFT_R:
                shift_r = make;
                break;
            case CTRL_L:
                ctrl_l = make;   //按下时置1，弹起时置0
                break;
            case CTRL_R:
                ctrl_r = make;
                break;
            case ALT_L:
                alt_l = make;   //按下时置1，弹起时置0
                break;
            case ALT_R:
                alt_r = make;
                break;
            default:
                break;
            }
            //break code先不做处理
            if(make){
                //返回值key是4字节，32位信息位共包括：从1计算
                //双字节扫码的第二字节或单字节扫描码(占低8位)
                //是否是特殊键，第9bit位
                //shift_l状态信息，第10bit位
                //shift_r状态信息，第11bit位
                //ctrl_l状态信息，第12bit位
                //ctrl_r状态信息，第13bit位
                //alt_l状态信息，第14bit位
                //alt_r状态信息，第15bit位
                key |= shift_l ? FLAG_SHIFT_L: 0;
                key |= shift_r ? FLAG_SHIFT_R: 0;
                key |= ctrl_l ? FLAG_CTRL_L: 0;
                key |= ctrl_r ? FLAG_CTRL_R: 0;
                key |= alt_l ? FLAG_ALT_L: 0;
                key |= alt_r ? FLAG_ALT_R: 0;

                //上层软件负责具体处理
                in_process(p_tty, key);
            }
        }
    }
}

/*以原子操作从键盘缓冲区中读入下一个字符*/
PRIVATE u8 get_byte_from_kbuf(){
    u8 scan_code;
    while(kb_in.count <= 0){//在此处等待下一个字符的到来
    }
    disable_int();  //关中断
    scan_code = *(kb_in.p_tail);
    kb_in.p_tail++;
    if(kb_in.p_tail == kb_in.buf + KB_IN_BYTES){
        //将p_tail移至缓冲区头
        kb_in.p_tail = kb_in.buf;
    }
    kb_in.count--;
    enable_int();   //开中断
    return scan_code;
}