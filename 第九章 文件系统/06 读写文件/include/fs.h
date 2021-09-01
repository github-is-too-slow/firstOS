#ifndef _ORANGES_FS_H_
#define _ORANGES_FS_H_
/*驱动程序的封装*/
struct dev_drv_map {
	int driver_nr; /**< The proc nr.\ of the device driver. */
};

/**
 * @def   MAGIC_V1
 * @brief Magic number of FS v1.0
 * 文件系统版本号
 */
#define	MAGIC_V1	0x111

/**
 * @struct super_block fs.h "include/fs.h"
 * @brief  The 2nd sector of the FS
 * 超级块,占用一个扇区
 * Remember to change SUPER_BLOCK_SIZE if the members are changed.
 */
struct super_block {
	u32	magic;		  /**< Magic number */
	u32	nr_inodes;	  /**< How many inodes */
	u32	nr_sects;	  /**< How many sectors */
	u32	nr_imap_sects;	  /**< How many inode-map sectors */
	u32	nr_smap_sects;	  /**< How many sector-map sectors */
	u32	n_1st_sect;	  /**< Number of the 1st data sector 数据区*/
	u32	nr_inode_sects;   /**< How many inode sectors */
	u32	root_inode;       /**< Inode nr of root directory 根目录iNode号*/
	u32	inode_size;       /**< INODE_SIZE */
	u32	inode_isize_off;  /**< Offset of `struct inode::i_size' */
	u32	inode_start_off;  /**< Offset of `struct inode::i_start_sect' */
	u32	dir_ent_size;     /**< DIR_ENTRY_SIZE根目录项大小 */
	u32	dir_ent_inode_off;/**< Offset of `struct dir_entry::inode_nr' */
	u32	dir_ent_fname_off;/**< Offset of `struct dir_entry::name' */

	/*
	 * the following item(s) are only present in memory
	 */
	int	sb_dev; 	/**< the super block's home device 超级块所属的设备号*/
};

/**
 * @def   SUPER_BLOCK_SIZE
 * @brief The size of super block \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 */
#define	SUPER_BLOCK_SIZE	56  //磁盘上的大小

/**
 * @struct inode
 * @brief  i-node
 * 索引节点，用于索引一个文件的起始扇区号和扇区数
 * The \c start_sect and\c nr_sects locate the file in the device,
 * and the size show how many bytes is used.
 * If <tt> size < (nr_sects * SECTOR_SIZE) </tt>, the rest bytes
 * are wasted and reserved for later writing.
 *
 * \b NOTE: Remember to change INODE_SIZE if the members are changed
 */
struct inode {
	u32	i_mode;		/**< Accsess mode 文件模式，区分普通文件和特殊文件*/
	u32	i_size;		/**< File size文件大小字节 */
	u32	i_start_sect;	/**< The first sector of the data */
	u32	i_nr_sects;	/**< How many sectors the file occupies */
	u8	_unused[16];	/**< Stuff for alignment 对齐32字节*/

	/* the following items are only present in memory */
	int	i_dev;		//inode所在设备号
	int	i_cnt;		/**< How many procs share this inode 打开进程数 */
	int	i_num;		/**< inode nr. 对应的iNode号 */
};

/**
 * @def   INODE_SIZE
 * @brief The size of i-node stored \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 */
#define	INODE_SIZE	32

/**
 * @def   MAX_FILENAME_LEN
 * @brief Max len of a filename
 * @see   dir_entry
 */
#define	MAX_FILENAME_LEN	12

/**
 * @struct dir_entry
 * @brief  Directory Entry
 * 根目录项，用于索引一个文件的inode
 */
struct dir_entry {
	int	inode_nr;		/**< inode nr. iNode号*/
	char	name[MAX_FILENAME_LEN];	/**< Filename 文件名*/
};

/**
 * @def   DIR_ENTRY_SIZE
 * @brief The size of directory entry in the device.
 *
 * It is as same as the size in memory.
 */
#define	DIR_ENTRY_SIZE	sizeof(struct dir_entry)

/**
 * Since all invocations of `rw_sector()' in FS look similar (most of the
 * params are the same), we use this macro to make code more readable.
 * 读写磁盘扇区，只需要一个设备号和一个该设备上的扇区号即可
 * 读的时候默认将扇区中的数据读取fsbuf缓冲区中
 * 写的时候默认将fsbuf的前512字节写入扇区中
 */
#define RD_SECT(dev,sect_nr) rw_sector(DEV_READ, dev, (sect_nr) * SECTOR_SIZE, SECTOR_SIZE, TASK_FS, fsbuf)
#define WR_SECT(dev,sect_nr) rw_sector(DEV_WRITE, dev, (sect_nr) * SECTOR_SIZE, SECTOR_SIZE, TASK_FS, fsbuf)

/*文件描述符*/
struct file_desc
{
    int fd_mode;        //注意与i_mode不同，i_mode表示文件类型，fd_mode表示操作模式
    int fd_pos;         //当前读写位置，相对于文件起始位置
    struct inode *fd_inode;     //对应文件的inode数据结构
};
#endif