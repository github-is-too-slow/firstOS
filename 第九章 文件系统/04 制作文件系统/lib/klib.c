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

/**
 * 延迟函数
 **/
PUBLIC void delay(int time){
    int i, j, k;
    for(k = 0; k < time; k++){
        for(i = 0; i < 10; i++){
            for(j = 0; j < 10000; j++){
            }
        }
    }
}

/**
 * 清屏操作
 **/
PUBLIC void clearDisplay(){
    disp_pos = 0;
    for(int i = 0; i < 80 * 25; i++){
        disp_str(" ");
    }
    disp_pos = 0;
}

/**
 * min
 **/
PUBLIC int min(int a, int b){
    return a > b? b: a;
}