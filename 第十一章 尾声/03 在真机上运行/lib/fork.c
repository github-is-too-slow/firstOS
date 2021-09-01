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

/*****************************************************************************
 *                                fork
 * 在Linux中是用于创建子进程的一个系统调用
 *****************************************************************************/
/**
 * Create a child process, which is actually a copy of the caller.
 *  子进程是父进程调用者的完全状态拷贝
 * @return   On success, the PID of the child process is returned in the
 *         parent's thread of execution, and a 0 is returned in the child's
 *         thread of execution.成功时在父进程的调用执行中返回子进程的pid，而在子线程的调用执行中返回0
 *
 *           On failure, a -1 will be returned in the parent's context, no
 *         child process will be created.失败返回-1
 * 该系统调用会向MM发送消息
 *****************************************************************************/
PUBLIC int fork()
{
	MESSAGE msg;
	msg.type = FORK;

	send_recv(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);
	assert(msg.RETVAL == 0);

	return msg.PID;  //返回子进程的pid，即在proc_table进程表中的下标
}