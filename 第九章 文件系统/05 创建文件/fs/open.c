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

PRIVATE struct inode * create_file(char * path, int flags);
PRIVATE int alloc_imap_bit(int dev);
PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc);
PRIVATE struct inode * new_inode(int dev, int inode_nr, int start_sect);
PRIVATE void new_dir_entry(struct inode * dir_inode, int inode_nr, char * filename);

/*****************************************************************************
 *                                do_open
 * 打开文件所需做的事情：
 * 1.判断文件是否存在，
 * 2.若不存在，创建文件，进入4
 * 3.若存在，直接进入4
 * 4.获取文件的iNode，并且建立进程表、文件描述符表、iNode表的3表关系
 *****************************************************************************/
/**
 * Open a file and return the file descriptor.
 *
 * @return File descriptor if successful, otherwise a negative error code-1.
 *****************************************************************************/
PUBLIC int do_open()
{
	int fd = -1;		/* return value 默认失败*/

	char pathname[MAX_FILENAME_LEN];

	/* get parameters from the message */
	int flags = fs_msg.FLAGS;	/* access mode */
	int name_len = fs_msg.NAME_LEN;	/* length of filename */
	int src = fs_msg.source;	/* caller proc nr. */
	assert(name_len < MAX_FILENAME_LEN);
	memcpy((void*)va2la(TASK_FS, pathname),
		  (void*)va2la(src, fs_msg.PATHNAME),
		  name_len);
	pathname[name_len] = 0; //字符串

	/* find a free slot in PROCESS::filp[] */
	int i;
	for (i = 0; i < NR_FILES; i++) {
		if (pcaller->filp[i] == 0) {//请求用户进程的filp，里面存放着fd的指针
			fd = i; //找到了
			break;
		}
	}
	if ((fd < 0) || (fd >= NR_FILES))
		panic("filp[] is full (PID:%d)", proc2pid(pcaller));

	/* find a free slot in f_desc_table[] */
	for (i = 0; i < NR_FILE_DESC; i++)
		if (f_desc_table[i].fd_inode == 0)  //找到了
			break;
	if (i >= NR_FILE_DESC)
		panic("f_desc_table[] is full (PID:%d)", proc2pid(pcaller));
    //直接从磁盘根目录区找文件，返回文件名对应的iNode号，iNode号从1开始
	int inode_nr = search_file(pathname);

	struct inode * pin = 0;
	if (flags & O_CREATE) {
		if (inode_nr) {//创建文件但文件存在，inode_nr>=1
			printf("file exists.\n");
			return -1;
		}
		else {
            //在物理磁盘上创建文件，并且在内存中分配iNode镜像
			pin = create_file(pathname, flags);
		}
	}
	else {//不是创建就是读写，而读写时文件必定假定已经存在了
		assert(flags & O_RDWR);

		char filename[MAX_FILENAME_LEN];
		struct inode * dir_inode; //根目录iNode
        //分解出路径，得到文件名
		if (strip_path(filename, pathname, &dir_inode) != 0)
			return -1;
        //要么从iNode表中获取iNode，要么从磁盘中加载相应的iNode
		pin = get_inode(dir_inode->i_dev, inode_nr);
	}

	if (pin) {//此时要么是新创建文件的iNode，要么是旧文件的iNode
        //打开操作，就是连接3个数组filp/f_desc_table/inode_table的指向关系
		/* connects proc with file_descriptor 连接操作1*/
		pcaller->filp[fd] = &f_desc_table[i];

		/* connects file_descriptor with inode 连接操作2*/
		f_desc_table[i].fd_inode = pin;

		f_desc_table[i].fd_mode = flags; //操作模式
		/* f_desc_table[i].fd_cnt = 1; */
		f_desc_table[i].fd_pos = 0; //初始读写指针指向文件开头

		int imode = pin->i_mode & I_TYPE_MASK;
        //如果是字符特殊设备，交给相应的驱动程序
        //目前进不来
		if (imode == I_CHAR_SPECIAL) {
			MESSAGE driver_msg;

			driver_msg.type = DEV_OPEN;
			int dev = pin->i_start_sect; //设备号
			driver_msg.DEVICE = MINOR(dev);  //次设备号
			assert(MAJOR(dev) == 4);    //主设备号
			assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);

			send_recv(BOTH,
				  dd_map[MAJOR(dev)].driver_nr,
				  &driver_msg);
		}
		else if (imode == I_DIRECTORY) {//根目录
			assert(pin->i_num == ROOT_INODE);
		}
		else {//普通文件
			assert(pin->i_mode == I_REGULAR);
		}
	}
	else {
		return -1;
	}

	return fd;
}

/*****************************************************************************
 *                                create_file
 *创建文件所需做的事情
 *1.为文件内容分配扇区
 *2.在iNode_array中分配一个i-node
 *3.在inode-map中分配一位
 *4.在sector-map中分配一位或多位
 *5.在相应目录中写入一个目录项
 *****************************************************************************/
/**
 * Create a file and return it's inode ptr.返回iNode指针
 *
 * @param[in] path   The full path of the new file
 * @param[in] flags  Attribiutes of the new file
 *
 * @return           Ptr to i-node of the new file if successful, otherwise 0.
 *
 * @see open()
 * @see do_open()
 *****************************************************************************/
PRIVATE struct inode * create_file(char * path, int flags)
{
	char filename[MAX_FILENAME_LEN];
	struct inode * dir_inode;
    //提取文件名
	if (strip_path(filename, path, &dir_inode) != 0)
		return 0;

	int inode_nr = alloc_imap_bit(dir_inode->i_dev);

	int free_sect_nr = alloc_smap_bit(dir_inode->i_dev,
					  NR_DEFAULT_FILE_SECTS);

	struct inode *newino = new_inode(dir_inode->i_dev, inode_nr,
					 free_sect_nr);

	new_dir_entry(dir_inode, newino->i_num, filename);

	return newino;
}

/*****************************************************************************
 *                                do_close
 *****************************************************************************/
/**
 * Handle the message CLOSE.
 * 关闭文件所需做的工作：
 * 1.将对应inode的cnt进程引用数减去1，若减至0，表明可以被其他进程的inode引用
 * 2.断开3表连接
 * @return Zero if success.
 *****************************************************************************/
PUBLIC int do_close()
{
	int fd = fs_msg.FD;
	put_inode(pcaller->filp[fd]->fd_inode);
	pcaller->filp[fd]->fd_inode = 0;
	pcaller->filp[fd] = 0;

	return 0;
}

/*****************************************************************************
 *                                alloc_imap_bit
 *****************************************************************************/
/**
 * Allocate a bit in inode-map.
 *
 * @param dev  In which device the inode-map is located.
 *
 * @return  I-node nr.返回分配的inode号
 *****************************************************************************/
PRIVATE int alloc_imap_bit(int dev)
{
	int inode_nr = 0;
	int i, j, k;

	int imap_blk0_nr = 1 + 1; /* 1 boot sector & 1 super block */
	struct super_block * sb = get_super_block(dev);

	for (i = 0; i < sb->nr_imap_sects; i++) {
		RD_SECT(dev, imap_blk0_nr + i);

		for (j = 0; j < SECTOR_SIZE; j++) {
			/* skip `11111111' bytes */
			if (fsbuf[j] == 0xFF)
				continue;

			/* skip `1' bits */
			for (k = 0; ((fsbuf[j] >> k) & 1) != 0; k++) {}

			/* i: sector index; j: byte index; k: bit index */
			inode_nr = (i * SECTOR_SIZE + j) * 8 + k;
			fsbuf[j] |= (1 << k);   //置1，表示分配了一个inode

			/* write the bit to imap */
			WR_SECT(dev, imap_blk0_nr + i);
			break;
		}

		return inode_nr;
	}

	/* no free bit in imap */
	panic("inode-map is probably full.\n");

	return 0;
}

/*****************************************************************************
 *                                alloc_smap_bit
 *****************************************************************************/
/**
 * Allocate a bit in sector-map.
 *
 * @param dev  In which device the sector-map is located.
 * @param nr_sects_to_alloc  How many sectors are allocated.
 *
 * @return  The 1st sector nr allocated.返回起始扇区号
 *****************************************************************************/
PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc)
{
	/* int nr_sects_to_alloc = NR_DEFAULT_FILE_SECTS; */

	int i; /* sector index */
	int j; /* byte index */
	int k; /* bit index */

	struct super_block * sb = get_super_block(dev);

	int smap_blk0_nr = 1 + 1 + sb->nr_imap_sects;
	int free_sect_nr = 0;

	for (i = 0; i < sb->nr_smap_sects; i++) { /* smap_blk0_nr + i :
						     current sect nr. */
		RD_SECT(dev, smap_blk0_nr + i);

		/* byte offset in current sect */
		for (j = 0; j < SECTOR_SIZE && nr_sects_to_alloc > 0; j++) {
			k = 0;
			if (!free_sect_nr) {//表明未找到空闲起始扇区
				/* loop until a free bit is found */
				if (fsbuf[j] == 0xFF) continue;
				for (; ((fsbuf[j] >> k) & 1) != 0; k++) {}
				free_sect_nr = (i * SECTOR_SIZE + j) * 8 +
					k - 1 + sb->n_1st_sect;  //得到空闲的起始扇区号
			}
            //已经找到了空闲起始扇区，正在分配后续扇区
			for (; k < 8; k++) { /* repeat till enough bits are set */
				assert(((fsbuf[j] >> k) & 1) == 0);
				fsbuf[j] |= (1 << k);  //每轮分配8个扇区
				if (--nr_sects_to_alloc == 0)
					break;
			}
		}
        //一个扇区已经分配完毕
		if (free_sect_nr) /* free bit found, write the bits to smap */
			WR_SECT(dev, smap_blk0_nr + i);

		if (nr_sects_to_alloc == 0)
			break;
	}

	assert(nr_sects_to_alloc == 0);

	return free_sect_nr;
}

/*****************************************************************************
 *                                new_inode
 *****************************************************************************/
/**
 * Generate a new i-node and write it to disk.
 * 在内存的inode table缓冲区中分配一个新inode，并将其内容写入磁盘中
 * @param dev  Home device of the i-node.
 * @param inode_nr  I-node nr.
 * @param start_sect  Start sector of the file pointed by the new i-node.
 *
 * @return  Ptr of the new i-node.
 *****************************************************************************/
PRIVATE struct inode * new_inode(int dev, int inode_nr, int start_sect)
{
	struct inode * new_inode = get_inode(dev, inode_nr);

	new_inode->i_mode = I_REGULAR;  //普通文件
	new_inode->i_size = 0;
	new_inode->i_start_sect = start_sect;
	new_inode->i_nr_sects = NR_DEFAULT_FILE_SECTS;

	new_inode->i_dev = dev;
	new_inode->i_cnt = 1;
	new_inode->i_num = inode_nr;   //inode号

	/* write to the inode array */
    //同步写入磁盘对应的inode区
	sync_inode(new_inode);

	return new_inode;
}

/*****************************************************************************
 *                                new_dir_entry
 *****************************************************************************/
/**
 * Write a new entry into the directory.
 *
 * @param dir_inode  I-node of the directory.
 * @param inode_nr   I-node nr of the new file.
 * @param filename   Filename of the new file.
 *****************************************************************************/
PRIVATE void new_dir_entry(struct inode *dir_inode, int inode_nr, char *filename)
{
	/* write the dir_entry */
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE) / SECTOR_SIZE;
	int nr_dir_entries =
		dir_inode->i_size / DIR_ENTRY_SIZE; /**
						     * including unused slots
						     * (the file has been
						     * deleted but the slot
						     * is still there)
						     */
	int m = 0;
	struct dir_entry * pde;
	struct dir_entry * new_de = 0;

	int i, j;
	for (i = 0; i < nr_dir_blks; i++) {
		RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);

		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
			if (++m > nr_dir_entries)
				break;

			if (pde->inode_nr == 0) { /* it's a free slot */
				new_de = pde;
				break;
			}
		}
		if (m > nr_dir_entries ||/* all entries have been iterated or */
		    new_de)              /* free slot is found */
			break;
	}
	if (!new_de) { /* reached the end of the dir，在dir末尾新增一个entry */
		new_de = pde;
		dir_inode->i_size += DIR_ENTRY_SIZE;
	}
	new_de->inode_nr = inode_nr;
	strcpy(new_de->name, filename);

	/* write dir block -- ROOT dir block */
	WR_SECT(dir_inode->i_dev, dir_blk0_nr + i);

	/* update dir inode 无论dir_inode有没有更改，都进行一次同步*/
	sync_inode(dir_inode);
}