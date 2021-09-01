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
PUBLIC int sys_write(char *buf, int len, PROCESS *p_proc);
PUBLIC void write(char *buf, int len);
PUBLIC int printf(const char *fmt, ...);
#endif
