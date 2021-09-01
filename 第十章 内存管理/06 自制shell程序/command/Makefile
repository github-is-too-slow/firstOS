# commands/Makefile

ENTRYPOINT	= 0x1000
HD		= ../80m.img

ASM		= nasm
DASM	= objdump
CC		= gcc
LD		= ld
ASMFLAGS	= -I ../include/ -f elf
CFLAGS		= -I ../include/ -c -fno-builtin -fno-stack-protector -Wall -m32
LDFLAGS		= -s -Ttext $(ENTRYPOINT) -m elf_i386
DASMFLAGS	= -D
LIB		= ../lib/orangescrt.a
#生成的二进制目标文件
BIN		= echo pwd

# All Phony Targets
.PHONY : everything final clean realclean disasm all install

# Default starting position
everything : $(BIN)

install : final
	cp ../kernel.bin ./ -v
	tar vcf inst.tar kernel.bin $(BIN)
	dd if=inst.tar of=$(HD) seek=`echo "obase=10;ibase=16;(\`egrep -e '^ROOT_BASE' ../boot/include/loader.inc | sed -e 's/.*0x//g' | sed -e 's/\r//g'\`+\`egrep -e '#define[[:space:]]*INSTALL_START_SECT' ../include/const.h | sed -e 's/.*0x//g' | sed -e 's/\r//g'\`)*200" | bc` bs=1 count=`ls -l inst.tar | awk -F " " '{print $$5}' | sed -e 's/\r//g'` conv=notrunc

all : realclean everything

final : all clean

clean :
	rm -f *.o

realclean :
	rm -f $(BIN) *.o

kernel.bin :
	cp ../kernel.bin ./

start.o : start.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

echo.o : echo.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

echo : echo.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $^

pwd.o : pwd.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

pwd : pwd.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $^