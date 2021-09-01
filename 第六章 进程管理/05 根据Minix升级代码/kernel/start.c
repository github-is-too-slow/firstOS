#include "type.h"
#include "const.h"
#include "string.h"
#include "proto.h"
#include "protect.h"
#include "process.h"
#include "global.h"

/**
 * 功能：完成从loader到kernel的堆栈和GDT的切换
 * 默认：gdt_ptr中已经存储着loader.bin中GDT的指针
 **/
PUBLIC void cstart(){
    disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n-----\"CStart\" begins-----\n");
    //将loader中gdt复制到kernel中
    memcpy(&gdt,         //&gdt是指向DESCRIPTOR[GDT_SIZE]数组类型的指针
           (void*)(*((u32*)(&gdt_ptr[2]))),    //base of old gdt,四字节
           *((u16*)(&gdt_ptr[0])) + 1);        //limit of old gdt, 两字节
    //更新gdt_ptr的值，指向新的GDT表
    u16 *p_gdt_limit = (u16*)(&gdt_ptr[0]);
    u32 *p_gdt_base = (u32*)(&gdt_ptr[2]);
    *p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
    *p_gdt_base = (u32)&gdt;
    //初始化中断描述符表IDT、设置LDT、TSS在GDT中的描述符
    init_protect();
    //更新idt_ptr的值，指向IDT表
    u16 *p_idt_limit = (u16*)(&idt_ptr[0]);
    u32 *p_idt_base = (u32*)(&idt_ptr[2]);
    *p_idt_limit = IDT_SIZE * sizeof(GATE) - 1;
    *p_idt_base = (u32)&idt;
    disp_str("-----\"CStart\" ends-----\n");
}