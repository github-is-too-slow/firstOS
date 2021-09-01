#ifndef _ORANGES_CONST_H_
#define _ORANGES_CONST_H_

/* the assert macro 断言宏*/
#define ASSERT
#ifdef ASSERT
void assertion_failure(char *exp, char *file, char *base_file, int line);
//断言宏,#exp将表达式转化为字符串常量
#define assert(exp)  if (exp) ; \
        else assertion_failure(#exp, __FILE__, __BASE_FILE__, __LINE__)
#else
#define assert(exp)
#endif

/* magic chars used by `sys_printx' */
#define MAG_CH_PANIC	'\002'
#define MAG_CH_ASSERT	'\003'

/* ipc function函数值 */
#define SEND		1
#define RECEIVE		2
#define BOTH		3	/* BOTH = (SEND | RECEIVE) */

/* 进程的p_flags取值，0表示可以执行，SENDING表示由于消息未达到所以阻塞，RECEIVING由于等待接收消息而被阻塞 */
#define SENDING   0x02	/* set when proc trying to send */
#define RECEIVING 0x04	/* set when proc trying to recv */

/*任务数量和用户进程数量*/
#define NR_TASKS 4
#define NR_PROCS 3
/*系统调用子程序数量*/
#define NR_SYS_CALL 2

/*硬盘相关*/
/* Hardware interrupts */
#define	NR_IRQ		16	/* Number of IRQs */
#define	CLOCK_IRQ	0
#define	KEYBOARD_IRQ	1
#define	CASCADE_IRQ	2	/* cascade enable for 2nd AT controller */
#define	ETHER_IRQ	3	/* default ethernet interrupt vector */
#define	SECONDARY_IRQ	3	/* RS232 interrupt vector for port 2 */
#define	RS232_IRQ	4	/* RS232 interrupt vector for port 1 */
#define	XT_WINI_IRQ	5	/* xt winchester */
#define	FLOPPY_IRQ	6	/* floppy disk */
#define	PRINTER_IRQ	7
#define	AT_WINI_IRQ	14	/* at winchester */

/* tasks */
/* 注意 TASK_XXX 的定义要与 global.c 中对应 */
#define INVALID_DRIVER	-20
#define INTERRUPT	-10
#define TASK_TTY	0       //系统级终端任务task_tty
#define TASK_SYS	1       //系统级消息发送接受任务task_sys
#define TASK_HD		2		//系统级硬盘驱动任务task_hd
#define TASK_FS     3	    //系统级文件系统任务task_fs
/* #define TASK_WINCH	2 */
/* #define TASK_FS	3 */
/* #define TASK_MM	4 */
#define ANY		(NR_TASKS + NR_PROCS + 10)
#define NO_TASK		(NR_TASKS + NR_PROCS + 20)
typedef enum msg_type {
	NO_USE,
	/*
	 * when hard interrupt occurs, a msg (with type==HARD_INT) will
	 * be sent to some tasks
	 */
	HARD_INT,

	/* SYS task */
	GET_TICKS,
	//打开硬盘
	DEV_OPEN
}MsgType;

#define	RETVAL		u.m3.m3i1

/*布尔值*/
#define TRUE 1
#define FALSE 0

/*函数类型*/
#define PUBLIC
#define PRIVATE static  //限制全局变量或函数的作用域

/*引用变量*/
#define EXTERN extern

/*前景色*/
#define BLACK   0x0     //0000
#define WHITE   0x7     //0111
#define RED     0x4     //0100
#define GREEN   0x2     //0010
#define BLUE    0x1     //0001
#define FLASH   0x80    //1000 0000
#define BRIGHT  0x08    //0000 1000
#define MAKE_COLOR(x, y)  (x | y)

/* VGA寄存器 */
#define	CRTC_ADDR_REG	0x3D4	/* CRT Controller Registers - Addr Register */
#define	CRTC_DATA_REG	0x3D5	/* CRT Controller Registers - Data Register */
#define	START_ADDR_H	0xC	/* reg index of video mem start addr (MSB) 起始位置*/
#define	START_ADDR_L	0xD	/* reg index of video mem start addr (LSB) */
#define	CURSOR_H	0xE	/* reg index of cursor position (MSB) 游标*/
#define	CURSOR_L	0xF	/* reg index of cursor position (LSB) */
#define	V_MEM_BASE	0xB8000	/* base of color video memory */
#define	V_MEM_SIZE	0x8000	/* 32K: B8000H -> BFFFFH */

/* AT keyboard */
/* 8042 ports */
/* I/O port for keyboard data*/
//Read : Read Output Buffer
//Write: Write Input Buffer(8042 Data&8048 Command) */
#define KB_DATA		0x60

/* I/O port for keyboard command*/
#define KB_CMD		0x64	//Write: Write Input Buffer(8042 Command)
#define KB_STATUS   0x64    //Read : Read Status Register


#define LED_CODE	0xED
#define KB_ACK		0xFA

#define NR_CONSOLES 3   /*TTY任务和console控制台个数*/

#endif