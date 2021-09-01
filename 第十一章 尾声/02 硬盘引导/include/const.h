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

/*可配置的系统常量*/
#define INSTALL_START_SECT 0x9320
#define INSTALL_NR_SECTS 0x800   //刚好支持1M


/*文件系统相关常量*/
#define	NR_DEFAULT_FILE_SECTS	2048 /* 2048 * 512 = 1MB 单个文件最大占用1M*/
/* INODE::i_mode (octal, lower 12 bits reserved) */
#define I_TYPE_MASK     0170000
#define I_REGULAR       0100000
#define I_BLOCK_SPECIAL 0060000
#define I_DIRECTORY     0040000
#define I_CHAR_SPECIAL  0020000
#define I_NAMED_PIPE	0010000
#define	NR_FILES	64		//单进程能够操作的文件上限
#define	NR_FILE_DESC	64	/* 所有进程能操作文件上限FIXME */
#define	NR_INODE	64	/* FIXME */
#define	NR_SUPER_BLOCK	8
#define	is_special(m)	((((m) & I_TYPE_MASK) == I_BLOCK_SPECIAL) || (((m) & I_TYPE_MASK) == I_CHAR_SPECIAL))

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
#define MINOR_hd1a 		0x10		//分区hdla对应的次设备号
#define MINOR_hd2a		(MINOR_hd1a + NR_SUB_PER_PART) //分区hd2a对应的次设备号
// #define ROOT_DEV		MAKE_DEV(DEV_HD, MINOR_BOOT)  //求出启动分区对应的设备号
#define ROOT_DEV		MAKE_DEV(DEV_HD, MINOR_hd2a)  //求出启动分区对应的设备号
#define	P_PRIMARY	0	//主分区标识
#define	P_EXTENDED	1	//扩展分区标识
#define SECTOR_SIZE_SHIFT	9		//偏移字节右移9位，得到篇扇区数
#define	DIOCTL_GET_GEO	1  //硬盘io控制请求号

/*文件系统相关*/
#define	INVALID_INODE		0
#define	ROOT_INODE		1  //根目录文件的inode号

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
//注意进程处于waiting和hanging只会造成该进程不会被调度
//并不会影响收发消息的操作，其状态改变等待MM内存管理模块处理
#define WAITING   0x08	/* set when proc waiting for the child to terminate */
#define HANGING   0x10	/* set when proc exits without being waited by parent */
#define FREE_SLOT 0x20	/* 空闲进程*/

/*任务数量和用户进程数量*/
#define NR_TASKS 5
#define NR_PROCS 32  //最多支持的用户进程
#define NR_NATIVE_PROCS	4	//TestA、TestB、TestC、Init
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
//task_xxx是任务进程pid，即在proc_table进程表中的下标
#define INVALID_DRIVER	-20
#define INTERRUPT	-10
#define TASK_TTY	0       //系统级终端任务task_tty
#define TASK_SYS	1       //系统级消息发送接受任务task_sys，这里只是发送中断调用计数
#define TASK_HD		2		//系统级硬盘驱动任务task_hd
#define TASK_FS     3	    //系统级文件系统任务task_fs
#define TASK_MM		4		//系统级内存管理任务task_mm
#define INIT		5
#define ANY		(NR_TASKS + NR_PROCS + 10)
#define NO_TASK		(NR_TASKS + NR_PROCS + 20)
typedef enum msg_type {
	NO_USE,
	/*
	 * when hard interrupt occurs, a msg (with type==HARD_INT) will
	 * be sent to some tasks
	 */
	HARD_INT,		//硬盘中断

	/* SYS task */
	GET_TICKS,
	//硬盘驱动负责的相关消息
	DEV_OPEN,
	DEV_CLOSE,
	DEV_READ,
	DEV_WRITE,
	DEV_IOCTL,
	//文件系统负责的相关消息
	OPEN,
	CLOSE,
	READ,
	WRITE,
	//删除一个文件
	UNLINK,
	SYSCALL_RET,
	STAT,
	LSEEK,
	//TTY发出的消息，由文件系统负责
	RESUME_PROC,
	SUSPEND_PROC,
	//内存调度MM负责的相关消息
	FORK,
	GET_PID,
	EXIT,
	WAIT,
	EXEC
}MsgType;
/* macros for messages */
#define	FD		u.m3.m3i1
#define	PATHNAME	u.m3.m3p1
#define	FLAGS		u.m3.m3i1
#define	NAME_LEN	u.m3.m3i2
#define	BUF_LEN		u.m3.m3i3
#define	CNT		u.m3.m3i2
#define	REQUEST		u.m3.m3i2
#define	PROC_NR		u.m3.m3i3
#define	DEVICE		u.m3.m3i4
#define	POSITION	u.m3.m3l1
#define	BUF		u.m3.m3p2
#define	OFFSET		u.m3.m3i2
#define	WHENCE		u.m3.m3i3

#define	PID		u.m3.m3i2
#define	RETVAL		u.m3.m3i1
#define	STATUS		u.m3.m3i1
/*消息体中的成员对应的信息描述*/
#define	RETVAL		u.m3.m3i1	//普通的返回值
#define	CNT		u.m3.m3i2		//读写的字节数量count
#define	REQUEST		u.m3.m3i2   //用户io控制请求标识
#define	PROC_NR		u.m3.m3i3  //进程pid
#define	DEVICE		u.m3.m3i4	//操作的硬盘分区的次设备号
#define	POSITION	u.m3.m3l1  //读写偏移字节，long型
#define	BUF		u.m3.m3p2    	//进程的缓冲区指针，相对于进程段的基址
#define	FD		u.m3.m3i1		//文件描述符在filp中的下标
#define	PATHNAME	u.m3.m3p1	//文件全路径名
#define	FLAGS		u.m3.m3i1	//打开文件模式
#define	NAME_LEN	u.m3.m3i2	//文件名长度
#define	OFFSET		u.m3.m3i2
#define	WHENCE		u.m3.m3i3    //文件偏移的基址
#define	BUF_LEN		u.m3.m3i3	//同一块区域可以被不同的内容使用
#define	PID		u.m3.m3i2		//子进程pid
#define	STATUS		u.m3.m3i1


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