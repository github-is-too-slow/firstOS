#include "const.h"
#include "type.h"
#include "proto.h"
/**
 * 整形转化为字符串,将32位整数用16进制方式显示出来
 * 数字前面的0不被显示出来
 **/
PUBLIC char *itoa(char *str, int num){
    char *p = str;
    char ch;
    int i;
    int flag = 0;
    *p++ = '0';
    *p++ = 'x';
    if(num == 0){
        *p++ = '0';
    }else{
        for(i = 28; i >= 0; i -= 4){
            ch = (num >> i) & 0xF;
            if(flag || (ch > 0)){
                flag = 1;   //遇到第一个非0位
                ch += '0';
                if(ch > '9'){
                    ch += 7;
                }
                *p++ = ch;
            }
        }
    }
    *p = 0;
    return str;
}

/**
 * disp_int
 **/
PUBLIC void disp_int(int input){
    char output[16];
    itoa(output, input);
    disp_str(output);
}



