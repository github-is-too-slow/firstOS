#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "string.h"
#include "protect.h"
#include "fs.h"
#include "stdio.h"
#include "process.h"
#include "hd.h"
#include "keyboard.h"
#include "global.h"
#include "proto.h"

#define TTY_FIRST   (tty_table)      //指向第一个TTY任务
#define TTY_END     (tty_table + NR_CONSOLES)

/*对tty缓冲区和控制台指针进行初始化*/
PRIVATE void init_tty(TTY *p_tty){
    p_tty->inbuf_count = 0;
    p_tty->p_inbuf_header = p_tty->p_inbuf_tail = p_tty->in_buf;
    init_screen(p_tty);
}

/*封装的tty读取键盘缓冲区操作，具体操作还是由keyboard_read来做*/
PRIVATE void tty_dev_read(TTY *tty){
    // assert(1 + 1 != 2);
    // panic("in TTY %d...", p_tty - tty_table);
    if(is_current_console(tty->p_console)){
        //从键盘缓冲区中依次读取一个字节
        //并将解析到的四字节结果key放入tty缓冲区中
        //注意每次调用一次keyboard_read只会从键盘缓冲区中解析处一个key结果
        //并放入tty缓冲区中
        keyboard_read(tty);
    }
}

/**
 * 从tty缓冲区中取出字符打印，注意此时缓冲区中的key只有可打印字符，
 * 第9bit位为0
 * 只要缓冲区中有数据就打印，不管是不是当前控制台
**/
PRIVATE void tty_dev_write(TTY* tty)
{
	while (tty->inbuf_count) {  //传送数据的前提是有数据
		char ch = *(tty->p_inbuf_tail); //只需要截取低8位打印即可
		tty->p_inbuf_tail++;
		if (tty->p_inbuf_tail == tty->in_buf + TTY_IN_BYTES)
			tty->p_inbuf_tail = tty->in_buf;
		tty->inbuf_count--;

		if (tty->tty_left_cnt) {//请求进程还有请求字符
			if (ch >= ' ' && ch <= '~') { /* printable */
                //首先回显到屏幕上
				out_char(tty->p_console, ch);
                //然后将字符传送到请求进程的缓冲区中
				void * p = (void*)(tty->tty_req_buf + tty->tty_trans_cnt);
				memcpy(p, (void *)va2la(TASK_TTY, &ch), 1);
				tty->tty_trans_cnt++;
				tty->tty_left_cnt--;
			}//退格键，前提是已经有数据传输了
			else if (ch == '\b' && tty->tty_trans_cnt) {
				out_char(tty->p_console, ch);
                //下一个传送字符会覆盖之前的字符
				tty->tty_trans_cnt--;
				tty->tty_left_cnt++;
			}
            //不管用户期待字符数是否满足，只要用户按下回车键即结束
            //或者用户已经输入满字符也会自动结束
			if (ch == '\n' || tty->tty_left_cnt == 0) {
                //结束时屏幕会换行
				out_char(tty->p_console, '\n');
				MESSAGE msg;
				msg.type = RESUME_PROC;
				msg.PROC_NR = tty->tty_procnr;
				msg.CNT = tty->tty_trans_cnt;
                //向FS发送解除请求进程阻塞的消息
				send_recv(SEND, tty->tty_caller, &msg);
                //当FS接收到消息后，tty将剩余字符数清空
				tty->tty_left_cnt = 0;
			}
		}
	}
}

/*****************************************************************************
 *                                tty_do_read
 *****************************************************************************/
/**
 * Invoked when task TTY receives DEV_READ message.
 *
 * @note The routine will return immediately after setting some members of
 * TTY struct, telling FS to suspend the proc who wants to read. The real
 * transfer (tty buffer -> proc buffer) is not done here.
 * tty接收到用户进程请求从磁盘读取数据后，记下发出请求的进程号、缓冲区地址后，
 * 立即给FS进程发送了一条消息，这样的话，FS便不会被阻塞掉，
 * 它仍然可以接收其他进程的请求消息。而真正的传输工作是在tty_dev_write中进行
 * 当该函数完成传输工作后，它会向FS发送一条解除进程的消息，FS接收到之后，
 * 立即向用户进程发送一条消息后，用户进程就解除阻塞了
 * @param tty  From which TTY the caller proc wants to read.
 * @param msg  The MESSAGE just received.
 *****************************************************************************/
PRIVATE void tty_do_read(TTY* tty, MESSAGE* msg)
{
	/* tell the tty: */
	tty->tty_caller   = msg->source;  /* who called, usually FS */
	tty->tty_procnr   = msg->PROC_NR; /* who wants the chars */
	tty->tty_req_buf  = va2la(tty->tty_procnr,
				  msg->BUF);/* where the chars should be put用户进程缓冲区线性地址 */
	tty->tty_left_cnt = msg->CNT; /* how many chars are requested要传输字符数 */
	tty->tty_trans_cnt= 0; /* how many chars have been transferred 已经传输字符数初始化为0*/
    //告诉FS这是一条挂起用户进程的消息，FS所需做的事情便是不理会该消息，
    //这样便把进程给挂起了，因为此时用户进程还在等待接收FS的消息
	msg->type = SUSPEND_PROC;
	msg->CNT = tty->tty_left_cnt;
	send_recv(SEND, tty->tty_caller, msg);
}


/*****************************************************************************
 *                                tty_do_write
 *****************************************************************************/
/**
 * Invoked when task TTY receives DEV_WRITE message.
 * 用户进程请求向tty屏幕上输出一些信息，tty所做的是直接利用out_char写入显存即可，
 * 不用将这些信息写入tty缓冲区中
 * @param tty  To which TTY the calller proc is bound.
 * @param msg  The MESSAGE.
 *****************************************************************************/
PRIVATE void tty_do_write(TTY* tty, MESSAGE* msg)
{
	char buf[TTY_OUT_BUF_LEN];  //2字节的输出缓冲区
    //用户输出缓冲区线性地址
	char * p = (char*)va2la(msg->PROC_NR, msg->BUF);
	int i = msg->CNT;
	int j;

	while (i) {
		int bytes = min(TTY_OUT_BUF_LEN, i);
		memcpy(va2la(TASK_TTY, buf), (void*)p, bytes);
		for (j = 0; j < bytes; j++)
			out_char(tty->p_console, buf[j]);
		i -= bytes;
		p += bytes;
	}
    //显示完毕，立即向FS发送消息，FS立即向进程发送消息
	msg->type = SYSCALL_RET;
	send_recv(SEND, msg->source, msg);
}

PUBLIC void task_tty(){
    TTY *tty;
    MESSAGE msg;

    //对键盘缓冲区和键盘中断又一次初始化
    init_keyboard();
    for(tty = TTY_FIRST; tty < TTY_END; tty++){
        //对每一个tty进行初始化
        init_tty(tty);
    }
    //首次选中当前控制台为第一个tty
    select_console(0);
    while (1) {
        //tty设备的切换只在此处会调度，
        //只有一个tty设备中没有消息可以处理时，才会切换到下一个tty设备
        //用户键盘输入也可能切换当前tty设备
		for (tty = TTY_FIRST; tty < TTY_END; tty++) {
			do {
                //在tty是当前tty时，将键盘缓冲区中的数据读入tty缓冲区中
				tty_dev_read(tty);
                //将每个tty缓冲区中的数据回显到屏幕上，并且直接输送到请求进程缓冲区中
                //并且在输送完毕后向FS发送消息，解除FS和请求进程的阻塞
				tty_dev_write(tty);
			} while (tty->inbuf_count);
		}
        //读取和传送一轮数据从磁盘读取请求之后，开始接受下一轮数据读取请求消息
		send_recv(RECEIVE, ANY, &msg);

		int src = msg.source;
		assert(src != TASK_TTY);

		TTY* ptty = &tty_table[msg.DEVICE];  //次设备号，请求的是哪个tty设备

		switch (msg.type) {
		case DEV_OPEN:
            //接受并直接返回，什么也不干
			reset_msg(&msg);
			msg.type = SYSCALL_RET;
			send_recv(SEND, src, &msg);
			break;
		case DEV_READ:
            //真正的读取键盘并传送是在tty_dev_read、tty_dev_write做的
			tty_do_read(ptty, &msg);
			break;
		case DEV_WRITE:
            //在该函数内直接完成数据显示工作
			tty_do_write(ptty, &msg);
			break;
		case HARD_INT:
			/**
			 * waked up by clock_handler -- a key was just pressed
			 * @see clock_handler() inform_int()
			 * 由时钟中断唤醒该进程
			 */
			key_pressed = 0;
			continue;
		default:
			// dump_msg("TTY::unknown msg", &msg);
            assert(0);
			break;
		}
	}
}

/**
 * 将一个key放入tty缓冲区中
 **/
PRIVATE void put_key(TTY *p_tty, u32 key){
    if(p_tty->inbuf_count < TTY_IN_BYTES){
        //tty缓冲区未满
        *(p_tty->p_inbuf_header) = key;
        p_tty->p_inbuf_header++;
        if(p_tty->p_inbuf_header == p_tty->in_buf + TTY_IN_BYTES){
            p_tty->p_inbuf_header = p_tty->in_buf;
        }
        p_tty->inbuf_count++;
    }
}

/*仅将可打印字符写入tty缓冲区中*/
PUBLIC void in_process(TTY *p_tty, u32 key){
    //可打印字符
    if(!(key & FLAG_EXT)){
        put_key(p_tty, key);
    }else {
        //得到低9bit的原生信息,特殊键在第9位为1
        int raw_code = key & MASK_RAW;
        switch (raw_code)
        {
        case UP://控制滚屏
            if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)){//shift + up键
                scroll_screen(p_tty->p_console, SCR_UP);
            }
            break;
        case DOWN:
            if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)){//shift + down键
                scroll_screen(p_tty->p_console, SCR_DN);
            }
            break;
        case F1://切换终端
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
        case ENTER://回车键，这里将其当做可打印字符处理，放入了tty缓冲区中了
                   //仍然不违背tty缓冲区中只有可打印字符的原则
            put_key(p_tty, '\n');
            break;
        case BACKSPACE://退格键，这里将其当做可打印字符处理
            put_key(p_tty, '\b');
            break;
        default:
            break;
        }
    }
}

/**
 * 将字符串写入指定tty终端对应的控制台中，供打印
 * 注意：tty缓冲区只供键盘输入缓冲区的转储，用户进程直接写入到控制台中，并更新光标位置
 **/
PUBLIC void tty_write(TTY *p_tty, char *buf, int len){
    char *p = buf;
    int i = len;
    while(i){
        out_char(p_tty->p_console, *p++);
        i--;
    }
}

/**
 * 供系统调用使用，将字符串写入对应的控制台中
 **/
PUBLIC int sys_write(char *buf, int len, PROCESS *p_proc){
    tty_write(&tty_table[p_proc->nr_tty], buf, len);
    return 0;
}

/**
 * s: 是printf函数传递过来的要显示的嵌入数据后的格式串指针
 **/
PUBLIC int sys_printx(int _unused1, int _unused2, char* s, PROCESS *p_proc)
{
	const char * p;
	char ch;

	char reenter_err[] = "? k_reenter is incorrect for unknown reason";
	reenter_err[0] = MAG_CH_PANIC;   //默认是panic严重错误

	/**
	 * @note Code in both Ring 0 and Ring 1~3 may invoke printx().
     * 运行在ring0和ring1~3的进程都有可能调用invoke该函数
	 * If this happens in Ring 0, no linear-physical address mapping
	 * is needed.
	 *
	 * @attention The value of `k_reenter' is tricky here. When
	 *   -# printx() is called in Ring 0
	 *      - k_reenter > 0. When code in Ring 0 calls printx(),
	 *        an `interrupt re-enter' will occur (printx() generates
	 *        a software interrupt). Thus `k_reenter' will be increased
	 *        by `kernel.asm::save' and be greater than 0.
     *   当在ring0中又调用该函数，则发生中断重入，则k_reenter > 0
	 *   -# printx() is called in Ring 1~3
	 *      - k_reenter == 0.
     *   在ring1~3中调用该函数，因为是第一次进入中断，所以k_reenter = 0
	 */
	if (k_reenter == 0)  /* printx() called in Ring<1~3> */
		p = va2la(proc2pid(p_proc), s);
        //由虚拟地址求线性地址，因为s字符串在ring1~3进程中，所以要基于相应段求线性地址
	else if (k_reenter > 0) /* printx() called in Ring<0> */
		p = s;
	else	/* this should NOT happen */
		p = reenter_err;  //此时k_reenter更新异常，是panic异常

	/**
	 * @note if assertion fails in any TASK, the system will be halted;
	 * if it fails in a USER PROC, it'll return like any normal syscall
	 * does.
	 */
	if ((*p == MAG_CH_PANIC) ||
	    (*p == MAG_CH_ASSERT && p_proc_ready < &proc_table[NR_TASKS])) {
        //假如是panic异常或者是task系统进程下的assert断言错误，则会停止整个系统的运转
		disable_int();
		char * v = (char*)V_MEM_BASE;
		const char * q = p + 1; /* +1: skip the magic char */

		while (v < (char*)(V_MEM_BASE + V_MEM_SIZE)) {
			*v++ = *q++;
			*v++ = RED_CHAR;
			if (!*q) {//如果下一个字符为空，隔8行再显示一次
				while (((int)v - V_MEM_BASE) % (SCREEN_WIDTH * 16)) {
					/* *v++ = ' '; */
					v++;
					*v++ = GRAY_CHAR;
				}
				q = p + 1; //重新显示字符串，直至整个显存空闲显示完毕，即显示在所有控制台中，
			}
		}

        //不允许编译优化的内联汇编语句：暂停CPU，但可以响应键盘中断
		__asm__ __volatile__("hlt");
	}

    //用户进程下的assert断言异常,像普通函数返回，
    //之后在assertion_failure中进入死循环，从而仅仅使自己停转
	while ((ch = *p++) != 0) {
		if (ch == MAG_CH_PANIC || ch == MAG_CH_ASSERT)
			continue; /* skip the magic char */
        //仅仅显示在自己的控制台中
		out_char(tty_table[p_proc->nr_tty].p_console, ch);
	}

	return 0;
}