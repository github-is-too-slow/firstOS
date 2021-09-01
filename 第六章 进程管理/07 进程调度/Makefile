###############################
# Makefile for Orange's
#################################

#entry point 内核入口地址(线性或物理地址)
#与load.inc文件中的KernelEntryPointPhyAddr保持一致
ENTRYPOINT	= 0x30400

#offset of entry point in kernel file
ENTRYOFFSET	= 0x400

#programs, flags, etc
ASM = nasm
DASM = ndisasm
CC = gcc
LD = ld
ASMBFLAGS = -I boot/include/
ASMKFLAGS = -I include/ -f elf
CFLAGS = -I include/ -c -fno-builtin -fno-stack-protector -m32
LDFLAGS = -s -Ttext $(ENTRYPOINT) -m elf_i386
DASMFLAGS = -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

#this program,生成的中间文件和目标文件
ORANGEBOOT = boot/boot.bin boot/loader.bin
ORANGEKERNEL = kernel.bin
OBJS = kernel/kernel.o  kernel/clock.o kernel/global.o kernel/i8259A.o kernel/main.o \
	   kernel/process.o kernel/protect.o kernel/start.o kernel/syscall.o \
	   lib/klib.o lib/kliba.o lib/string.o

DASMOUTPUT = kernel.bin.asm

#all phony targets 假目标文件，当做动作名
.phony: everything final image clean realclean disasm all buildimg

#default starting position，当输入make默认执行该文件中第一个动作名
everything: $(ORANGEBOOT) $(ORANGEKERNEL)

all: realclean everything

final: all clean

image: final buildimg

clean:
	rm -f $(OBJS)

realclean:
	rm -f $(OBJS) $(ORANGEBOOT) $(ORANGEKERNEL)

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

$(ORANGEKERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

kernel/kernel.o: kernel/kernel.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/clock.o: kernel/clock.c include/type.h include/const.h \
		include/proto.h include/protect.h include/global.h include/process.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/global.o: kernel/global.c include/type.h include/const.h include/process.h \
					include/proto.h include/protect.h include/global.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/i8259A.o: kernel/i8259A.c include/type.h include/const.h \
		include/proto.h include/protect.h include/global.h include/process.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/main.o: kernel/main.c include/type.h include/const.h include/string.h \
		include/proto.h include/protect.h include/global.h include/process.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/process.o: kernel/process.c include/type.h include/const.h \
		include/proto.h include/protect.h include/global.h include/process.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/protect.o: kernel/protect.c include/type.h include/const.h include/string.h \
		include/proto.h include/protect.h include/global.h include/process.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/start.o: kernel/start.c include/type.h include/const.h include/string.h \
		include/proto.h include/protect.h include/global.h include/process.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/syscall.o: kernel/syscall.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/klib.o: lib/klib.c include/type.h include/const.h include/process.h \
		include/proto.h include/protect.h include/global.h
	$(CC) $(CFLAGS) -o $@ $<

lib/kliba.o: lib/kliba.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/string.o: lib/string.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<




