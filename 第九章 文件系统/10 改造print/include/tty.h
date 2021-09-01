#ifndef _ORANGES_TTY_H_
#define _ORANGES_TTY_H_
#define TTY_IN_BYTES 256    /*tty输入缓冲区大小*/
#define TTY_OUT_BUF_LEN		2	/* tty output buffer size */

/*tty终端任务数据结构，跟键盘缓冲区类似，只不过多了一个console控制台元素*/
/*并且缓冲区中存储的是u32类型的key*/
typedef struct s_tty
{
    u32         in_buf[TTY_IN_BYTES];   //tty输入缓冲区
    u32         *p_inbuf_header;        //缓冲区下一个空闲位置
    u32         *p_inbuf_tail;          //待处理的第一个键
    int         inbuf_count;            //缓冲区中键个数

    int tty_caller; //直接向tty发送消息的进程，这里是FS
    int tty_procnr; //请求数据的进程
    int tty_req_buf;    //请求进程缓冲区线性地址
    int tty_left_cnt;   //请求读入的字符
    int tty_trans_cnt;  //tty已经向进程传送的数据

    CONSOLE     *p_console;             //控制台数据结构
}TTY;
#endif