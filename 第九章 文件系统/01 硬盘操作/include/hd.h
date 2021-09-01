#define REG_STATUS	0x1F7  //既是状态寄存器，又是命令寄存器
#define REG_CMD		REG_STATUS
#define	STATUS_BSY	0x80  //BSY位位于最高位
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


