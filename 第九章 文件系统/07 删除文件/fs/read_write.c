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

/*****************************************************************************
 *                                do_rdwt
 *****************************************************************************/
/**
 * Read/Write file and return byte count read/written.
 * 读写文件，并返回读写字节数
 * Sector map is not needed to update, since the sectors for the file have been
 * allocated and the bits are set when the file was created.
 *
 * @return How many bytes have been read/written.
 *****************************************************************************/
PUBLIC int do_rdwt()
{
	int fd = fs_msg.FD;	/**< file descriptor. */
	void * buf = fs_msg.BUF;/**< r/w buffer */
	int len = fs_msg.CNT;	/**< r/w bytes字节数 */

	int src = fs_msg.source;		/* caller proc nr. */

	assert((pcaller->filp[fd] >= &f_desc_table[0]) &&
	       (pcaller->filp[fd] < &f_desc_table[NR_FILE_DESC]));

	if (!(pcaller->filp[fd]->fd_mode & O_RDWR)) //读写模式
		return -1;

	int pos = pcaller->filp[fd]->fd_pos;

    //文件inode，包含文件的起始扇区，文件类型、大小等信息
	struct inode * pin = pcaller->filp[fd]->fd_inode;

	assert(pin >= &inode_table[0] && pin < &inode_table[NR_INODE]);

	int imode = pin->i_mode & I_TYPE_MASK;

	if (imode == I_CHAR_SPECIAL) {//读写字符特殊设备
		int t = fs_msg.type == READ ? DEV_READ : DEV_WRITE;
		fs_msg.type = t;

		int dev = pin->i_start_sect;  //获取设备号
		assert(MAJOR(dev) == 4);

		fs_msg.DEVICE	= MINOR(dev);
		fs_msg.BUF	= buf;
		fs_msg.CNT	= len;
		fs_msg.PROC_NR	= src;  //请求进程pid
		assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
		send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &fs_msg);
		assert(fs_msg.CNT == len);

		return fs_msg.CNT;
	}
	else {
		assert(pin->i_mode == I_REGULAR || pin->i_mode == I_DIRECTORY);
		assert((fs_msg.type == READ) || (fs_msg.type == WRITE));

		int pos_end;
		if (fs_msg.type == READ)//读取不能超过文件已有大小
			pos_end = min(pos + len, pin->i_size);
		else		/* WRITE 写入时不能超过文件最大长度*/
			pos_end = min(pos + len, pin->i_nr_sects * SECTOR_SIZE);
        //相当于从rw_sect_min号的off位置开始读写，直至rw_sect_max号扇区中的某个位置
		int off = pos % SECTOR_SIZE;
		int rw_sect_min=pin->i_start_sect+(pos >> SECTOR_SIZE_SHIFT);
		int rw_sect_max=pin->i_start_sect+(pos_end >> SECTOR_SIZE_SHIFT);
        //这是读写的单位(扇区数)，最大不超过fsbuf分配的空间(这里是1M)
        //以下操作每次直接将fsbuf中chunk个扇区读出或写入
		int chunk = min(rw_sect_max - rw_sect_min + 1,
				FSBUF_SIZE >> SECTOR_SIZE_SHIFT);

		int bytes_rw = 0;  //请求进程的读写位置
		int bytes_left = len;
		int i;
		for (i = rw_sect_min; i <= rw_sect_max; i += chunk) {
			/* read/write this amount of bytes every time */
            //每次读写的字节数
			int bytes = min(bytes_left, chunk * SECTOR_SIZE - off);
            //不管读写都先读出来
			rw_sector(DEV_READ,
				  pin->i_dev,
				  i * SECTOR_SIZE,
				  chunk * SECTOR_SIZE,
				  TASK_FS,
				  fsbuf);
			if (fs_msg.type == READ) {
				memcpy((void*)va2la(src, buf + bytes_rw),
					  (void*)va2la(TASK_FS, fsbuf + off),
					  bytes);
			}
			else {	/* WRITE */
				memcpy((void*)va2la(TASK_FS, fsbuf + off),
					  (void*)va2la(src, buf + bytes_rw),
					  bytes);
                //重新写入到磁盘
				rw_sector(DEV_WRITE,
					  pin->i_dev,
					  i * SECTOR_SIZE,
					  chunk * SECTOR_SIZE,
					  TASK_FS,
					  fsbuf);
			}
			off = 0;  //第一块起始磁盘是有偏移的，其余块都是整数块
			bytes_rw += bytes;  //每次更新读写位置
			pcaller->filp[fd]->fd_pos += bytes;
			bytes_left -= bytes;
		}
        //追加文件，需要同步inode
		if (pcaller->filp[fd]->fd_pos > pin->i_size) {
			/* update inode::size */
			pin->i_size = pcaller->filp[fd]->fd_pos;

			/* write the updated i-node back to disk */
			sync_inode(pin);
		}
		return bytes_rw;
	}
}