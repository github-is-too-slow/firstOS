#ifndef _ORANGES_CONST_H_
#define _ORANGES_CONST_H_

/*布尔值*/
#define TRUE 1
#define FALSE 0

/*函数类型*/
#define PUBLIC
#define PRIVATE static  //限制全局变量或函数的作用域

/*引用变量*/
#define EXTERN extern

/*前景色*/
#define BLACK   0x0     //0000
#define WHITE   0x7     //0111
#define RED     0x4     //0100
#define GREEN   0x2     //0010
#define BLUE    0x1     //0001
#define FLASH   0x80    //1000 0000
#define BRIGHT  0x08    //0000 1000
#define MAKE_COLOR(x, y)  (x | y)

/* VGA寄存器 */
#define	CRTC_ADDR_REG	0x3D4	/* CRT Controller Registers - Addr Register */
#define	CRTC_DATA_REG	0x3D5	/* CRT Controller Registers - Data Register */
#define	START_ADDR_H	0xC	/* reg index of video mem start addr (MSB) 起始位置*/
#define	START_ADDR_L	0xD	/* reg index of video mem start addr (LSB) */
#define	CURSOR_H	0xE	/* reg index of cursor position (MSB) 游标*/
#define	CURSOR_L	0xF	/* reg index of cursor position (LSB) */
#define	V_MEM_BASE	0xB8000	/* base of color video memory */
#define	V_MEM_SIZE	0x8000	/* 32K: B8000H -> BFFFFH */

#define NR_CONSOLES 3   /*TTY任务和console控制台个数*/

#endif