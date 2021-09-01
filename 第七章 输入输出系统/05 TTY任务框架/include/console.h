#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_
typedef struct s_console
{
    unsigned int current_start_addr;        //当前控制台滚动到了什么位置
    unsigned int original_addr;             //当前控制台对应的显存位置
    unsigned int v_mem_limit;               //显存大小
    unsigned int cursor;                    //当前光标位置
}CONSOLE;

#define DEFAULT_CHAR_COLOR	0x07	/* 0000 0111 黑底白字 */
#endif