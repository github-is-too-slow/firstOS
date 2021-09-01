#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "string.h"
#include "protect.h"
#include "process.h"
#include "hd.h"
#include "fs.h"
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
PRIVATE void tty_do_read(TTY *p_tty){
    // assert(1 + 1 != 2);
    // panic("in TTY %d...", p_tty - tty_table);
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
    char output[2] = {'\0', '\0'};
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