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

/*磁盘相关常量*/
#define	MAX_DRIVES		2   //最大驱动器数
#define	NR_PART_PER_DRIVE	4 //一个磁盘上的最大分区数
#define	NR_SUB_PER_PART		16		//一个分区上的最大逻辑分区
#define	NR_SUB_PER_DRIVE	(NR_SUB_PER_PART * NR_PART_PER_DRIVE)
#define	NR_PRIM_PER_DRIVE	(NR_PART_PER_DRIVE + 1)  //将磁盘本身也作为一个主分区
#define	MAX_PRIM		(MAX_DRIVES * NR_PRIM_PER_DRIVE - 1)  //多个驱动器统一编码
#define	MAX_SUBPARTITIONS	(NR_SUB_PER_DRIVE * MAX_DRIVES)
/* major device numbers (corresponding to kernel/global.c::dd_map[]) */
/*主设备号*/
#define	NO_DEV			0
#define	DEV_FLOPPY		1
#define	DEV_CDROM		2
#define	DEV_HD			3   //ata硬盘主设备号
#define	DEV_CHAR_TTY    4
#define	DEV_SCSI		5   //scsi硬盘
/* make device number from major and minor numbers */
/*利用主设备号和次设备号生成设备号*/
#define	MAJOR_SHIFT		8
#define	MAKE_DEV(a,b)		((a << MAJOR_SHIFT) | b)
/* separate major and minor numbers from device number */
/*利用设备号反求主设备号和次设备号*/
#define	MAJOR(x)		((x >> MAJOR_SHIFT) & 0xFF)
#define	MINOR(x)		(x & 0xFF)
#define	INVALID_INODE		0
#define	ROOT_INODE		1
#define MINOR_hd1a 		0x10		//分区hdla对应的次设备号
#define MINOR_hd2a		(MINOR_hd1a + NR_SUB_PER_PART) //分区hd2a对应的次设备号
// #define ROOT_DEV		MAKE_DEV(DEV_HD, MINOR_BOOT)  //求出启动分区对应的设备号
#define ROOT_DEV		MAKE_DEV(DEV_HD, MINOR_hd2a)  //求出启动分区对应的设备号
#define	P_PRIMARY	0	//主分区标识
#define	P_EXTENDED	1	//扩展分区标识
#define SECTOR_SIZE_SHIFT	9		//偏移字节右移9位，得到篇扇区数
#define	DIOCTL_GET_GEO	1  //硬盘io控制请求号

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
	DEV_OPEN,
	DEV_CLOSE,
	DEV_READ,
	DEV_WRITE,
	DEV_IOCTL
}MsgType;

/*消息体中的成员对应的信息描述*/
#define	RETVAL		u.m3.m3i1	//普通的返回值
#define	CNT		u.m3.m3i2		//读写的字节数量count
#define	REQUEST		u.m3.m3i2   //用户io控制请求标识
#define	PROC_NR		u.m3.m3i3  //进程pid
#define	DEVICE		u.m3.m3i4	//操作的硬盘分区的次设备号
#define	POSITION	u.m3.m3l1  //读写偏移字节，long型
#define	BUF		u.m3.m3p2    	//进程的缓冲区指针，相对于进程段的基址

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