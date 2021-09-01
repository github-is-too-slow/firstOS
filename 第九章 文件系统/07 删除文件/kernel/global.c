#define GLOBAL_VARIABLES_HERE_

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

/**
 * 添加一个进程的步骤：
 * 1.添加进程体(main.c)
 * 2.增加进程体原型(proto.h)
 * 3.在task_table中添加进程体入口地址和栈信息(global.c)
 * 4.定义任务堆栈，并更新进程数量NR_TASKS和栈大小STACK_SIZE_TOTAL(process.h)
 **/
PUBLIC TASK task_table[NR_TASKS] = {
    {task_tty,  STACK_SIZE_TASK_TTY,"task_tty"},
    {task_sys, STACK_SIZE_TASK_SYS, "task_sys"},
    {task_hd, STACK_SIZE_TASK_HD, "task_hd"},
    {task_fs, STACK_SIZE_TASK_HD, "task_hd"}
};

PUBLIC TASK user_proc_table[NR_PROCS] = {
    {TestA,     STACK_SIZE_TESTA,   "TestA"},
    {TestB,     STACK_SIZE_TESTB,   "TestB"},
    {TestC,     STACK_SIZE_TESTC,   "TestC"},
};

/**
 * 具体的i9259A中断处理逻辑指针数组
 * 由中断处理例程中的call [irq_table + 4 * %1]跳转
 **/
PUBLIC irq_handler irq_table[NR_IRQ];

/**
 * 系统调用子程序数量
 * 系统调用子程序表
 **/
PUBLIC  p_sys_call sys_call_table[NR_SYS_CALL] ={sys_printx, sys_sendrec};

/**
 * TTY终端任务和console控制台定义
 **/
PUBLIC TTY      tty_table[NR_CONSOLES];
PUBLIC CONSOLE  console_table[NR_CONSOLES];

/**
 * 主设备号到驱动程序(pid)的映射
 * 主设备号决定选取哪个驱动程序执行，从设备号决定操作哪个分区
 **/
struct dev_drv_map dd_map[] = {
	/* driver nr.		major device nr.
	   ----------		---------------- */
	{INVALID_DRIVER},	/**< 0 : Unused */
	{INVALID_DRIVER},	/**< 1 : Reserved for floppy driver */
	{INVALID_DRIVER},	/**< 2 : Reserved for cdrom driver */
	{TASK_HD},		/**< 3 : Hard disk */
	{TASK_TTY},		/**< 4 : TTY */
	{INVALID_DRIVER}	/**< 5 : Reserved for scsi disk driver */
};

PUBLIC 	u8 *fsbuf	= (u8*)0x600000;		//文件系统缓冲区
PUBLIC	const int FSBUF_SIZE = 0x100000;	//1M的大小