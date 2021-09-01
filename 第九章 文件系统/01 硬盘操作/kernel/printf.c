#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "string.h"
#include "protect.h"
#include "process.h"
#include "global.h"
#include "proto.h"

/**
 * 功能:按照指定格式打印若干数据，此处只支持%x十六进制打印整数
 * fmt: 格式串指针
 * ...: 类型和个数不定的不定参数
 **/
PUBLIC int printf(const char *fmt, ...){
    int i;
    char buf[256];
    //4是格式串指针fmt在堆栈中的大小
    //目前arg是char *类型，指向栈中的第一个不定参数
    //更确切的说：arg是个局部变量，里面的值是第一个不定参数的内存地址
    va_list arg = (va_list)((char*)(&fmt) + 4);
    i = vsprintf(buf, fmt, arg);
    //此时已经将参数嵌入到了格式串中并保存到了buf缓冲区中，i代表了字符个数
    buf[i] = 0; //将其转化为字符串
	printx(buf);
    return i;  //返回写入的字符个数，当然不包括空字符'\0'了
}