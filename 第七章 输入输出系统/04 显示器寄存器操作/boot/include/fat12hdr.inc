;FAT12磁盘头部信息
BS_OEMNAME      db  'Billion '      ;厂商名，必须8字节
BPB_BytesPerSec dw 512              ;每个扇区字节数
BPB_SecPerClus  db 1                ;每簇多少扇区
BPB_RsvdSecCnt  dw 1                ;Boot引导代码占用多少扇区
BPB_NumFATs     db 2                ;共有多少个FAT表
BPB_RootEntCnt  dw 224              ;根目录文件最大数
BPB_TotSec16    dw 2880             ;逻辑扇区总数
BPB_Media       db  0xF0            ;媒体描述符
BPB_FATSz16     dw 9                ;每个FAT占用扇区数
BPB_SecPerTrk   dw 18               ;每个磁道扇区数
BPB_NumHeads    dw  2               ;磁头数
BPB_HiddSec     dd 0                ;隐藏扇区数
BPB_TotSec32    dd 0                ;当BPB_TotSec16为0时用BPB_TotSec32表示
BS_DrvNum       db 0                ;中断13的驱动器号
BS_Reservedl    db 0                ;保留
BS_BootSig      db 29h              ;扩展引导标记
BS_VolID        dd 0                ;卷序列号
BS_VolLab       db  'Billion0.01'   ;卷标，必须11字节
BS_FileSysType  db 'FAT12   '       ;文件系统类型，必须8字节

;=========================================================================
;基于FAT12头部的一些常量定义，如果头部信息改变，下面的常量也要做出改变
;=========================================================================
;BPB_FATSz16
FATSz               equ     9
;根目录占用空间：BPB_RootEntCnt、BPB_BytesPerSec
RootDirSectors      equ     14
;根目录的第一个扇区号：BPB_RsvdSecCnt、BPB_NumFATs、BPB_FATSz16
SectorNoOfRootDir   equ 19
;FAT1的第一个扇区号：BPB_RsvdSecCnt
SectorNoOfFAT1      equ     1
;用于计算簇号对应扇区号：BPB_RsvdSecCnt、BPB_NumFATs、BPB_FATSz16
DeltaSectorNo       equ     17