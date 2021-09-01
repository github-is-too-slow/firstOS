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

//磁盘数据缓冲区，以字节为单位
PRIVATE	u8	hdbuf[SECTOR_SIZE * 2];
//磁盘状态
PRIVATE	u8	hd_status;

/**
 * 硬盘中断处理子程序，ring0，属于内核代码
 **/
PUBLIC void hd_handler(int irq){
    //读取状态寄存器，以便恢复中断
    hd_status = in_byte(REG_STATUS);
    //向task_hd发送中断消息，通知有磁盘中断
    inform_int(TASK_HD);
}

/**
 * 初始化硬盘
 **/
PRIVATE void init_hd(){
    u8 *pNrDrivers = (u8*)(0x475);     //BIOS参数区
    printf("pNrDrivers: %d\n", *pNrDrivers);  //硬盘驱动器数量
    assert(*pNrDrivers);
    //设置硬盘中断处理子程序
    put_irq_handler(AT_WINI_IRQ, hd_handler);
    //打开硬盘中断
    enable_irq(CASCADE_IRQ);
    enable_irq(AT_WINI_IRQ);
}

//在指定时间内循环检测磁盘的状态寄存器是否可用
PRIVATE int waitfor(int mask, int val, int timeout){
    int t = get_ticks();
    while(((get_ticks() - t) * 1000 / INT_FREQ) < timeout){
        if((in_byte(REG_STATUS) & mask) == val){
            //可用
            return 1;
        }
    }
    //检测超时
    return 0;
}

/**
 * 向磁盘发送命令
 **/
PRIVATE void hd_cmd_out(struct hd_cmd *cmd){
    //向磁盘发送命令
    //检查状态寄存器的busy位，必须为0才能够使用磁盘
    if(!waitfor(STATUS_BSY, 0, HD_TIMEOUT)){
        panic("hd_error");
    }
    //通过device control寄存器打开磁盘中断
    out_byte(REG_DEV_CTRL, 0);
    out_byte(REG_FEATURES, cmd->features);
    out_byte(REG_NSECTOR, cmd->sector_count);
    out_byte(REG_LBA_LOW, cmd->lba_low);
    out_byte(REG_LBA_MID, cmd->lba_mid);
    out_byte(REG_LBA_HIGH, cmd->lba_high);
    out_byte(REG_DEVICE, cmd->device);
    //发送IDENTIFY命令，一旦写入该命令，磁盘就开始工作了
    out_byte(REG_CMD, cmd->command);
}

/**
 * 等待磁盘中断消息
 **/
PRIVATE void interrupt_wait(){
    MESSAGE msg;
    //等待接收发送给task_hd进程的中断消息，由中断处理程序hd_handler发起
    //接受interrupt中断消息，只是起到了延迟到磁盘发出中断后，再处理的作用，
    //对消息体不会做任何事情
    send_recv(RECEIVE, INTERRUPT, &msg);
}

/**
 * 打印磁盘缓冲区中的磁盘信息
 **/
PRIVATE void print_identify_info(u16 *hdinfo){
    int i, k;
    char s[64]; //暂存缓冲区
    struct iden_info_ascii {
        int idx;
        int len;
        char *desc;
    }iinfo[] = {{10, 20, "HD SN"},   //序列号起始索引和长度
                {27, 40, "HD Model"}}; //型号起始索引和长度
    for(k = 0; k < sizeof(iinfo) / sizeof(iinfo[0]); k++){
        //得到某信息的起始位置
        char *p = (char*)&hdinfo[iinfo[k].idx];
        //打印若干长度,len是以字符为单位
        for(i = 0; i < iinfo[k].len / 2; i++){
            s[i * 2 + 1] = *p++;
            s[i * 2] = *p++;  //高位字节为0，空字符
        }
        s[i * 2] = 0;
        printf("%s: %s\n", iinfo[k].desc, s);
    }
    int capabilities = hdinfo[49];  //功能字节,bit9为1表示支持LBA
    printf("LBA supported: %s\n", (capabilities & 0x200)? "Yes": "No");
    int cmd_set_supported = hdinfo[83];  //支持的命令集，bit10为1表示支持48位寻址
    printf("LBA48 supported: %s\n", (cmd_set_supported & 0x400)? "Yes": "No");
    int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
    printf("HD size: %dMB\n", sectors * 512 / 1000000);
}

/**
 * 获取磁盘信息
 **/
PRIVATE void hd_identify(int drive){
    struct hd_cmd cmd;
    //采用逻辑块地址模式，操作主盘
    cmd.device = MAKE_DEVICE_REG(0, drive, 0);
    cmd.command = ATA_IDENTIFY;
    //向磁盘发送IDENTIFY命令，用于获取磁盘参数，共256个2字节参数
    hd_cmd_out(&cmd);
    //等待磁盘产生中断，接收其中断消息,延迟到磁盘发出中断后继续后续处理
    interrupt_wait();
    //从data寄存器中读取磁盘参数，从0x1F0端口地址读取，重复16次，共读取512个字节
    port_read(REG_DATA, hdbuf, SECTOR_SIZE);
    print_identify_info((u16*)hdbuf);
}

/**
 * 硬盘驱动DH Driver 主循环
 **/
PUBLIC void task_hd(){
    MESSAGE msg;
    //初始化硬盘
    init_hd();
    while(1){//不断地接收任意进程的磁盘处理请求，然后向磁盘发送请求处理，并将磁盘信息打印出来
        send_recv(RECEIVE, ANY, &msg);
        int src = msg.source;
        switch (msg.type)
        {
        case DEV_OPEN:
            hd_identify(0); //
            break;
        default:
            // dump_msg("HD driver:: unknown msg\n", &msg)
            //在此无限循环
            spin("FS::main loop (invalid msg.type)");
            break;
        }
        //向发起者回复消息
        send_recv(SEND, src, &msg);
    }
}