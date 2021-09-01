#ifndef ORANGES_HD_H_
#define ORANGES_HD_H_

#define REG_STATUS	0x1F7  //既是状态寄存器，又是命令寄存器
#define REG_CMD		REG_STATUS
#define	STATUS_BSY	0x80  //状态寄存器BSY位位于最高位，向磁盘发送命令必须检查该位
//状态寄存器请求位位于第4位，向data寄存器写入数据时必须检查
//当该位为1，表明磁盘准备好传输数据了
#define	STATUS_DRQ	0x08
#define	HD_TIMEOUT		10000  //检测超时时间10s
#define REG_DEV_CTRL	0x3F6  //device control寄存器
#define REG_DATA	0x1F0		/*	Data				I/O		*/
#define REG_FEATURES	0x1F1		/*	Features			O		*/
#define REG_ERROR	REG_FEATURES	/*	Error				I		*/
#define REG_NSECTOR	0x1F2		/*	Sector Count			I/O		*/
#define REG_LBA_LOW	0x1F3		/*	Sector Number / LBA Bits 0-7	I/O		*/
#define REG_LBA_MID	0x1F4		/*	Cylinder Low / LBA Bits 8-15	I/O		*/
#define REG_LBA_HIGH	0x1F5		/*	Cylinder High / LBA Bits 16-23	I/O		*/
#define REG_DEVICE	0x1F6
#define SECTOR_SIZE		512
#define ATA_IDENTIFY 0xEC  //IDENTIFY命令
#define ATA_READ		0x20    //读写命令
#define ATA_WRITE		0x30
#define	PARTITION_TABLE_OFFSET	0x1BE  //分区表在扇区中的偏移
#define NO_PART		0x00	/* unused entry 分区类型全0表示没有分区*/
#define EXT_PART	0x05	/* 扩展分区类型号*/
//生成device寄存器的值，
//由LBA模式位:指定操作模式(CHS:柱面/磁头/扇区号定位扇区,LBA:逻辑块地址)
//DRV位:指定主盘或从盘
//低4位:磁头号或LBA的24到27位
#define MAKE_DEVICE_REG(lba, drv, lba_highest) (((lba) << 6) | (drv << 4) | (lba_highest & 0xF) | 0xA0)

//磁盘命令块寄存器数据结构
struct hd_cmd
{
    u8 features;
    u8 sector_count;
    u8 lba_low;
    u8 lba_mid;
    u8 lba_high;
    u8 device;
    u8 command;
};

struct part_info
{
    u32     base;       //一个主分区/扩展分区/逻辑分区的起始扇区号
    u32     size;       //包含多少扇区
};

struct hd_info
{
    int open_cnt;       //当前硬盘打开的次数，也即：当前打开硬盘的进程数
    struct part_info    primary[NR_PRIM_PER_DRIVE];     //4个主分区
    struct part_info    logical[NR_SUB_PER_DRIVE];      //64个逻辑分区
};

//分区表项的组成
struct part_ent {
	u8 boot_ind;		/**
				 * boot indicator，表示是否可引导，
                 * 80h代表可引导，00h不可引导，其他非法
				 *   Bit 7 is the active partition flag,
				 *   bits 6-0 are zero (when not zero this
				 *   byte is also the drive number of the
				 *   drive to boot so the active partition
				 *   is always found on drive 80H, the first
				 *   hard disk).
				 */

	u8 start_head;		/**起始磁头
				 * Starting Head
				 */

	u8 start_sector;	/**起始扇区号和起始柱面号第8.9位
				 * Starting Sector.
				 *   Only bits 0-5 are used. Bits 6-7 are
				 *   the upper two bits for the Starting
				 *   Cylinder field.
				 */

	u8 start_cyl;		/**起始柱面号低8位
				 * Starting Cylinder.
				 *   This field contains the lower 8 bits
				 *   of the cylinder value. Starting cylinder
				 *   is thus a 10-bit number, with a maximum
				 *   value of 1023.
				 */

	u8 sys_id;		/**
				 * System ID分区类型
				 * e.g.
				 *   01: FAT12
				 *   81: MINIX
				 *   83: Linux
				 */

	u8 end_head;		/**
				 * Ending Head结束磁头号
				 */

	u8 end_sector;		/**
				 * Ending Sector.结束扇区号和结束柱面号第8.9位
				 *   Only bits 0-5 are used. Bits 6-7 are
				 *   the upper two bits for the Ending
				 *    Cylinder field.
				 */

	u8 end_cyl;		/**
				 * Ending Cylinder.结束柱面号低8位
				 *   This field contains the lower 8 bits
				 *   of the cylinder value. Ending cylinder
				 *   is thus a 10-bit number, with a maximum
				 *   value of 1023.
				 */

	u32 start_sect;	/**起始扇区的LBA,重要
				 * starting sector counting from
				 * 0 / Relative Sector. / start in LBA
				 */

	u32 nr_sects;		/**扇区数目
				 * nr of sectors in partition
				 */

};
#endif


