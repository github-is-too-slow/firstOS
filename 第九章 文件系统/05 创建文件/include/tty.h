#ifndef _ORANGES_TTY_H_
#define _ORANGES_TTY_H_
#define TTY_IN_BYTES 256    /*tty输入缓冲区大小*/

/*tty终端任务数据结构，跟键盘缓冲区类似，只不过多了一个console控制台元素*/
/*并且缓冲区中存储的是u32类型的key*/
typedef struct s_tty
{
    u32         in_buf[TTY_IN_BYTES];   //tty输入缓冲区
    u32         *p_inbuf_header;        //缓冲区下一个空闲位置
    u32         *p_inbuf_tail;          //待处理的第一个键
    int         inbuf_count;            //缓冲区中键个数
    CONSOLE     *p_console;             //控制台数据结构
}TTY;
#endif