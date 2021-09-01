/*一些用汇编写的函数原型*/
PUBLIC void out_byte(u16 port, u8 value);
PUBLIC u8 in_byte(u16 port);
PUBLIC void disp_str(char *info);
PUBLIC void disp_color_str(char *info, int text_color);
PUBLIC void restart();
PUBLIC void disable_irq(int irq);
PUBLIC void enable_irq(int irq);
PUBLIC int get_ticks();

/*一些C函数*/
PUBLIC void disp_int(int input);
PUBLIC void delay(int time);
PUBLIC void TestA();
PUBLIC void TestB();
PUBLIC void TestC();
PUBLIC void clock_handler(int irq);
PUBLIC void init_protect();
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC int  sys_get_ticks();
PUBLIC void milli_delay(int milli_sec);
PUBLIC void schedule();
PUBLIC void clearDisplay();
