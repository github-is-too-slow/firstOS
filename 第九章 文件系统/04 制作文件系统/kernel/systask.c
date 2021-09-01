#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "string.h"
#include "protect.h"
#include "process.h"
#include "hd.h"
#include "fs.h"
#include "keyboard.h"
#include "global.h"
#include "proto.h"

//系统级任务，ring01，负责给用户进程返回中断计数
PUBLIC void task_sys(){
    MESSAGE msg;  //准备消息体
    while(1){
        //接收任意进程的消息，如果发送队列中有进程，则选第一个为其服务,否则，阻塞
        send_recv(RECEIVE, ANY, &msg);
        //已经接收到了消息，封装在msg中
        int src = msg.source; //msg的发起者是谁
        switch (msg.type)
        {
        case GET_TICKS:  //发送进程为了获得时钟计数
            msg.RETVAL = ticks;  //msg.u.m3.m3i1
            send_recv(SEND, src, &msg);
            break;
        default:
            panic("unknown msg type");
            break;
        }
    }
}