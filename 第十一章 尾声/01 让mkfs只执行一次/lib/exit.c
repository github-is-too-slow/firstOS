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
 *                                exit
 *************************************************************************//**
 * Terminate the current process.
 *
 * @param status  The value returned to the parent.
 *****************************************************************************/
PUBLIC void exit(int status)
{
	MESSAGE msg;
	msg.type	= EXIT;
	msg.STATUS	= status;

	send_recv(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);
}