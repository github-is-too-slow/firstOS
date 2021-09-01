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

//利用次设备号求出分区所在的驱动器号
#define DRV_OF_DEV(dev) (dev <= MAX_PRIM? \
                        dev / NR_PRIM_PER_DRIVE: \
                        (dev - MINOR_hd1a) / NR_SUB_PER_DRIVE)
//磁盘数据缓冲区，以字节为单位
PRIVATE	u8	hdbuf[SECTOR_SIZE * 2];
//磁盘状态
PRIVATE	u8	hd_status;
//硬盘中的分区信息，目前只考虑通道IDE0上的0号硬盘
PRIVATE struct hd_info hd_info[MAX_DRIVES];


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
    int i;
    u8 *pNrDrivers = (u8*)(0x475);     //BIOS参数区
    printf("pNrDrivers: %d\n", *pNrDrivers);  //硬盘驱动器数量
    assert(*pNrDrivers);
    //设置硬盘中断处理子程序
    put_irq_handler(AT_WINI_IRQ, hd_handler);
    //打开硬盘中断
    enable_irq(CASCADE_IRQ);
    enable_irq(AT_WINI_IRQ);
    for(i = 0; i < (sizeof(hd_info) / sizeof(hd_info[0])); i++){
        //初始化每一个硬盘分区信息为0
        memset(&hd_info[i], 0, sizeof(hd_info[0]));
    }
    hd_info[0].open_cnt = 0;  //以便在可以在第一次访问时打印分区信息
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
PRIVATE void print_identify_info(u16 *hd_info){
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
        char *p = (char*)&hd_info[iinfo[k].idx];
        //打印若干长度,len是以字符为单位
        for(i = 0; i < iinfo[k].len / 2; i++){
            s[i * 2 + 1] = *p++;
            s[i * 2] = *p++;  //高位字节为0，空字符
        }
        s[i * 2] = 0;
        printf("%s: %s\n", iinfo[k].desc, s);
    }
    int capabilities = hd_info[49];  //功能字节,bit9为1表示支持LBA
    printf("LBA supported: %s\n", (capabilities & 0x200)? "Yes": "No");
    int cmd_set_supported = hd_info[83];  //支持的命令集，bit10为1表示支持48位寻址
    printf("LBA48 supported: %s\n", (cmd_set_supported & 0x400)? "Yes": "No");
    int sectors = ((int)hd_info[61] << 16) + hd_info[60];
    printf("HD size: %dMB\n\n", sectors * 512 / 1000000);
}

/**
 * 获取磁盘信息
 **/
PRIVATE void hd_identify(int drive){
    struct hd_cmd cmd;
    //采用CHS地址模式，操作主盘
    cmd.device = MAKE_DEVICE_REG(0, drive, 0);
    cmd.command = ATA_IDENTIFY;
    //向磁盘发送IDENTIFY命令，用于获取磁盘参数，共256个2字节参数
    hd_cmd_out(&cmd);
    //等待磁盘产生中断，接收其中断消息,延迟到磁盘发出中断后继续后续处理
    interrupt_wait();
    //从data寄存器中读取磁盘参数，从0x1F0端口地址读取，重复16次，共读取512个字节
    port_read(REG_DATA, hdbuf, SECTOR_SIZE);
    print_identify_info((u16*)hdbuf);
    //初始化硬盘的起始扇区号和大小
    hd_info[drive].primary[0].base = 0;
    hd_info[drive].primary[0].size = ((int)hdbuf[61] << 16) + hdbuf[60];
}

/**
 * 从指定驱动器的指定扇区的指定偏移字节处开始读取分区表的信息到指定内存
 * 注意分区表的大小是固定的，包含4个分区项，64字节
 **/
PRIVATE void get_part_table(int driver, int sect_nr, struct part_ent *entry){
    struct hd_cmd cmd;
    cmd.features = 0;
    cmd.sector_count = 1;   //读取一个扇区
    cmd.lba_low = sect_nr & 0xFF;
    cmd.lba_mid = (sect_nr >> 8) & 0xFF;
    cmd.lba_high = (sect_nr >> 16) & 0xFF;
    cmd.device = MAKE_DEVICE_REG(1, driver, (sect_nr >> 24) & 0xF);
    cmd.command = ATA_READ;  //发出读命令
    hd_cmd_out(&cmd);       //向磁盘发出命令
    interrupt_wait();       //等待磁盘发出中断
    port_read(REG_DATA, hdbuf, SECTOR_SIZE);     //读取一个扇区字节
    memcpy(entry,     //从偏移字节处读取分区表
            hdbuf + PARTITION_TABLE_OFFSET,
            sizeof(struct part_ent) * NR_PART_PER_DRIVE);
}

/**
 *指定次设备号，获取所在硬盘上的所有分区内容
**/
PRIVATE void partition(int device, int style){
    int i;
    int driver = DRV_OF_DEV(device);
    //所在的硬盘信息
    struct hd_info *hdi = &hd_info[driver];
    //分区表信息缓冲区
    struct part_ent part_tbl[NR_SUB_PER_DRIVE];
    if(style == P_PRIMARY){
        //此时需要获取4个主分区所在的分区表信息，即一级分区表
        get_part_table(driver, driver, part_tbl);
        int nr_prim_parts = 0;
        for(i = 0; i < NR_PART_PER_DRIVE; i++){//0 ~ 3
            if(part_tbl[i].sys_id == NO_PART){
                continue;
            }
            nr_prim_parts++;
            int dev_nr = i + 1;
            //获取分区基址
            hdi->primary[dev_nr].base = part_tbl[i].start_sect;
            //获取分区大小
            hdi->primary[dev_nr].size = part_tbl[i].nr_sects;
            if(part_tbl[i].sys_id == EXT_PART){
                //获取对应扩展分区上分区信息
                partition(device + dev_nr, P_EXTENDED);
            }
        }
        assert(nr_prim_parts != 0);
    }else if(style == P_EXTENDED){
        //此时需要获取指定主扩展分区上的所有逻辑分区和扩展分区
        int j = device % NR_PRIM_PER_DRIVE;  //获取主分区对应的次设备号，1~4
        int ext_start_sect = hdi->primary[j].base;  //主分区起始扇区号
        int s = ext_start_sect;
        int nr_1st_sub = (j - 1) * NR_SUB_PER_PART; //0/16/32/48
        for(i = 0; i < NR_SUB_PER_PART; i++){
            int dev_nr = nr_1st_sub + i;//在hdi->logical表中的索引
            //获取扩展分区第一个扇区中的分区表
            get_part_table(driver, s, part_tbl);
            //获取0号分区项表示一个逻辑分区或扩展分区
            //分区起始扇区号是相对于所在扩展分区而言
            hdi->logical[dev_nr].base = s + part_tbl[0].start_sect;
            hdi->logical[dev_nr].size = part_tbl[0].nr_sects;
            //指向下一个扩展分区的起始扇区，每个扩展分区的分区表中只有两个分区项，一个指向逻辑分区，一个指向下一个扩展分区
            //或者根本没有逻辑分区，第一个指向扩展分区，第一个为0
            s = ext_start_sect + part_tbl[1].start_sect;
            if(part_tbl[1].sys_id == NO_PART){
                //当该主扩展分区没有子扩展分区了就结束了
                break;
            }
        }
    }else {
        assert(0);  //永远不会执行到这里
    }
}

/**
 * 打印硬盘分区信息
 **/
PRIVATE void print_hd_info(struct hd_info *hdi){
    int i;
    //主分区
    for(i = 0; i < NR_PRIM_PER_DRIVE; i++){
        printf("%sPART_%d: base %d(0x%x), size %d(0x%x) (in sector)\n", i == 0? " ": "     ", i,
        hdi->primary[i].base,
        hdi->primary[i].base,
        hdi->primary[i].size,
        hdi->primary[i].size);
    }
    //逻辑分区
    for(i = 0; i < NR_SUB_PER_DRIVE; i++){
        if(hdi->logical[i].size == 0){
            continue;
        }
        printf("         ""%d: base %d(0x%x), size %d(0x%x) (in sector)\n", i,
        hdi->logical[i].base,
        hdi->logical[i].base,
        hdi->logical[i].size,
        hdi->logical[i].size);
    }
}

/*接收分区的次设备号，打开磁盘操作*/
PUBLIC void hd_open(int minor_device){
    //求出驱动器号
    int driver = DRV_OF_DEV(minor_device);
    assert(driver == 0);
    hd_identify(driver);  //获取一些硬盘描述信息
    if(hd_info[driver].open_cnt++ == 0){
        //首先获取驱动器上的所有分区信息
        partition(driver * NR_PRIM_PER_DRIVE, P_PRIMARY);
        //打印分区信息
        print_hd_info(&hd_info[driver]);
    }
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
            hd_open(msg.DEVICE); //负责打开硬盘，此处只是显示硬盘的分区信息
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