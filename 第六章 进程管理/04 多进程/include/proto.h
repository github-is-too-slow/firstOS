/*一些用汇编写的函数原型*/
PUBLIC void out_byte(u16 port, u8 value);
PUBLIC u8 in_byte(u16 port);
PUBLIC void disp_str(char *info);
PUBLIC void disp_color_str(char *info, int text_color);

/*一些C函数*/
PUBLIC void disp_int(int input);
PUBLIC void delay(int time);
PUBLIC void TestA();
PUBLIC void TestB();
PUBLIC void TestC();
