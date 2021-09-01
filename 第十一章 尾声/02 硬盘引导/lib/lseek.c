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
 *                                lseek
 *****************************************************************************/
/**
 * Reposition r/w file offset.
 *
 * @param fd      File descriptor.
 * @param offset  The offset according to `whence'.
 * @param whence  SEEK_SET, SEEK_CUR or SEEK_END.
 *  重新定位读写指针，有3个基址：文件头，当前位置，文件末尾
 * @return  The resulting offset location as measured in bytes from the
 *          beginning of the file.
 *****************************************************************************/
PUBLIC int lseek(int fd, int offset, int whence)
{
	MESSAGE msg;
	msg.type   = LSEEK;
	msg.FD     = fd;
	msg.OFFSET = offset;
	msg.WHENCE = whence;

	send_recv(BOTH, TASK_FS, &msg);
    //最终返回的是相对于文件开始的偏移字节
	return msg.OFFSET;
}