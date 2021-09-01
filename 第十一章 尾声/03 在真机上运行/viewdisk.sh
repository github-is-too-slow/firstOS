#!/bin/bash
img_file="80m.img"
#1.查看磁盘信息
xxd -u -a -g 1 -c 16 -s 0xbf9200 -l 512 80m.img
#2.将hdboot.bin导入硬盘主引导记录中，即真正的第一块扇区中，
#此时经过软盘启动一次后，(hd0,4)分区中也已经存在了hdldr、kernel程序，便可以引导操作系统了
#skip=blocks：从输入文件开头跳过blocks个块后再开始复制。
#seek=blocks：从输出文件开头跳过blocks个块后再开始复制。
#dd if=boot/hdboot.bin of=80m.img bs=1 count=446 conv=notrunc
#dd if=boot/hdboot.bin of=80m.img seek=510 skip=510 bs=1 count=2 conv=notrunc

#2.将grub的stage1(相当于grub系统的bootloader)和stage2(相当于grub系统)写入硬盘
dd if=stage1 of=${img_file} bs=1 count=446 conv=notrunc
dd if=stage2 of=${img_file} bs=512 seek=1 conv=notrunc

#3.将hdboot.bin写入到(hd0,4)分区的第一块扇区上，
#并且由grub引导该程序，从而完成操作系统的加载，grub程序的引导程序放在硬盘的第一块扇区中
dd if=boot/hdboot.bin of=${img_file} seek=`echo "obase=10;ibase=16;(\`egrep -e '^ROOT_BASE' boot/include/loader.inc | sed -e 's/.*0x//g' | sed -e 's/\r//g'\`)*200" | bc` bs=1 count=446 conv=notrunc
dd if=boot/hdboot.bin of=${img_file} seek=`echo "obase=10;ibase=16;(\`egrep -e '^ROOT_BASE' boot/include/loader.inc | sed -e 's/.*0x//g' | sed -e 's/\r//g'\`)*200+1FE" | bc` skip=510 bs=1 count=2 conv=notrunc

#4.从硬盘加载，BIOS首先会加载主引导扇区中的grub BootLoader
bochs -f hdbochsrc
