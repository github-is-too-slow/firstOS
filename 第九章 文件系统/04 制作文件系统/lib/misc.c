#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "string.h"
#include "protect.h"
#include "process.h"
#include "fs.h"
#include "global.h"
#include "proto.h"


/*****************************************************************************
 *                                spin
 *****************************************************************************/
PUBLIC void spin(char * func_name)
{
	printf("\nspinning in %s ...\n", func_name);
	while (1) {}
}

/*****************************************************************************
 *                           assertion_failure
 *************************************************************************//**
 * Invoked by assert().被断言宏assert(exp)调用
 *
 * @param exp       The failure expression itself.值为false的表达式
 * @param file      __FILE__
 * @param base_file __BASE_FILE__
 * @param line      __LINE__
 *****************************************************************************/
PUBLIC void assertion_failure(char *exp, char *file, char *base_file, int line)
{
	printf("%c  assert(%s) failed: file: %s, base_file: %s, ln%d",
	       MAG_CH_ASSERT,
	       exp, file, base_file, line);

	/**
	 * If assertion fails in a TASK, the system will halt before
	 * printl() returns. If it happens in a USER PROC, printl() will
	 * return like a common routine and arrive here.
	 * @see sys_printx()
	 *	假如是在任务中出现了断言assert异常，就会使整个系统停止，
	 *  而在用户进程中，只会使当前进程陷入无限循环
	 * We use a forever loop to prevent the proc from going on:
	 */
	spin("assertion_failure()");

	/* should never arrive here */
    //内联汇编，程序会在spin中循环，而不会出现该异常
    __asm__ __volatile__("ud2");
}