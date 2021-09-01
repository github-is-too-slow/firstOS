/**
 * 文件系统模块主程序
 **/
#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "string.h"
#include "protect.h"
#include "process.h"
#include "global.h"
#include "hd.h"
#include "proto.h"

PUBLIC void task_fs(){
    printf("Task FS begins.\n");
    MESSAGE driver_msg;
    //发送打开硬盘设备消息给硬盘驱动任务
    driver_msg.type = DEV_OPEN;
    send_recv(BOTH, TASK_HD, &driver_msg);
    //只发送一次就在此停止
    spin("FS");
}