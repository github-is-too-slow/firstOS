#ifndef _ORANGES_PROTO_H_
#define _ORANGES_PROTO_H_
/*一些用汇编写的函数原型*/
PUBLIC void out_byte(u16 port, u8 value);
PUBLIC u8 in_byte(u16 port);
PUBLIC void disp_str(char *info);
PUBLIC void disp_color_str(char *info, int text_color);
PUBLIC void restart();
PUBLIC void disable_irq();
PUBLIC void enable_irq();
PUBLIC int get_ticks();
PUBLIC void disable_int();
PUBLIC void enable_int();
PUBLIC void port_read(u16 port, void *buf, int n);
PUBLIC void port_write(u16 port, void *buf, int n);

/*一些C函数*/
PUBLIC char *itoa(char *str, int num);
PUBLIC void disp_int(int input);
PUBLIC void delay(int time);
PUBLIC void TestA();
PUBLIC void TestB();
PUBLIC void TestC();
PUBLIC void task_tty();
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void init_clock();
PUBLIC void init_keyboard();
PUBLIC void init_protect();
PUBLIC int  sys_get_ticks();
PUBLIC void milli_delay(int milli_sec);
PUBLIC void schedule();
PUBLIC void clearDisplay();
PUBLIC void keyboard_read(TTY *p_tty);
PUBLIC void in_process(TTY *p_tty, u32 key);
PUBLIC int is_current_console(CONSOLE *p_con);
PUBLIC void out_char(CONSOLE *p_con, char ch);
PUBLIC void init_screen(TTY *p_tty);
PUBLIC void select_console(int nr_console);
PUBLIC void scroll_screen(CONSOLE *p_con, int direction);
PUBLIC int vsprintf(char *buf, const char *fmt, va_list args);
PUBLIC int sprintf(char *buf, const char *fmt, ...);
PUBLIC int sys_write(char *buf, int len, PROCESS *p_proc);
PUBLIC void write(char *buf, int len);
PUBLIC int printf(const char *fmt, ...);
PUBLIC void printx(char* s);
PUBLIC int sys_printx(int _unused1, int _unused2, char* s, PROCESS *p_proc);
PUBLIC void panic(const char *fmt, ...);
PUBLIC int sendrec(int function, int src_dest, MESSAGE *p_msg);
PUBLIC int sys_sendrec(int function, int src_dest, MESSAGE *m, PROCESS *p);
PUBLIC void task_sys();
PUBLIC int send_recv(int function, int src_dest, MESSAGE *msg);
PUBLIC void reset_msg(MESSAGE *p);
PUBLIC void *va2la(int pid, void *va);
PUBLIC int ldt_seg_linear(PROCESS *p, int idx);
PUBLIC void spin(char * func_name);
PUBLIC void task_hd();
PUBLIC void inform_int(int task_nr);
PUBLIC void task_fs();
PUBLIC int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf);
PUBLIC int min(int a, int b);
PUBLIC int search_file(char * path);
PUBLIC int strip_path(char * filename, const char * pathname, struct inode** ppinode);
PUBLIC int memcmp(const char *str1, const char *str2, int strlen);
PUBLIC struct super_block * get_super_block(int dev);
PUBLIC struct inode * get_inode(int dev, int num);
PUBLIC void put_inode(struct inode * pinode);
PUBLIC void sync_inode(struct inode * p);
PUBLIC int do_open();
PUBLIC int do_close();
#define proc2pid(x) (x - proc_table)
#endif
