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

/*======================================================================*
                                i2a
 *======================================================================*/
PRIVATE char* i2a(int val, int base, char ** ps)
{//ps就是指向vsprintf中的指针q,q起始指向inner_buf首元素
	int m = val % base;
	int q = val / base;
	if (q) {//商为0时，不在递归下去
		i2a(q, base, ps);
	}
	*(*ps)++ = (m < 10) ? (m + '0') : (m - 10 + 'A'); //赋值并移动q指针指向

	return *ps;
}


/*======================================================================*
                                vsprintf
 *======================================================================*/
/*
 *  为更好地理解此函数的原理，可参考 printf 的注释部分。
 */
PUBLIC int vsprintf(char *buf, const char *fmt, va_list args)
{
	char*	p;

	va_list	p_next_arg = args;
	int	m;

	char	inner_buf[256];
	char	cs;
	int	align_nr;

	for (p=buf;*fmt;fmt++) {
		if (*fmt != '%') {
			*p++ = *fmt;
			continue;
		}
		else {		/* a format string begins */
			align_nr = 0;  //该变量表示右对齐的最少宽度
		}

		fmt++;

		if (*fmt == '%') {
			*p++ = *fmt;  //%%转义
			continue;
		}
		else if (*fmt == '0') {
			cs = '0'; //代表填充字符为0，如%05c
			fmt++;
		}
		else {
			cs = ' '; //代表填充字符为空格，如%5c
		}
		while (((unsigned char)(*fmt) >= '0') && ((unsigned char)(*fmt) <= '9')) {
			align_nr *= 10;
			align_nr += *fmt - '0';  //计算填充位数
			fmt++;
		}

		char * q = inner_buf;
		memset(q, 0, sizeof(inner_buf));  //内部缓冲区重置为空字符

		switch (*fmt) {
		case 'c':  //如：%5c,一个字节的字符
			*q++ = *((char*)p_next_arg);
			p_next_arg += 4;  //移动到下一个可变参数
			break;
		case 'x':  //如：%5x,16进制显示的整数
			m = *((int*)p_next_arg);
			i2a(m, 16, &q);
			p_next_arg += 4;
			break;
		case 'd':  //如：%5d,10进制显示一个整数
			m = *((int*)p_next_arg);
			if (m < 0) {
				m = m * (-1);  //求绝对值
				*q++ = '-';
			}
			i2a(m, 10, &q);
			p_next_arg += 4;
			break;
		case 's':  //如： %5s,显示一个字符串
			strcpy(q, (*((char**)p_next_arg)));
			q += strlen(*((char**)p_next_arg)); //此时q指向末尾空字符
			p_next_arg += 4;
			break;
		default:
			break;
		}
		//此时转化的字符串在缓冲区inner_buf中
		//先填充指定字符以对齐
		int k;
		for (k = 0; k < ((align_nr > strlen(inner_buf)) ? (align_nr - strlen(inner_buf)) : 0); k++) {//只有要显示的字符宽度小于指定宽度时，才填充
			*p++ = cs;
		}
		q = inner_buf;
		while (*q) {
			*p++ = *q++;
		}
	}

	*p = 0;

	return (p - buf);  //返回字符个数
}


/*======================================================================*
                                 sprintf
 *======================================================================*/
int sprintf(char *buf, const char *fmt, ...)
{
	va_list arg = (va_list)((char*)(&fmt) + 4);        /* 4 是参数 fmt 所占堆栈中的大小 */
	return vsprintf(buf, fmt, arg);
}