# Details

Date : 2021-08-20 17:35:49

Directory c:\Users\带ta去蒙古国\Desktop\自己动手写操作系统\第十章 内存管理\01 fork一个子进程

Total : 55 files,  5314 codes, 1609 comments, 627 blanks, all 7550 lines

[summary](results.md)

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [第十章 内存管理/01 fork一个子进程/Makefile](/第十章 内存管理/01 fork一个子进程/Makefile) | Makefile | 175 | 11 | 49 | 235 |
| [第十章 内存管理/01 fork一个子进程/boot/boot.asm](/第十章 内存管理/01 fork一个子进程/boot/boot.asm) | nasm | 197 | 20 | 15 | 232 |
| [第十章 内存管理/01 fork一个子进程/boot/include/fat12hdr.inc](/第十章 内存管理/01 fork一个子进程/boot/include/fat12hdr.inc) | nasm | 24 | 9 | 1 | 34 |
| [第十章 内存管理/01 fork一个子进程/boot/include/lib.inc](/第十章 内存管理/01 fork一个子进程/boot/include/lib.inc) | nasm | 116 | 20 | 12 | 148 |
| [第十章 内存管理/01 fork一个子进程/boot/include/loader.inc](/第十章 内存管理/01 fork一个子进程/boot/include/loader.inc) | nasm | 10 | 4 | 1 | 15 |
| [第十章 内存管理/01 fork一个子进程/boot/include/pm.inc](/第十章 内存管理/01 fork一个子进程/boot/include/pm.inc) | nasm | 38 | 18 | 7 | 63 |
| [第十章 内存管理/01 fork一个子进程/boot/loader.asm](/第十章 内存管理/01 fork一个子进程/boot/loader.asm) | nasm | 404 | 51 | 24 | 479 |
| [第十章 内存管理/01 fork一个子进程/fs/link.c](/第十章 内存管理/01 fork一个子进程/fs/link.c) | C | 130 | 66 | 20 | 216 |
| [第十章 内存管理/01 fork一个子进程/fs/main.c](/第十章 内存管理/01 fork一个子进程/fs/main.c) | C | 259 | 154 | 48 | 461 |
| [第十章 内存管理/01 fork一个子进程/fs/misc.c](/第十章 内存管理/01 fork一个子进程/fs/misc.c) | C | 63 | 52 | 12 | 127 |
| [第十章 内存管理/01 fork一个子进程/fs/open.c](/第十章 内存管理/01 fork一个子进程/fs/open.c) | C | 216 | 121 | 53 | 390 |
| [第十章 内存管理/01 fork一个子进程/fs/read_write.c](/第十章 内存管理/01 fork一个子进程/fs/read_write.c) | C | 93 | 22 | 15 | 130 |
| [第十章 内存管理/01 fork一个子进程/include/config.h](/第十章 内存管理/01 fork一个子进程/include/config.h) | C++ | 4 | 0 | 0 | 4 |
| [第十章 内存管理/01 fork一个子进程/include/console.h](/第十章 内存管理/01 fork一个子进程/include/console.h) | C++ | 17 | 0 | 3 | 20 |
| [第十章 内存管理/01 fork一个子进程/include/const.h](/第十章 内存管理/01 fork一个子进程/include/const.h) | C++ | 139 | 45 | 23 | 207 |
| [第十章 内存管理/01 fork一个子进程/include/elf.h](/第十章 内存管理/01 fork一个子进程/include/elf.h) | C++ | 48 | 1 | 5 | 54 |
| [第十章 内存管理/01 fork一个子进程/include/fs.h](/第十章 内存管理/01 fork一个子进程/include/fs.h) | C++ | 50 | 65 | 12 | 127 |
| [第十章 内存管理/01 fork一个子进程/include/global.h](/第十章 内存管理/01 fork一个子进程/include/global.h) | C++ | 33 | 1 | 1 | 35 |
| [第十章 内存管理/01 fork一个子进程/include/hd.h](/第十章 内存管理/01 fork一个子进程/include/hd.h) | C++ | 58 | 54 | 18 | 130 |
| [第十章 内存管理/01 fork一个子进程/include/keyboard.h](/第十章 内存管理/01 fork一个子进程/include/keyboard.h) | C++ | 95 | 17 | 13 | 125 |
| [第十章 内存管理/01 fork一个子进程/include/keymap.h](/第十章 内存管理/01 fork一个子进程/include/keymap.h) | C++ | 133 | 7 | 6 | 146 |
| [第十章 内存管理/01 fork一个子进程/include/process.h](/第十章 内存管理/01 fork一个子进程/include/process.h) | C++ | 100 | 10 | 4 | 114 |
| [第十章 内存管理/01 fork一个子进程/include/protect.h](/第十章 内存管理/01 fork一个子进程/include/protect.h) | C++ | 128 | 18 | 17 | 163 |
| [第十章 内存管理/01 fork一个子进程/include/proto.h](/第十章 内存管理/01 fork一个子进程/include/proto.h) | C++ | 72 | 2 | 2 | 76 |
| [第十章 内存管理/01 fork一个子进程/include/sconst.inc](/第十章 内存管理/01 fork一个子进程/include/sconst.inc) | nasm | 32 | 5 | 2 | 39 |
| [第十章 内存管理/01 fork一个子进程/include/stdio.h](/第十章 内存管理/01 fork一个子进程/include/stdio.h) | C++ | 12 | 1 | 1 | 14 |
| [第十章 内存管理/01 fork一个子进程/include/string.h](/第十章 内存管理/01 fork一个子进程/include/string.h) | C++ | 7 | 0 | 0 | 7 |
| [第十章 内存管理/01 fork一个子进程/include/tty.h](/第十章 内存管理/01 fork一个子进程/include/tty.h) | C++ | 18 | 2 | 3 | 23 |
| [第十章 内存管理/01 fork一个子进程/include/type.h](/第十章 内存管理/01 fork一个子进程/include/type.h) | C++ | 12 | 0 | 0 | 12 |
| [第十章 内存管理/01 fork一个子进程/kernel/clock.c](/第十章 内存管理/01 fork一个子进程/kernel/clock.c) | C | 37 | 16 | 5 | 58 |
| [第十章 内存管理/01 fork一个子进程/kernel/console.c](/第十章 内存管理/01 fork一个子进程/kernel/console.c) | C | 105 | 29 | 9 | 143 |
| [第十章 内存管理/01 fork一个子进程/kernel/global.c](/第十章 内存管理/01 fork一个子进程/kernel/global.c) | C | 39 | 24 | 8 | 71 |
| [第十章 内存管理/01 fork一个子进程/kernel/hd.c](/第十章 内存管理/01 fork一个子进程/kernel/hd.c) | C | 270 | 112 | 19 | 401 |
| [第十章 内存管理/01 fork一个子进程/kernel/i8259A.c](/第十章 内存管理/01 fork一个子进程/kernel/i8259A.c) | C | 37 | 22 | 8 | 67 |
| [第十章 内存管理/01 fork一个子进程/kernel/kernel.asm](/第十章 内存管理/01 fork一个子进程/kernel/kernel.asm) | nasm | 251 | 78 | 20 | 349 |
| [第十章 内存管理/01 fork一个子进程/kernel/keyboard.c](/第十章 内存管理/01 fork一个子进程/kernel/keyboard.c) | C | 269 | 62 | 17 | 348 |
| [第十章 内存管理/01 fork一个子进程/kernel/main.c](/第十章 内存管理/01 fork一个子进程/kernel/main.c) | C | 174 | 40 | 40 | 254 |
| [第十章 内存管理/01 fork一个子进程/kernel/printf.c](/第十章 内存管理/01 fork一个子进程/kernel/printf.c) | C | 32 | 15 | 5 | 52 |
| [第十章 内存管理/01 fork一个子进程/kernel/process.c](/第十章 内存管理/01 fork一个子进程/kernel/process.c) | C | 303 | 52 | 13 | 368 |
| [第十章 内存管理/01 fork一个子进程/kernel/protect.c](/第十章 内存管理/01 fork一个子进程/kernel/protect.c) | C | 165 | 28 | 9 | 202 |
| [第十章 内存管理/01 fork一个子进程/kernel/start.c](/第十章 内存管理/01 fork一个子进程/kernel/start.c) | C | 29 | 8 | 1 | 38 |
| [第十章 内存管理/01 fork一个子进程/kernel/syscall.asm](/第十章 内存管理/01 fork一个子进程/kernel/syscall.asm) | nasm | 20 | 4 | 5 | 29 |
| [第十章 内存管理/01 fork一个子进程/kernel/systask.c](/第十章 内存管理/01 fork一个子进程/kernel/systask.c) | C | 30 | 3 | 1 | 34 |
| [第十章 内存管理/01 fork一个子进程/kernel/tty.c](/第十章 内存管理/01 fork一个子进程/kernel/tty.c) | C | 231 | 120 | 26 | 377 |
| [第十章 内存管理/01 fork一个子进程/kernel/vsprintf.c](/第十章 内存管理/01 fork一个子进程/kernel/vsprintf.c) | C | 102 | 14 | 15 | 131 |
| [第十章 内存管理/01 fork一个子进程/lib/close.c](/第十章 内存管理/01 fork一个子进程/lib/close.c) | C | 21 | 11 | 3 | 35 |
| [第十章 内存管理/01 fork一个子进程/lib/fork.c](/第十章 内存管理/01 fork一个子进程/lib/fork.c) | C | 22 | 15 | 3 | 40 |
| [第十章 内存管理/01 fork一个子进程/lib/klib.c](/第十章 内存管理/01 fork一个子进程/lib/klib.c) | C | 114 | 48 | 16 | 178 |
| [第十章 内存管理/01 fork一个子进程/lib/kliba.asm](/第十章 内存管理/01 fork一个子进程/lib/kliba.asm) | nasm | 178 | 42 | 12 | 232 |
| [第十章 内存管理/01 fork一个子进程/lib/misc.c](/第十章 内存管理/01 fork一个子进程/lib/misc.c) | C | 27 | 23 | 5 | 55 |
| [第十章 内存管理/01 fork一个子进程/lib/open.c](/第十章 内存管理/01 fork一个子进程/lib/open.c) | C | 24 | 14 | 5 | 43 |
| [第十章 内存管理/01 fork一个子进程/lib/read.c](/第十章 内存管理/01 fork一个子进程/lib/read.c) | C | 23 | 14 | 3 | 40 |
| [第十章 内存管理/01 fork一个子进程/lib/string.asm](/第十章 内存管理/01 fork一个子进程/lib/string.asm) | nasm | 83 | 14 | 5 | 102 |
| [第十章 内存管理/01 fork一个子进程/lib/unlink.c](/第十章 内存管理/01 fork一个子进程/lib/unlink.c) | C | 22 | 11 | 4 | 37 |
| [第十章 内存管理/01 fork一个子进程/lib/write.c](/第十章 内存管理/01 fork一个子进程/lib/write.c) | C | 23 | 14 | 3 | 40 |

[summary](results.md)