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
#include "hd.h"
#include "fs.h"
#include "keyboard.h"
#include "global.h"
#include "proto.h"

PRIVATE void init_fs();
PRIVATE void mkfs();

PUBLIC void task_fs(){
    printf("Task FS begins.\n");
    //初始化文件系统
    init_fs();
    spin("FS");
}

/**
 * 初始化文件系统
 **/
PRIVATE void init_fs()
{
	/* open the device: hard disk 这里仅输出整块驱动器上的分区信息*/
	MESSAGE driver_msg;
	driver_msg.type = DEV_OPEN;
	driver_msg.DEVICE = MINOR(ROOT_DEV);
	assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);
    //制作文件系统，并开始监听用户进程消息
	mkfs();
}

/*****************************************************************************
 *                                mkfs
 *****************************************************************************/
/**
 * <Ring 1> Make a available Orange'S FS in the disk. It will
 *          - Write a super block to sector 1. 超级块
 *          - Create three special files: dev_tty0, dev_tty1, dev_tty2  3个特殊设备文件
 *          - Create the inode map   iNode映射表
 *          - Create the sector map     sector映射表
 *          - Create the inodes of the files  iNode_array表
 *          - Create `/', the root directory    根目录区
 *****************************************************************************/
PRIVATE void mkfs()
{
	MESSAGE driver_msg;
	int i, j;

	int bits_per_sect = SECTOR_SIZE * 8; /* 8 bits per byte */

	/* get the geometry of ROOTDEV 获取启动分区的信息：起始扇区号和扇区数*/
	struct part_info geo;
	driver_msg.type		= DEV_IOCTL;
	driver_msg.DEVICE	= MINOR(ROOT_DEV);
	driver_msg.REQUEST	= DIOCTL_GET_GEO;
	driver_msg.BUF		= &geo;
	driver_msg.PROC_NR	= TASK_FS;
	assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);

	printf("root dev size: 0x%x sectors\n\n", geo.size);

	/************************/
	/*      super block     */
	/************************/
	struct super_block sb;
	sb.magic	  = MAGIC_V1;
	sb.nr_inodes	  = bits_per_sect;  //支持的iNode最大个数，同时也是文件最大个数
	sb.nr_inode_sects = sb.nr_inodes * INODE_SIZE / SECTOR_SIZE;
	sb.nr_sects	  = geo.size; /* partition size in sector分区扇区总数，并且0号扇区就是引导扇区 */
	sb.nr_imap_sects  = 1;
	sb.nr_smap_sects  = sb.nr_sects / bits_per_sect + 1;   //向上取整
    //数据区的第一个扇区号
	sb.n_1st_sect	  = 1 + 1 +   /* boot sector & super block */
		sb.nr_imap_sects + sb.nr_smap_sects + sb.nr_inode_sects;
	sb.root_inode	  = ROOT_INODE;  //根目录也当做一个特殊文件，iNode号为1
	sb.inode_size	  = INODE_SIZE;
	struct inode x;     //得到iNode中各元素的偏移
	sb.inode_isize_off= (int)&x.i_size - (int)&x;
	sb.inode_start_off= (int)&x.i_start_sect - (int)&x;
	sb.dir_ent_size	  = DIR_ENTRY_SIZE;
	struct dir_entry de;
	sb.dir_ent_inode_off = (int)&de.inode_nr - (int)&de;
	sb.dir_ent_fname_off = (int)&de.name - (int)&de;

	memset(fsbuf, 0x90, SECTOR_SIZE);   //超级块所在1号扇区其余位置填充为0x90
	memcpy(fsbuf, &sb, SUPER_BLOCK_SIZE);

	/* write the super block */
    //写扇区宏操作，只需给出分区设备号和待写入的扇区号即可，
    //默认是将512字节大小的fsbuf缓冲区中的内容写入
	WR_SECT(ROOT_DEV, 1);

    //显示文件系统中各部分在整块磁盘上的偏移字节
	printf("devbase:0x%x00, sb:0x%x00, imap:0x%x00, smap:0x%x00\n"
	       "        inodes:0x%x00, 1st_sector:0x%x00\n\n",
	       geo.base * 2,    //乘以了512，单位字节
	       (geo.base + 1) * 2,
	       (geo.base + 1 + 1) * 2,
	       (geo.base + 1 + 1 + sb.nr_imap_sects) * 2,
	       (geo.base + 1 + 1 + sb.nr_imap_sects + sb.nr_smap_sects) * 2,
	       (geo.base + sb.n_1st_sect) * 2);

	/************************/
	/*       inode map      */
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	for (i = 0; i < (NR_CONSOLES + 2); i++)  //0号iNode保留，1号iNode为根目录iNode
		fsbuf[0] |= 1 << i;

	assert(fsbuf[0] == 0x1F);/* 0001 1111 :
				  *    | ||||
				  *    | |||`--- bit 0 : reserved 保留
				  *    | ||`---- bit 1 : the first inode, 对应根目录
				  *    | ||              which indicates `/'
				  *    | |`----- bit 2 : /dev_tty0  3个终端设备
				  *    | `------ bit 3 : /dev_tty1
				  *    `-------- bit 4 : /dev_tty2
				  */
	WR_SECT(ROOT_DEV, 2);  //写入2号扇区块

	/************************/
	/*      secter map      */
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	int nr_sects = NR_DEFAULT_FILE_SECTS + 1;
	/*             ~~~~~~~~~~~~~~~~~~~|~   |
	 *                                |    `--- bit 0 is reserved 0号位保留
	 *                                `-------- for `/'根目录占用扇区数
	 */
    //占用的整数个字节，假设nr_sects = 10个bit
	for (i = 0; i < nr_sects / 8; i++)
		fsbuf[i] = 0xFF;
    //剩余的bit位=2，i=1
	for (j = 0; j < nr_sects % 8; j++)
		fsbuf[i] |= (1 << j);
    //写入3号扇区块
	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects);

	/* zeromemory the rest sector-map */
    //将sector-map其余的扇区中的位初始化为0
	memset(fsbuf, 0, SECTOR_SIZE);
	for (i = 1; i < sb.nr_smap_sects; i++)
		WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + i);

	/************************/
	/*       inodes          */
    /*      共包含根目录、`dev_tty0', `dev_tty1', `dev_tty2'的iNode */
	/************************/
	/* inode of `/'根目录 */
	memset(fsbuf, 0, SECTOR_SIZE);
    //每个元素是iNode类型
	struct inode * pi = (struct inode*)fsbuf;
    //文件模式：目录模式
	pi->i_mode = I_DIRECTORY;
	pi->i_size = DIR_ENTRY_SIZE * 4; /* 目前有4 files:
					  * `.',当前目录文件
					  * `dev_tty0', `dev_tty1', `dev_tty2',
					  */
	pi->i_start_sect = sb.n_1st_sect;
	pi->i_nr_sects = NR_DEFAULT_FILE_SECTS;
	/* inode of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++) {
		pi = (struct inode*)(fsbuf + (INODE_SIZE * (i + 1)));
		pi->i_mode = I_CHAR_SPECIAL;  //字符设备特殊文件
		pi->i_size = 0;
		pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);   //文件代表设备的设备号
		pi->i_nr_sects = 0; //在磁盘上不占用空间，只占有一个iNode节点空间
	}
    //写入到iNode array所在的第一个扇区
	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + sb.nr_smap_sects);
    //只需要初始化第一个扇区即可，其余扇区的值不重要，
    //因为有多少文件，是由iNode_map和root根目录文件决定的

	/************************/
	/*          `/'         */
    //根目录初始化
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry * pde = (struct dir_entry *)fsbuf;

	pde->inode_nr = 1;  //根目录的iNode号为1，这里以inode map中bit的索引为准，0号保留
	strcpy(pde->name, ".");

	/* dir entries of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++) {
		pde++;
		pde->inode_nr = i + 2; /* dev_tty0's inode_nr is 2 */
		sprintf(pde->name, "dev_tty%d", i);
	}
    //写入到根目录的第一个扇区
	WR_SECT(ROOT_DEV, sb.n_1st_sect);
}

/*****************************************************************************
 *                                rw_sector
 *                          文件系统向用户进程提供的读写磁盘接口，
 *                          是通过向磁盘驱动进程发送读写消息实现的
 *****************************************************************************/
/**
 * <Ring 1> R/W a sector via messaging with the corresponding driver.
 *
 * @param io_type  DEV_READ or DEV_WRITE  磁盘驱动提供的读写类型
 * @param dev      device nr  //分区次设备号
 * @param pos      Byte offset from/to where to r/w.相对于分区第一个扇区的偏移字节
 * @param bytes    r/w count in bytes.
 * @param proc_nr  To whom the buffer belongs. 谁发送消息的进程pid
 * @param buf      r/w buffer.  属于进程的读写缓冲区
 *
 * @return Zero if success.
 *****************************************************************************/
PUBLIC int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr,
		     void* buf)
{
	MESSAGE driver_msg;

	driver_msg.type		= io_type;
	driver_msg.DEVICE	= MINOR(dev);
	driver_msg.POSITION	= pos;
	driver_msg.BUF		= buf;
	driver_msg.CNT		= bytes;
	driver_msg.PROC_NR	= proc_nr;
	assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);

	return 0;
}
