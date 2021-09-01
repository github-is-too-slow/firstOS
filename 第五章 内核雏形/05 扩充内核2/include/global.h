#ifdef GLOBAL_VARIABLES_HERE_
#undef EXTERN   //取消EXTERN的定义
#define EXTERN
#endif

//全局变量定义在此处
EXTERN  int         disp_pos;
EXTERN  u8          gdt_ptr[6];
EXTERN  DESCRIPTOR  gdt[GDT_SIZE];
EXTERN  u8          idt_ptr[6];
EXTERN  GATE        idt[IDT_SIZE];