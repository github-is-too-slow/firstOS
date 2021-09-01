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
 *                                open
 *                             创建或打开一个文件
 *****************************************************************************/
/**
 * open/create a file.
 * 这是给用户进程提供的接口，与文件系统进程FS通讯
 * @param pathname  The full path of the file to be opened/created.
 * 注意：这里提供的是全路径名，如：'/blah'
 * @param flags     O_CREAT, O_RDWR, etc.
 *创建/读写模式
 * @return File descriptor if successful, otherwise -1.
 * 返回值从0开始，是进程表中filp数组的下标，错误返回-1
 *****************************************************************************/
PUBLIC int open(char *pathname, int flags)
{
	MESSAGE msg;

	msg.type	= OPEN;     //打开或创建文件

	msg.PATHNAME	= (void*)pathname;
	msg.FLAGS	= flags;
	msg.NAME_LEN	= strlen(pathname);     //字符串长度

	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.FD;
}