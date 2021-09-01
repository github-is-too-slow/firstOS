/**
 * 进程涉及的4个方面：
 * 1. 进程体：如何加载到内存中
 * 2. 进程表：保存进程的状态信息：寄存器、ldt选择子、ldt表
 * 3. GDT：进程ldt表在GDT中的描述符
 * 4. TSS：一个进程任务对应一个TSS，保存任务切换时ss0/esp0堆栈指针
 **/

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
#include "elf.h"
#include "proto.h"

PUBLIC int get_ticks(){
    MESSAGE msg;
    reset_msg(&msg);
    msg.type = GET_TICKS;
    send_recv(BOTH, TASK_SYS, &msg);
    return msg.RETVAL;
}

/*****************************************************************************
 *                                Init
 * Linux中所有进程的公共祖先，其他进程都由该进程fork复制生成
 *****************************************************************************/
PUBLIC void Init()
{
	int fd_stdin  = open("/dev_tty0", O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	assert(fd_stdout == 1);

	print("Init() is running ...\n");

	int pid = fork();
	if (pid != 0) { /* parent process */
		print("parent is running, child pid:%d\n", pid);
		int s;
		int child = wait(&s);
		print("child (%d) exited with status: %d.\n", child, s);
	}
	else {	/* child process */
		print("child is running, pid:%d\n", getpid());
		exit(123);
	}

	while (1) {
		int s;
		int child = wait(&s);
		print("child (%d) exited with status: %d.\n", child, s);
	}
}


PUBLIC void TestA(){
    //测试普通文件删除操作
    int fd;
	int n;
    int i;
	char filename[] = "rdwr.txt";
	char bufw[] = "abcdefghig";
	int rd_bytes = 10;
	char bufr[rd_bytes + 1];

	assert(rd_bytes <= strlen(bufw));

	/* create */
	fd = open(filename, O_CREATE | O_RDWR);
	assert(fd != -1);
	printf("File created. fd: %d\n", fd);

	/* write */
	n = write(fd, bufw, strlen(bufw));
	assert(n == strlen(bufw));

	/* close */
	close(fd);

	/* open */
	fd = open(filename, O_RDWR);
	assert(fd != -1);
	printf("File opened. fd: %d\n", fd);

	/* read */
	n = read(fd, bufr, rd_bytes);
	assert(n == rd_bytes);
	bufr[n] = 0;
	printf("%d bytes read: %s\n", n, bufr);

	/* close */
	close(fd);

    char * filenames[] = {"/foo", "/bar", "/baz"};

	/* create files */
	// for (i = 0; i < sizeof(filenames) / sizeof(filenames[0]); i++) {
	// 	fd = open(filenames[i], O_CREATE | O_RDWR);
	// 	assert(fd != -1);
	// 	printf("File created: %s (fd %d)\n", filenames[i], fd);
	// 	close(fd);
	// }
	// char * rfilenames[] = {"/bar", "/foo", "/baz", "/dev_tty0"};

	// /* 以不同顺序删除文件remove files */
	// for (i = 0; i < sizeof(rfilenames) / sizeof(rfilenames[0]); i++) {
	// 	if (unlink(rfilenames[i]) == 0)
	// 		printf("File removed: %s\n", rfilenames[i]);
	// 	else
	// 		printf("Failed to remove file: %s\n", rfilenames[i]);
	// }
	spin("TestA");
}

PUBLIC void TestB(){
    //测试print读写操作
    char tty_name[] = "/dev_tty1";

	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];

	while (1) {
		print("$ ");
		int r = read(fd_stdin, rdbuf, 70);
		rdbuf[r] = 0;

		if (strcmp(rdbuf, "hello") == 0)
			print("hello world!\n");
		else
			if (rdbuf[0])
				print("{%s}\n", rdbuf);
	}

	assert(0); /* never arrive here */
}

PUBLIC void TestC(){
    int i = 10;
    while(i){
        // disp_color_str("C.", BRIGHT | MAKE_COLOR(BLACK,RED));
        // disp_int(get_ticks());
        // printf("C ");
        milli_delay(1000);  //20次时钟中断
        i--;
    }
    while(1){}
}

PUBLIC int kernel_main(){
    disp_str("-----\"kernel_main\" begins-----\n\n");

	int i, j;
    int eflags, prio;
    u8  rpl;
    u8  priv; /* privilege */

	TASK * t;
	PROCESS * p = proc_table;

	char * stk = task_stack + STACK_SIZE_TOTAL;
	for (i = 0; i < NR_TASKS + NR_PROCS; i++,p++,t++) {
        //空闲进程表
		if (i >= NR_TASKS + NR_NATIVE_PROCS) {
			p->p_flags = FREE_SLOT;
			continue;
		}

        if (i < NR_TASKS) {     /* TASK 系统任务级进程*/
            t	= task_table + i;
            priv	= PRIVILEGE_TASK;   //描述符权限
            rpl     = RPL_TASK;         //选择子权限
            eflags  = 0x1202;/* IF=1, IOPL=1, bit 2 is always 1 具有I/O特权*/
            prio    = 15;
        }
        else {                  /* USER PROC 用户进程*/
            t	= user_proc_table + (i - NR_TASKS);
            priv	= PRIVILEGE_USER;
            rpl     = RPL_USER;
            eflags  = 0x202;	/* IF=1, bit 2 is always 1 */
            prio    = 5;
        }
		strcpy(p->p_name, t->name);	/* name of the process */
        //无父进程
		p->p_parent = NO_TASK;
        //非INIT祖先进程
        //提前编译好的进程ldt与内核的基址和大小保持一致
		if (strcmp(t->name, "INIT") != 0) {
			p->ldts[INDEX_LDT_C]  = gdt[SELECTOR_KERNEL_CS >> 3];
			p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

			/* change the DPLs改变权限 */
			p->ldts[INDEX_LDT_C].attr1  = DA_C   | priv << 5;
			p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
		}
		else {		/* INIT process */
			unsigned int k_base;  //内核基址
			unsigned int k_limit;   //内核大小
			int ret = get_kernel_map(&k_base, &k_limit);
			assert(ret == 0);
			init_descriptor(&p->ldts[INDEX_LDT_C],
				  0, /*保证了基址为0，只不过大小变化了*/
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

			init_descriptor(&p->ldts[INDEX_LDT_RW],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
		}

		p->regs.cs = INDEX_LDT_C << 3 |	SA_TIL | rpl;
		p->regs.ds =
			p->regs.es =
			p->regs.fs =
			p->regs.ss = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
		p->regs.eip	= (u32)t->initial_eip;
		p->regs.esp	= (u32)stk;
		p->regs.eflags	= eflags;

		p->ticks = p->priority = prio;  //优先级

		p->p_flags = 0;
		p->p_msg = 0;
		p->p_recvfrom = NO_TASK;
		p->p_sendto = NO_TASK;
		p->has_int_msg = 0;
		p->q_sending = 0;
		p->next_sending = 0;
		//初始化文件描述符指针为0
		for (j = 0; j < NR_FILES; j++)
			p->filp[j] = 0;
		//更换栈指针
		stk -= t->stacksize;
	}

	k_reenter = 0;
	ticks = 0;
	//当前执行进程为task_tty
	p_proc_ready	= proc_table;

	init_clock();
    init_keyboard();
	disp_str("-----\"kernel_main\" is ending-----\n\n");
	restart();
	while(1){}
}

/*****************************************************************************
 *                                panic严重错误，直接使CPU停止运行
 * panic是一个函数形式存在，而assert是以一个宏形式存在
 * panic只会用在ring1任务或ring0中发生致命错误时才是用
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)(&fmt) + 4);

    //panic自身的不定参数先嵌入一次
	i = vsprintf(buf, fmt, arg);

    //printf不定参数再嵌入一次输出
	printf("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}