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
 *                                unlink
 * 删除一个文件inode，不是真正的删除文件数据区
 *****************************************************************************/
/**
 * Delete a file.
 *
 * @param pathname  The full path of the file to delete.
 * 需要给定文件的全路径名
 * @return Zero 0 if successful, otherwise -1.
 *****************************************************************************/
PUBLIC int unlink(char * pathname)
{
	MESSAGE msg;
	msg.type   = UNLINK;

	msg.PATHNAME	= (void*)pathname;
	msg.NAME_LEN	= strlen(pathname);

	send_recv(BOTH, TASK_FS, &msg);

	return msg.RETVAL;
}