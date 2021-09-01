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

PUBLIC void do_fork_test();

PRIVATE void init_mm();

/*****************************************************************************
 *                                task_mm
 *****************************************************************************/
/**
 * <Ring 1> The main loop of TASK MM.
 *
 *****************************************************************************/
PUBLIC void task_mm()
{
	init_mm();

	while (1) {
		send_recv(RECEIVE, ANY, &mm_msg);
		int src = mm_msg.source;
		int reply = 1;

		int msgtype = mm_msg.type;

		switch (msgtype) {
		case FORK:
			mm_msg.RETVAL = do_fork();
			break;
		//无论是退出还是等待，子进程表无需过多关注，因为它们已经名存实亡了，进程表被清理就够了
		//而父进程可能处于receiving状态，而这个状态是由子进程cleanup进程表时主动向父进程发送消息的
		//父进程此时必定由receiveing状态转为可调度状态，并且消息体也已经传递过去了，
		//因此不需要MM进程在向其发送消息了
		case EXIT:
			do_exit(mm_msg.STATUS);
			reply = 0;
			break;
		case WAIT:
			do_wait();
			reply = 0;
			break;
		default:
			// dump_msg("MM::unknown msg", &mm_msg);
			assert(0);
			break;
		}

		if (reply) {
			mm_msg.type = SYSCALL_RET;
			send_recv(SEND, src, &mm_msg);
		}
	}
}

/*****************************************************************************
 *                                init_mm
 *****************************************************************************/
/**
 * Do some initialization work.
 *
 *****************************************************************************/
PRIVATE void init_mm()
{
	struct boot_params bp;
	get_boot_params(&bp);
    //获取内存信息
	memory_size = bp.mem_size;

	/* print memory size */
	printf("{MM} memsize:%dMB\n", memory_size / (1024 * 1024));
}

/*****************************************************************************
 *                                alloc_mem
 *****************************************************************************/
/**
 * Allocate a memory block for a proc.
 *
 * @param pid  Which proc the memory is for.子进程pid
 * @param memsize  How many bytes is needed.
 * 这里采用固定分区分配方式，从10MB开始给用户进程分配内存，每个进程1MB
 * @return  The base of the memory just allocated.
 *****************************************************************************/
PUBLIC int alloc_mem(int pid, int memsize)
{
	assert(pid >= (NR_TASKS + NR_NATIVE_PROCS));
	if (memsize > PROC_IMAGE_SIZE_DEFAULT) {
		panic("unsupported memory request: %d. "
		      "(should be less than %d)",
		      memsize,
		      PROC_IMAGE_SIZE_DEFAULT);
	}
	//分配空间的基址
	int base = PROCS_BASE +
		(pid - (NR_TASKS + NR_NATIVE_PROCS)) * PROC_IMAGE_SIZE_DEFAULT;

	if (base + memsize >= memory_size)
		panic("memory allocation failed. pid:%d", pid);

	return base;
}

/*****************************************************************************
 *                                free_mem
 *****************************************************************************/
/**
 * Free a memory block. Because a memory block is corresponding with a PID, so
 * we don't need to really `free' anything. In another word, a memory block is
 * dedicated to one and only one PID, no matter what proc actually uses this
 * PID.
 *
 * @param pid  Whose memory is to be freed.
 * 释放子进程内存空间，因为内存空间和进程是一一对应的，所以什么事也不用做
 * @return  Zero if success.
 *****************************************************************************/
PUBLIC int free_mem(int pid)
{
	return 0;
}