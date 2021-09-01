/**
 * 文件系统模块主程序
 **/
#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "string.h"
#include "protect.h"
#include "fs.h"
#include "stdio.h"
#include "process.h"
#include "hd.h"
#include "keyboard.h"
#include "global.h"
#include "proto.h"

PRIVATE void init_fs();
PRIVATE void mkfs();
PRIVATE void read_super_block(int dev);
PRIVATE int fs_fork();
PRIVATE int fs_exit();

PUBLIC void task_fs(){
    printf("Task FS begins.\n");
    //初始化文件系统,并且完成内存中f_desc_table[]、inode_table[]等的初始化
    init_fs();
    while(1){
		send_recv(RECEIVE, ANY, &fs_msg);

		int src = fs_msg.source;
		//进程表
		pcaller = &proc_table[src];

		switch (fs_msg.type) {
		case OPEN:
			fs_msg.FD = do_open();
			break;
		case CLOSE:
			fs_msg.RETVAL = do_close();
			break;
		case READ:
		case WRITE:
			fs_msg.CNT = do_rdwt();
			break;
		case UNLINK:
			fs_msg.RETVAL = do_unlink();
			break;
		case RESUME_PROC:
			src = fs_msg.PROC_NR;  //向用户进程发送一个消息，等价于解除了该进程
			break;
		case SUSPEND_PROC:
			break;
		case FORK:
			fs_msg.RETVAL = fs_fork();
			break;
		case EXIT:
			fs_msg.RETVAL = fs_exit();
			break;
		default:
			// dump_msg("FS::unknown message:", &fs_msg);
			assert(0);
			break;
		}
		/* reply 接收到tty发送的SUSPEND_PROC消息后，不想用户进程发送消息*/
		//这样就等价于阻塞了用户进程，
		//而接受到SUSPEND_PROC，表明读取键盘并传送工作已经完成，此时可以解除阻塞
		if (fs_msg.type != SUSPEND_PROC) {
			fs_msg.type = SYSCALL_RET; //文件系统统一回复类型
			send_recv(SEND, src, &fs_msg);
		}
	}
}

/**
 * 初始化文件系统
 **/
PRIVATE void init_fs()
{
	int i;
	/* f_desc_table[] 文件描述符表*/
	for (i = 0; i < NR_FILE_DESC; i++)
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));

	/* inode_table[] */
	for (i = 0; i < NR_INODE; i++)
		memset(&inode_table[i], 0, sizeof(struct inode));

	/* super_block[] */
	struct super_block * sb = super_block;
	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
		sb->sb_dev = NO_DEV;

	/* open the device: hard disk 这里仅输出整块驱动器上的分区信息*/
	MESSAGE driver_msg;
	driver_msg.type = DEV_OPEN;
	driver_msg.DEVICE = MINOR(ROOT_DEV);
	assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);

	//制作文件系统，并开始监听用户进程消息
	mkfs();

	/* load super block of ROOT，放入到super_block表中 */
	read_super_block(ROOT_DEV);
	//读取超级块，只是起到检测的作用
	sb = get_super_block(ROOT_DEV);
	assert(sb->magic == MAGIC_V1);
	//读取根目录的iNode，赋值给全局变量
	root_inode = get_inode(ROOT_DEV, ROOT_INODE);
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
	/*      super block      */
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
	for (i = 0; i < (NR_CONSOLES + 3); i++)  //0号iNode保留，1号iNode为根目录iNode
		fsbuf[0] |= 1 << i;

	assert(fsbuf[0] == 0x3F);/* 0011 1111 :
				  *    | ||||
				  *    | |||`--- bit 0 : reserved 保留
				  *    | ||`---- bit 1 : the first inode, 对应根目录
				  *    | ||              which indicates `/'
				  *    | |`----- bit 2 : /dev_tty0  3个终端设备
				  *    | `------ bit 3 : /dev_tty1
				  *    `-------- bit 4 : /dev_tty2
				  * 	---------bit 5 : /cmd.tar	多创建了一个压缩文件
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
	//在指定起始扇区写入cmd.tar
	/* cmd.tar */
	//起始扇区的在映射表中的偏移bit
	int bit_offset = INSTALL_START_SECT -
		sb.n_1st_sect + 1; /* sect M <-> bit (M - sb.n_1stsect + 1) */
	int bit_off_in_sect = bit_offset % (SECTOR_SIZE * 8);
	int bit_left = INSTALL_NR_SECTS;
	int cur_sect = bit_offset / (SECTOR_SIZE * 8);
	RD_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + cur_sect);
	while (bit_left) {
		//在当前扇区的偏移字节
		int byte_off = bit_off_in_sect / 8;
		/* this line is ineffecient in a loop, but I don't care */
		//先改变当前bit
		fsbuf[byte_off] |= 1 << (bit_off_in_sect % 8);
		bit_left--;
		//在当前扇区中的偏移bit加1
		bit_off_in_sect++;
		//如果偏移bit=512*8，就说明该扇区已经到达末尾了，需要读取下一个扇区
		if (bit_off_in_sect == (SECTOR_SIZE * 8)) {
			WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + cur_sect);
			cur_sect++;
			RD_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + cur_sect);
			bit_off_in_sect = 0;
		}
	}
	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + cur_sect);

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
	pi->i_size = DIR_ENTRY_SIZE * 5; /* 目前有4 files:
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
	/*/cmd.tar*/
	pi = (struct inode*)(fsbuf + (INODE_SIZE * (NR_CONSOLES + 1)));
	pi->i_mode = I_REGULAR;  //常规文件
	//文件大小都写满
	pi->i_size = INSTALL_NR_SECTS * SECTOR_SIZE;
	//起始扇区号，这个很重要
	pi->i_start_sect = INSTALL_START_SECT;
	pi->i_nr_sects = INSTALL_NR_SECTS;
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
	//cmd.tar 的inode号和文件名
	(++pde)->inode_nr = NR_CONSOLES + 2;
	strcpy(pde->name, "cmd.tar");
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

/*****************************************************************************
 *                                read_super_block
 *****************************************************************************/
/**
 * <Ring 1> Read super block from the given device then write it into a free
 *          super_block[] slot.
 *
 * @param dev  From which device设备号 the super block comes.
 *****************************************************************************/
PRIVATE void read_super_block(int dev)
{
	int i;
	MESSAGE driver_msg;

	driver_msg.type		= DEV_READ;
	driver_msg.DEVICE	= MINOR(dev);
	driver_msg.POSITION	= SECTOR_SIZE * 1;
	driver_msg.BUF		= fsbuf;
	driver_msg.CNT		= SECTOR_SIZE;
	driver_msg.PROC_NR	= TASK_FS;
	assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);

	/* find a free slot in super_block[] */
	for (i = 0; i < NR_SUPER_BLOCK; i++)
		if (super_block[i].sb_dev == NO_DEV)
			break;
	if (i == NR_SUPER_BLOCK)
		panic("super_block slots used up");

	assert(i == 0); /* currently we use only the 1st slot */

	struct super_block * psb = (struct super_block *)fsbuf;

	super_block[i] = *psb;  //结构体整体赋值
	super_block[i].sb_dev = dev;  //超级块所属设备号
}


/*****************************************************************************
 *                                get_super_block
 *****************************************************************************/
/**
 * <Ring 1> Get the super block from super_block[].
 *
 * @param dev Device nr.
 *
 * @return Super block ptr.
 *****************************************************************************/
PUBLIC struct super_block * get_super_block(int dev)
{
	struct super_block * sb = super_block;
	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
		if (sb->sb_dev == dev)
			return sb;

	panic("super block of devie %d not found.\n", dev);

	return 0;  //有效设备号大于0
}


/*****************************************************************************
 *                                get_inode
 *****************************************************************************/
/**
 * <Ring 1> Get the inode ptr of given inode nr. A cache -- inode_table[] -- is
 * maintained to make things faster. If the inode requested is already there,
 * just return it. Otherwise the inode will be read from the disk.
 * 获取iNode号对应的iNode指针，要么从内存中获取，要么从磁盘中加载到缓存中再返回
 * @param dev Device nr.
 * @param num I-node nr.
 *
 * @return The inode ptr requested.
 *****************************************************************************/
PUBLIC struct inode * get_inode(int dev, int num)
{
	if (num == 0)
		return 0;

	struct inode * p;
	struct inode * q = 0;
	for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++) {
		if (p->i_cnt) {	/* not a free slot 说明有进程正在使用，那么它的inode此时是有效的*/
			if ((p->i_dev == dev) && (p->i_num == num)) {
				/* this is the inode we want */
				p->i_cnt++;
				return p;
			}
		}
		else {		/* a free slot 此时这个inode没有进程正在使用，可以被覆盖*/
			if (!q) /* q hasn't been assigned yet */
				q = p; /* q <- the 1st free slot */
		}
	}

	if (!q)
		panic("the inode table is full");

	q->i_dev = dev;
	q->i_num = num;
	q->i_cnt = 1;

	struct super_block * sb = get_super_block(dev);
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects +
		((num - 1) / (SECTOR_SIZE / INODE_SIZE)); //inode所在扇区
	RD_SECT(dev, blk_nr);
	struct inode * pinode =
		(struct inode*)((u8*)fsbuf +
				((num - 1 ) % (SECTOR_SIZE / INODE_SIZE))
				 * INODE_SIZE);
	q->i_mode = pinode->i_mode;
	q->i_size = pinode->i_size;
	q->i_start_sect = pinode->i_start_sect;
	q->i_nr_sects = pinode->i_nr_sects;
	return q;
}

/*****************************************************************************
 *                                put_inode
 *****************************************************************************/
/**
 * Decrease the reference nr of a slot in inode_table[]. When the nr reaches
 * zero, it means the inode is not used any more and can be overwritten by
 * a new inode.
 * 当一个进程关闭一个文件时，应当将inode的引用减1，当减为0，表明可以覆盖
 * @param pinode I-node ptr.
 *****************************************************************************/
PUBLIC void put_inode(struct inode * pinode)
{
	assert(pinode->i_cnt > 0);
	pinode->i_cnt--;
}

/*****************************************************************************
 *                                sync_inode
 *****************************************************************************/
/**
 * <Ring 1> Write the inode back to the disk. Commonly invoked as soon as the
 *          inode is changed.
 * 当一个文件发生更改时，应当立即将缓存中的inode重写到磁盘中
 * @param p I-node ptr.
 *****************************************************************************/
PUBLIC void sync_inode(struct inode * p)
{
	struct inode * pinode;
	struct super_block * sb = get_super_block(p->i_dev);
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects +
		((p->i_num - 1) / (SECTOR_SIZE / INODE_SIZE));
	RD_SECT(p->i_dev, blk_nr);
	pinode = (struct inode*)((u8*)fsbuf +
				 (((p->i_num - 1) % (SECTOR_SIZE / INODE_SIZE))
				  * INODE_SIZE));
	pinode->i_mode = p->i_mode;
	pinode->i_size = p->i_size;
	pinode->i_start_sect = p->i_start_sect;
	pinode->i_nr_sects = p->i_nr_sects;
	WR_SECT(p->i_dev, blk_nr);
}

/*****************************************************************************
 *                                fs_fork
 * 解决创建一个子进程时文件共享的问题
 *****************************************************************************/
/**
 * Perform the aspects of fork() that relate to files.
 *
 * @return Zero if success, otherwise a negative integer.
 *****************************************************************************/
PRIVATE int fs_fork()
{
	int i;
	PROCESS* child = &proc_table[fs_msg.PID];
	for (i = 0; i < NR_FILES; i++) {
		if (child->filp[i]) {
			//文件描述符引用和inode引用均加1
			child->filp[i]->fd_cnt++;
			child->filp[i]->fd_inode->i_cnt++;
		}
	}
	return 0;
}

/*****************************************************************************
 *                                fs_exit
 *****************************************************************************/
/**
 * Perform the aspects of exit() that relate to files.
 * 当一个子进程退出时，该函数会执行一个退出相关的文件处理操作
 * @return Zero if success.
 *****************************************************************************/
PRIVATE int fs_exit()
{
	int i;
	PROCESS *p = &proc_table[fs_msg.PID];
	for (i = 0; i < NR_FILES; i++) {
		if (p->filp[i]) {
			/* release the inode引用 */
			p->filp[i]->fd_inode->i_cnt--;
			/* release the file desc slot父子进程是共享同一个文件描述符的 */
			if (--p->filp[i]->fd_cnt == 0)
				p->filp[i]->fd_inode = 0;  //inode指针置0，此后寻找文件描述符空闲插销是根据fd_inode = 0判断的
			p->filp[i] = 0;
		}
	}
	return 0;
}
