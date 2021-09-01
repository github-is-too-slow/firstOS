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

/**
 * @struct posix_tar_header
 * tar压缩包的文件头
 * Borrowed from GNU `tar'
 */
struct posix_tar_header
{				/* byte offset */
	char name[100];		/*   0 被压缩的原始文件名 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 文件大小，这里是八进制的字符串表示的文件大小 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[100];	/* 157 */
	char magic[6];		/* 257 */
	char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[155];	/* 345 */
	/* 500 */
};

/*****************************************************************************
 *                                untar
 *****************************************************************************/
/**
 * Extract the tar file and store them.
 * 压缩包只解压一次，新文件会覆盖掉旧文件
 * @param filename The tar file.
 *****************************************************************************/
void untar(char * filename)
{
	print("[extract `%s`\n", filename);
	//返回文件描述符的索引
	int fd = open(filename, O_RDWR);
	assert(fd != -1);

	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);
	int i = 0;
	int bytes = 0;

	while (1) {
		bytes = read(fd, buf, SECTOR_SIZE);
		assert(bytes == SECTOR_SIZE); /* size of a TAR file
					       * must be multiple of 512
						   *每次读压缩文件cmd.tar的一个扇区的大小
					       */
		if (buf[0] == 0) {
			if (i == 0)
				print("    need not unpack the file.\n");
			//如果压缩包的第一个字节为0，就意味着之前解压过，就不用再解压了
			break;
		}
		//解压文件数加1
		i++;
		struct posix_tar_header * phdr = (struct posix_tar_header *)buf;

		/* calculate the file size */
		//将八进制字符串转化为数字
		char * p = phdr->size;
		int f_len = 0;
		while (*p)
			f_len = (f_len * 8) + (*p++ - '0'); /* octal */

		int bytes_left = f_len;
		//假设文件之前已经解压过了，磁盘中存在该文件，
		//fdout就指向了该文件的inode，要么就指向一个新创建的inode
		//具有O_TRUNC可以覆盖原文件，此时文件描述符中读写指针position=0
		int fdout = open(phdr->name, O_CREATE | O_RDWR | O_TRUNC);
		if (fdout == -1) {
			print("    failed to extract file: %s\n", phdr->name);
			print(" aborted]\n");
			close(fd);
			return;
		}
		print("    %s\n", phdr->name);
		while (bytes_left) {
			int iobytes = min(chunk, bytes_left);
			//整数个扇区块读
			read(fd, buf,
			     ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
			bytes = write(fdout, buf, iobytes);
			assert(bytes == iobytes);
			bytes_left -= iobytes;
		}
		close(fdout);
	}
	//假设这次有解压压缩包，就修改第一个字节为0
	if (i) {
		lseek(fd, 0, SEEK_SET);
		buf[0] = 0;
		//修改第一个字节为0
		bytes = write(fd, buf, 1);
		assert(bytes == 1);
	}
	close(fd);
	print(" done]\n");
}

/*****************************************************************************
 *                                shabby_shell
 *****************************************************************************/
/**
 * A very very simple shell.
 * shell进程,这里就是一个shell命令解释器而已，真正的命令，如echo是单独的一个程序
 * @param tty_name  TTY file name.
 *****************************************************************************/
void shabby_shell(char * tty_name)
{
	//一个shell进程通常默认打开两个文件描述符
	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];
	char default_login_user[] = "jack@rose$ ";
	while (1) {
		write(1, default_login_user, 11);
		//读取用户输入的shell命令
		int r = read(0, rdbuf, 127);
		rdbuf[r] = 0;

		int argc = 0;
		//输入字符串的指针数组
		char * argv[PROC_ORIGIN_STACK];
		char * p = rdbuf;
		char * s;
		int word = 0;
		char ch;
		//开始解析用户输入
		do {
			ch = *p;
			//一个单词的首字符会满足该条件
			if (*p != ' ' && *p != 0 && !word) {
				s = p;
				word = 1;
			}
			//一个单词的末尾空格或结束标志会满足该条件
			if ((*p == ' ' || *p == 0) && word) {
				word = 0;
				argv[argc++] = s;
				//作为一个单词的结束标志
				*p = 0;
			}
			//一个单词的其余字符直接跳过
			p++;
		} while(ch);  //遇到字符串末尾退出解析
		//刚好最后一个指针的值为0，作为指针的结束标志
		argv[argc] = 0;
		//第一个单词作为shell命令程序的全路径地址
		int fd = open(argv[0], O_RDWR);
		if (fd == -1) {//没有该命令
			if (rdbuf[0]) {
				write(1, "{", 1);
				write(1, rdbuf, r);
				write(1, "}\n", 2);
			}
		}
		else {
			close(fd);
			//产生shell子进程，执行用户输入的命令程序
			int pid = fork();
			if (pid != 0) { /* parent shell解释程序*/
				int s;
				wait(&s);
			}
			else {	/* child shell命令程序 */
				//直接传递命令path和参数指针数组
				//该函数建立shell命令程序需要的栈空间，并且加载shell程序进入内存
				//并且作为该子进程的执行体
				//当该进程体执行完毕后，会主动退出exit，并将返回值传递给等待的父进程
				execv(argv[0], argv);
			}
		}
	}
	//当退出时，该shell解释程序关闭输入输出文件
	close(1);
	close(0);
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

	/* extract `cmd.tar' */
	//提取压缩包中的文件，会在磁盘上新建立3个新文件：/echo、/
	untar("/cmd.tar");

	char * tty_list[] = {"/dev_tty1", "/dev_tty2"};

	int i;
	for (i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++) {
		int pid = fork();
		if (pid != 0) { /* parent process Init父进程*/
			print("[parent is running, child pid:%d]\n", pid);
		}
		else {	/* child process */
			print("[child is running, pid:%d]\n", getpid());
			//子进程关闭原来父进程开始的输入输出文件
			//只有这样shell进程开启的输入输出文件描述符才是0和1
			close(fd_stdin);
			close(fd_stdout);
			//子进程循环执行shell解释程序，接收用户输入的命令，并开启子子进程去执行
			shabby_shell(tty_list[i]);
			assert(0);
		}
	}

	while (1) {
		int s;
		int child = wait(&s);
		print("child (%d) exited with status: %d.\n", child, s);
	}
}


PUBLIC void TestA(){
	spin("TestA");
}

PUBLIC void TestB(){
	spin("TestB");
}

PUBLIC void TestC(){
    spin("TestC");
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

		p->regs.cs = (INDEX_LDT_C << 3 |	SA_TIL | rpl);
		p->regs.ds =
			p->regs.es =
			p->regs.fs =
			p->regs.ss = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.gs = ((SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl);
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
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)(&fmt) + 4);

    //panic自身的不定参数先嵌入一次
	vsprintf(buf, fmt, arg);

    //printf不定参数再嵌入一次输出
	printf("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}