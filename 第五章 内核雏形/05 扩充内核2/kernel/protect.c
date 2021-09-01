#include "const.h"
#include "type.h"
#include "proto.h"
#include "protect.h"
#include "global.h"

//导入异常处理函数
void init_8259A();
void divide_error();
void single_step_exception();
void nmi();
void breakpoint_exception();
void overflow();
void bounds_check();
void inval_opcode();
void copr_not_available();
void double_fault();
void copr_seg_overrun();
void inval_tss();
void segment_not_present();
void stack_exception();
void general_protection();
void page_fault();
void copr_error();
//导入中断处理函数
void hwint00();
void hwint01();
void hwint02();
void hwint03();
void hwint04();
void hwint05();
void hwint06();
void hwint07();
void hwint08();
void hwint09();
void hwint10();
void hwint11();
void hwint12();
void hwint13();
void hwint14();
void hwint15();


/**
 * 初始化一个具体的IDT中断门描述符
 **/
PRIVATE void init_idt_desc(unsigned char vector, u8 desc_type,
                        int_handler handler, unsigned char privilege){
    GATE *p_gate = &idt[vector];
    u32 base = (u32)handler;
    p_gate->offset_low = base &0xFFFF;
    p_gate->selector = SELECTOR_KERNEL_CS;
    p_gate->dcount = 0;
    p_gate->attr = desc_type | (privilege << 5);
    p_gate->offset_high = (base >> 16) & 0xFFFF;
}

/**
 * 初始化IDT
 **/
PUBLIC void init_protect(){
    init_8259A();
    //全部初始化为中断门
    init_idt_desc(INT_VECTOR_DIVIDE, DA_386IGate, divide_error, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_DEBUG, DA_386IGate, single_step_exception, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_NMI, DA_386IGate, nmi, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_BREAKPOINT, DA_386IGate, breakpoint_exception, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_OVERFLOW, DA_386IGate, overflow, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_BOUNDS, DA_386IGate, bounds_check, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_INVAL_OP, DA_386IGate, inval_opcode, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_COPROC_NOT, DA_386IGate, copr_not_available, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_DOUBLE_FAULT, DA_386IGate, double_fault, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_COPROC_SEG, DA_386IGate, copr_seg_overrun, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_INVAL_TSS, DA_386IGate, inval_tss, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_SEG_NOT, DA_386IGate, segment_not_present, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_STACK_FAULT, DA_386IGate, stack_exception, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_PROTECTION, DA_386IGate, general_protection, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_PAGE_FAULT, DA_386IGate, page_fault, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_COPROC_ERR, DA_386IGate, copr_error, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 0, DA_386IGate, hwint00, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ0 + 1, DA_386IGate, hwint01, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ0 + 2, DA_386IGate, hwint02, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ0 + 3, DA_386IGate, hwint03, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ0 + 4, DA_386IGate, hwint04, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ0 + 5, DA_386IGate, hwint05, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ0 + 6, DA_386IGate, hwint06, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ0 + 7, DA_386IGate, hwint07, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ8 + 0, DA_386IGate, hwint08, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ8 + 1, DA_386IGate, hwint09, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ8 + 2, DA_386IGate, hwint10, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ8 + 3, DA_386IGate, hwint11, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ8 + 4, DA_386IGate, hwint12, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ8 + 5, DA_386IGate, hwint13, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ8 + 6, DA_386IGate, hwint14, PRIVILEGE_KRNL);
    init_idt_desc(INT_VECTOR_IRQ8 + 7, DA_386IGate, hwint15, PRIVILEGE_KRNL);
}

/**
 * 具体的异常处理函数
 **/
PUBLIC void exception_handle(int vec_no, int err_code, int eip, int cs, int eflags){
    int i;
    int text_color = 0x74;      //灰底红字
    char *err_msg[] = {
        "#DE DIVIDE ERROR",
        "#DB RESERVED",
        "--  NMI INTERRUPT",
        "#BP BREAKPOINT",
        "#OF OVERFLOW",
        "#BR BOUND RANGE EXCEEDED",
        "#UD INVALID OPCODE (UNDEFINED OPCODE)",
        "#NM DEVICE NOT AVAILABLE (NO MATH COPROCESSOR)",
        "#DF DOUBLE FAULT",
        "    COPROCESSOR SEGMENT OVERRUN (RESERVED)",
        "#TS INVALID TSS",
        "#NP SEGMENT NOT PRESENT",
        "#SS STACK SEGMENT FAULT",
        "#GP GENERAL PROTECTION",
        "#PF PAGE FAULT",
        "--  (INTEL RESERVED. DO NOT USE.)",
        "#MF X87 FPU FLOATING POINT ERROR (MATH FAULT)",
        "#AC ALIGNMENT CHECK",
        "#MC MACHINE CHECK",
        "XF SIMD FLOATING POINT EXCEPTION"
    };
    disp_pos = 0;
    for(i = 0; i < 80 * 5; i++){//前5行清屏
        disp_str(" ");
    }
    disp_pos = 0;

    disp_color_str("Exception! --> ", text_color);
    disp_color_str(err_msg[vec_no], text_color);
    disp_color_str("\n\n", text_color);
    disp_color_str("EFLAGS: ", text_color);
    disp_int(eflags);
    disp_color_str("CS: ", text_color);
    disp_int(cs);
    disp_color_str("EIP: ", text_color);
    disp_int(eip);
    if(err_code != 0xFFFFFFFF){
        disp_color_str("Error code: ", text_color);
        disp_int(err_code);
    }
}