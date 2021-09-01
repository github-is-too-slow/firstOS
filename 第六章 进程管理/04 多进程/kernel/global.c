#define GLOBAL_VARIABLES_HERE_

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "process.h"
#include "global.h"

/**
 * 添加一个进程的步骤：
 * 1.添加进程体(main.c)
 * 2.增加进程体原型(proto.h)
 * 3.在task_table中添加进程体入口地址和栈信息(global.c)
 * 4.定义任务堆栈，并更新进程数量NR_TASKS和栈大小STACK_SIZE_TOTAL(process.h)
 **/
PUBLIC TASK task_table[NR_TASKS] = {
    {TestA, STACK_SIZE_TESTA, "TestA"},
    {TestB, STACK_SIZE_TESTB, "TestB"},
    {TestC, STACK_SIZE_TESTC, "TestC"}
};