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

/*对调用函数的封装*/
PUBLIC int send_recv(int function, int src_dest, MESSAGE *msg){
    int ret = 0;
    if(function == RECEIVE){
        memset(msg, 0, sizeof(MESSAGE));
    }
    switch (function)
    {
    case BOTH:
        ret = sendrec(SEND, src_dest, msg);
        if(ret == 0){//成功发送
            ret = sendrec(RECEIVE, src_dest, msg);
        }
        break;
    case SEND:
    case RECEIVE:
        ret = sendrec(function, src_dest, msg);
        break;
    default:
        assert((function == BOTH) ||
                (function == SEND) ||
                (function == RECEIVE));
        break;
    }
    return ret;
}

/*****************************************************************************
 *                                memcmp
 *****************************************************************************/
/**
 * Compare memory areas.
 *
 * @param s1  The 1st area.
 * @param s2  The 2nd area.
 * @param n   The first n bytes will be compared.
 *
 * @return  an integer less than, equal to, or greater than zero if the first
 *          n bytes of s1 is found, respectively, to be less than, to match,
 *          or  be greater than the first n bytes of s2.
 *****************************************************************************/
PUBLIC int memcmp(const void * s1, const void *s2, int n)
{
	if ((s1 == 0) || (s2 == 0)) { /* for robustness */
		return (s1 - s2);
	}

	const char * p1 = (const char *)s1;
	const char * p2 = (const char *)s2;
	int i;
	for (i = 0; i < n; i++,p1++,p2++) {
		if (*p1 != *p2) {
			return (*p1 - *p2);
		}
	}
	return 0;
}

/*****************************************************************************
 *                                strcmp
 *****************************************************************************/
/**
 * Compare two strings.
 *
 * @param s1  The 1st string.
 * @param s2  The 2nd string.
 *
 * @return  an integer less than, equal to, or greater than zero if s1 (or the
 *          first n bytes thereof) is  found,  respectively,  to  be less than,
 *          to match, or be greater than s2.
 *****************************************************************************/
PUBLIC int strcmp(const char * s1, const char *s2)
{
	if ((s1 == 0) || (s2 == 0)) { /* for robustness */
		return (s1 - s2);
	}

	const char * p1 = s1;
	const char * p2 = s2;

	for (; *p1 && *p2; p1++,p2++) {
		if (*p1 != *p2) {
			break;
		}
	}

	return (*p1 - *p2);
}

/**
 * min
 **/
PUBLIC int min(int a, int b){
    return a > b? b: a;
}

PUBLIC int max(int a, int b){
    return a > b? a: b;
}