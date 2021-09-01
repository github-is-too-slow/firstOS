###############################
# Makefile for Orange's
# 该Makefile文件的主要作用有：
# 1.生成boot、loader、kernel文件
# 2.利用dd命令将boot写入软盘的第一个扇区中
# 3.利用设备挂载将loader、kernel拷贝到软盘的FAT12文件系统中
#################################

#entry point 内核入口地址(线性或物理地址)
#与load.inc文件中的KernelEntryPointPhyAddr保持一致
ENTRYPOINT	= 0x1400

#offset of entry point in kernel file
ENTRYOFFSET	= 0x400

#programs, flags, etc
ASM = nasm
DASM = ndisasm
CC = gcc
LD = ld
AR = ar
ASMBFLAGS = -I boot/include/
ASMKFLAGS = -I include/ -f elf
CFLAGS = -I include/ -c -fno-builtin -fno-stack-protector -Wall -m32
LDFLAGS = -s -Ttext $(ENTRYPOINT) -m elf_i386
DASMFLAGS = -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)
ARFLAGS		= rcs

#this program,生成的中间文件和目标文件
ORANGEBOOT = boot/boot.bin boot/loader.bin boot/hdboot.bin boot/hdldr.bin
ORANGEKERNEL = kernel.bin
LIB	= lib/orangescrt.a

OBJS = kernel/kernel.o  kernel/clock.o kernel/console.o kernel/global.o kernel/hd.o \
	   kernel/i8259A.o kernel/keyboard.o kernel/klib.o kernel/kliba.o kernel/main.o \
	   kernel/process.o kernel/protect.o kernel/start.o  kernel/systask.o kernel/tty.o \
	   fs/link.o fs/main.o fs/misc.o fs/open.o fs/read_write.o \
	   mm/exec.o mm/forkexit.o mm/main.o

LOBJS = lib/close.o lib/exec.o lib/exit.o lib/fork.o lib/getpid.o lib/lseek.o lib/misc.o \
		lib/open.o lib/printf.o lib/read.o lib/stat.o lib/string.o lib/syscall.o \
		lib/unlink.o lib/vsprintf.o lib/wait.o lib/write.o

DASMOUTPUT = kernel.bin.asm

#all phony targets 假目标文件，当做动作名
.phony: everything final image clean realclean disasm all buildimg

#default starting position，当输入make默认执行该文件中第一个动作名
everything: $(ORANGEBOOT) $(ORANGEKERNEL)

all: realclean everything

final: all clean

image: final buildimg

clean:
	rm -f $(OBJS) $(LOBJS)

realclean:
	rm -f $(OBJS) $(LOBJS) $(LIB) $(ORANGEBOOT) $(ORANGEKERNEL)

disasm:
	$(DASM) $(DASMFLAGS) $(ORANGEKERNEL) > $(DASMOUTPUT)

#假设镜像文件a.img已经存在于当前文件夹
buildimg:
	dd if=boot/boot.bin of=a.img bs=512 count=1 conv=notrunc
	sudo mount -o loop a.img /mnt/floppy
	sudo cp -fv boot/loader.bin /mnt/floppy
	sudo cp -fv kernel.bin /mnt/floppy
	sudo umount /mnt/floppy

boot/boot.bin: boot/boot.asm boot/include/loader.inc boot/include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/loader.bin: boot/loader.asm boot/include/loader.inc boot/include/fat12hdr.inc \
					boot/include/pm.inc boot/include/lib.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/hdboot.bin: boot/hdboot.asm boot/include/loader.inc boot/include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/hdldr.bin: boot/hdldr.asm boot/include/loader.inc boot/include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

$(ORANGEKERNEL): $(OBJS) $(LIB)
	$(LD) $(LDFLAGS) -o $@ $^

$(LIB): $(LOBJS)
	$(AR) $(ARFLAGS) $@ $^

kernel/kernel.o: kernel/kernel.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/clock.o: kernel/clock.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/console.o: kernel/console.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/global.o: kernel/global.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/hd.o: kernel/hd.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/i8259A.o: kernel/i8259A.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/keyboard.o: kernel/keyboard.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/klib.o: kernel/klib.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/kliba.o: kernel/kliba.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/main.o: kernel/main.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/process.o: kernel/process.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/protect.o: kernel/protect.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/start.o: kernel/start.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/systask.o: kernel/systask.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/tty.o: kernel/tty.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/close.o: lib/close.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/exec.o: lib/exec.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/exit.o: lib/exit.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/fork.o: lib/fork.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/getpid.o: lib/getpid.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/lseek.o: lib/lseek.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/misc.o: lib/misc.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/open.o: lib/open.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/printf.o: lib/printf.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/read.o: lib/read.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/stat.o: lib/stat.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/string.o: lib/string.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/syscall.o: lib/syscall.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/unlink.o: lib/unlink.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/vsprintf.o: lib/vsprintf.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/wait.o: lib/wait.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

lib/write.o: lib/write.c include/type.h include/const.h include/console.h include/tty.h \
		include/string.h include/protect.h include/process.h include/fs.h include/global.h \
		include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

fs/link.o: fs/link.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

fs/main.o: fs/main.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

fs/misc.o: fs/misc.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

fs/open.o: fs/open.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

fs/read_write.o: fs/read_write.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

mm/exec.o: mm/exec.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

mm/forkexit.o: mm/forkexit.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

mm/main.o: mm/main.c include/type.h include/const.h include/console.h \
		include/tty.h include/string.h include/protect.h include/process.h \
		include/hd.h include/fs.h include/keyboard.h include/keymap.h \
		include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<
