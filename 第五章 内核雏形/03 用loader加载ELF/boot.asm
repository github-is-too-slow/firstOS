;=====================================================================
;将Loader和Kernel加载到内存
;对应 5.4.1节
;=======================================================================
;%define _BOOT_DEBUG_
%ifdef _BOOT_DEBUG_
    org 0100h       ;调试状态，编译成.com文件，被DOS系统加载到 段地址:0100h 处
%else
    org 07c00h      ;Boot状态，BIOS将boot sector加载到 0:7c00h 处执行
%endif

;======================堆栈基址设置===============================
%ifdef _BOOT_DEBUG_
BaseOfStack equ 0100h       ;调试状态下堆栈基址
%else
BaseOfStack equ 07c00h      ;Boot状态堆栈基址
%endif
;==============================================================
;=====================================一些常量设置=============================
BaseOfLoader equ 09000h     ;Loader.bin被加载的段地址，注意boot.asm是运行在实模式下的
OffsetOfLoader equ 0100h    ;Loader.bin被加载的偏移地址
MessageLength equ 9     ;显示字符串的固定长度
;===================================================================================

    jmp short LABEL_START           ;start to boot
    nop

;引入软盘头部信息以及相关常量
%include "fat12hdr.inc"

;==============一些变量设置:必须设置在磁盘头部信息之后=======================
wRootDirSizeForLoop dw RootDirSectors
wSectorNo  dw 0     ;要读取的扇区号
bOdd db 0   ;扇区号是奇数还是偶数
;==================================一些显示字符串===============
LoaderFileName db 'LOADER  BIN', 0     ;LOADER.BIN文件名,固定11个字符
BootMessage db 'Booting  '  ;9字节，序号为0
Message1    db 'Ready.   '  ;9字节，序号为1
Message2    db 'No Loader'  ;9字节，序号为2
;===========================================================================

LABEL_START:
    mov ax, cs          ;堆栈所在段与boot.asm的段在同一段下，即段地址均一样
    mov ds, ax          ;因此在调试状态下该段地址DOS系统决定，因为它负责加载boot.com编译文件
    mov es, ax          ;Boot状态下由BIOS系统决定，段地址默认是0
    mov ss, ax
    mov sp, BaseOfStack     ;设置栈基址

    ;清屏
    mov ax, 0600h       ;AH=6, AL=0,代表滚动的行(同时赋给CH/CL/DH/DL)
    mov bx, 0700h       ;BH=07h,黑底白字
    mov cx, 0           ;左上角，CH=0,代表top,CL=0,代表left
    mov dx, 0184Fh      ;右下角，DH=0,代表bottom,DL=0,代表right
    int 10h

    mov dh, 0
    call DispStr    ;"Booting  ", 开始引导了

    ;软驱复位
    xor ah, ah
    xor dl, dl
    int 13h
    ;在软盘A的根目录寻找LOADER.BIN
    mov word [wSectorNo], SectorNoOfRootDir  ;将根目录起始扇区号赋给要读取的扇区号
LABEL_SEARCH_IN_ROOT_DIR_BEGIN:
    cmp word [wRootDirSizeForLoop], 0 ;判断根目录是否读取完毕
    jz LABEL_NO_LOADERBIN   ;读取完毕还是没有找loader.bin文件
    dec word [wRootDirSizeForLoop]
    mov ax, BaseOfLoader
    mov es, ax
    mov bx, OffsetOfLoader   ;暂时使用loader.bin加载的目标地址
    mov ax, [wSectorNo]     ;要读取的根目录占用的某个扇区号
    mov cl, 1
    call ReadSector ;此时指定扇区内容在es:bx中
    mov si, LoaderFileName ;ds:si -> "LOADER  BIN"
    mov di, OffsetOfLoader  ;es:di -> BaseOfLoader:0100h
    cld
    mov dx, 10h  ;32字节表示一个根目录项,一个扇区中共有16个
LABEL_SEARCH_FOR_LOADERBIN:
    cmp dx, 0
    jz LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR   ;一个扇区16个根目录项比较完毕
    dec dx
    mov cx, 11  ;比较11个字符是否相等
LABEL_CMP_FILENAME:
    cmp cx, 0
    jz LABEL_FILENAME_FOUND ;比较了11个字符都没有跳转，表明找到了
    dec cx
    lodsb   ;ds:si -> al, si++
    cmp al, byte [es:di]
    jz LABEL_GO_ON
    jmp LABEL_DIFFERETN ;因为文件名刚好是从每个32字节目录项的第一个字节开始，
LABEL_GO_ON:            ;只要发现不一样的字符,可以只比较开头11个文件名字符即可
    inc di
    jmp LABEL_CMP_FILENAME
LABEL_DIFFERETN:
    and di, 0FFE0h  ;di &= E0, 清空后五位，指向根目录项开始
    add di, 20h     ;加上32字节，指向下一个目录项
    mov si, LoaderFileName  ;
    jmp LABEL_SEARCH_FOR_LOADERBIN
LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR:
    add word [wSectorNo], 1
    jmp LABEL_SEARCH_IN_ROOT_DIR_BEGIN
LABEL_NO_LOADERBIN:
    mov dh, 2           ;"No Loader"
    call DispStr        ;显示字符串
%ifdef _BOOT_DEBUG_
    mov ax, 4c00h
    int 21h
%else
    jmp $       ;没有找到一直在此处循环
%endif
LABEL_FILENAME_FOUND:  ;此时已经找到了loader.bin所对应的根目录项
    mov ax, RootDirSectors
    and di, 0FFE0h  ;使es:di指向根目录项开始
    add di, 01Ah    ;使es:di指向该文件起始簇号的字节
    mov cx, word [es:di]    ;取起始簇号
    push cx
    add cx, ax
    add cx, DeltaSectorNo   ;得到了起始簇号对应的扇区号
    mov ax, BaseOfLoader
    mov es, ax
    mov bx, OffsetOfLoader      ;es:bx指向存储loader.bin的起始位置
    mov ax, cx              ;ax扇区号
LABEL_LOADERING_FILE:
    push ax
    push bx
    mov ah, 0Eh
    mov al, '.'
    mov bl, 0Fh
    int 10h         ;每将读取一个loader.bin的扇区，就打印一个.
    pop bx          ;形成Booting  .......的效果
    pop ax          ;电传打字机输出	AH=0EH	AL=字符，BH=页码，BL=颜色（只适用于图形模式）
    mov cl, 1       ;读取一个扇区
    call ReadSector
    pop ax          ;取出该扇区对应的簇号
    call GetFATEntry    ;返回的ax中存着下一个扇区对应的簇号
    cmp ax, 0FFFh       ;假如下一个簇号=0FFFh，表明文件结束
    jz LABEL_FILE_LOADED
    push ax     ;下一个簇号入栈
    mov dx, RootDirSectors
    add ax, dx
    add ax, DeltaSectorNo   ;得到下一个扇区号
    add bx, [BPB_BytesPerSec]   ;存储位置偏移512字节
    jmp LABEL_LOADERING_FILE
LABEL_FILE_LOADED:
    mov dh, 1
    call DispStr        ;"Ready.   " 表示引导已经完毕，loader.bin已经加载进入了内存

    jmp BaseOfLoader:OffsetOfLoader     ;正式跳转到loader.bin代码开始执行，转移控制权


;功能：显示一个字符串
;默认：显示的字符串默认9字节，字符串序号存在dh中
DispStr:
    mov ax, MessageLength
    mul dh
    add ax, BootMessage
    mov bp, ax
    mov ax, ds
    mov es, ax  ;es:bp起始段地址
    mov cx, MessageLength  ;串长度
    mov ax, 01301h  ;AH=13h,AL=01h
    mov bx, 0007h   ;页号为0(BH=0)，黑底白字高亮(BL=07h)
    mov dl, 0
    int 10h  ;AH=13H，AL= 显示输出方式，BH=页码，BL=颜色，CX=字符串长度，DH=行，DL=列，ES:BP=字符串偏移量
    ret

;功能：读取一定软盘扇区数到指定内存处
;默认：从第ax个Sector扇区开始(索引从0开始)，将cl个Sector读取es:bx中
ReadSector:
    push bp
    mov bp, sp
    sub esp, 2  ;用两个字节栈空间存储要读的扇区数
    mov byte [bp - 2], cl   ;读取扇区数入栈
    push bx
    mov bl, [BPB_SecPerTrk]     ;除数：每个磁道的扇区数
    div bl   ;扇区号/每个磁道的扇区数，商在al,余数在ah中
    inc ah
    mov cl, ah      ;cl中存读取的起始扇区号
    mov dh, al
    shr al, 1
    mov ch, al          ;ch中存柱面号
    and dh, 1           ;dh中存磁头号
    pop bx              ;bx中存着目标偏移地址
    mov dl, [BS_DrvNum]    ;dl中存驱动器号，0表示软盘A
.GoOnReading:
    mov ah, 2       ;int 13h子程序号
    mov al, byte [bp - 2]   ;al中存读取的扇区数
    int 13h
    jc .GoOnReading     ;若读取错误会将CF位置1，不停地读，直至成功
    add esp, 2
    pop bp
    ret

;功能：寻找指定簇号(FAT表索引)在FAT中的表项值
;默认：ax存储着簇号，计算得到的表项值存储在ax中
GetFATEntry:
    push es
    push bx
    push ax     ;此时ax中存储着待查找的簇号
    mov ax, BaseOfLoader
    sub ax, 0100h       ;注意这是段地址减去了0100h
    mov es, ax      ;在BaseOfLoader的4k字节存放FAT扇区
    pop ax
    mov byte [bOdd], 0
    mov bx, 3
    mul bx
    mov bx, 2
    div bx      ;ax乘以3除以2后得到簇号对应的偏移字节(相对于FAT表)
    cmp dx, 0   ;通过余数判断簇号对应的12bit是占偏移2字节的高位还是低位
    jz LABEL_EVEN
    mov byte [bOdd], 1
LABEL_EVEN:
    ;现在ax中存着簇号对应FATEntry的偏移量，接下来计算
    xor dx, dx
    mov bx, [BPB_BytesPerSec]
    div bx      ;通过除以512字节，ax中存着相对于FAT表的偏移扇区数，dx存着在扇区中的偏移量
    push dx
    mov bx, 0   ;读取FATEntry对应的两字节进入es:bx
    add ax, SectorNoOfFAT1  ;FAT表的第一扇区号
    mov cl, 2   ;读取两个两个扇区，因为FATEntry有可能跨越两个扇区
    call ReadSector ;读取到了es:bx中
    pop dx
    add bx, dx  ;得到在扇区中的偏移量
    mov ax, [es:bx]
    cmp byte [bOdd], 1      ;假如对应是偶数，占低12bit，反之占高12bit
    jnz LABEL_EVEN2
    shr ax, 4
LABEL_EVEN2:
    and ax, 0FFFh
    pop bx
    pop es
    ret ;最终ax中存着下一个簇号

times 510 - ($ - $$) db 0
dw 0xAA55   ;结束标志

