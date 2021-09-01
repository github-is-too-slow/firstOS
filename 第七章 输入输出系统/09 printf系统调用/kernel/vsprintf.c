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
 * 实现将参数嵌入到格式串中，并放入到buf缓冲区中
**/
PUBLIC int vsprintf(char *buf, const char *fmt, va_list args){
    char *p;
    char tmp[256];
    va_list p_next_arg = args;  //目前指向第一个参数
    for(p = buf; *fmt; fmt++){
        //p指向buf缓冲区，*fmt指向格式串中的一个字符，末尾是'/0'
        if(*fmt != '%'){//%是个标志字符
            *p++ = *fmt;   //非格式串就复制
            continue;
        }
        //遇到了%x
        fmt++;
        switch (*fmt)
        {
        case 'x':
            //将转化后的0xFFFF16进制字符串放入tmp中，并以'\0'结尾
            itoa(tmp, *((int*)p_next_arg));
            strcpy(p, tmp);
            p_next_arg += 4; //指向下一个参数，如果有的话
            p += strlen(tmp); //p刚好指向'\0'
            break;
        case 's':
            break;      //目前不支持%s,会将其过滤掉
        default:
            break;
        }
    }
    return (p - buf);   //表示缓冲区中字符个数，不包含'\0'空字符
}