%include "sconst.inc"

extern disp_pos

global disp_str
global disp_color_str
global out_byte
global in_byte
global disable_irq
global enable_irq
global disable_int
global enable_int

[section .text]
;功能：显示一个字符串
;默认：void disp_str(char *info),待显示字符串地址在栈顶
disp_str:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    mov esi, [ebp + 8]  ;得到待显示字符串的内存首地址
    mov edi, [disp_pos]
    mov ah, 0Fh         ;字符串颜色属性
.LOOP_STR1:
    lodsb
    test al, al
    jz .LOOP_STR2
    cmp al, 0Ah
    jnz .LOOP_STR3      ;不是回车
    push eax
    mov eax, edi
    mov bl, 160
    div bl
    and eax, 0FFh
    inc eax
    mov bl, 160
    mul bl
    mov edi, eax
    pop eax
    jmp .LOOP_STR1
.LOOP_STR3:     ;不是回车显示一个字符
    mov [gs:edi], ax
    add edi, 2
    jmp .LOOP_STR1
.LOOP_STR2:
    mov [disp_pos], edi
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
;disp_color_str

;功能：显示一个字符串
;默认：void disp_color_str(char *info, int text_color)
disp_color_str:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    mov esi, [ebp + 8]  ;得到待显示字符串的内存首地址
    mov edi, [disp_pos]
    mov ah, [ebp + 12]  ;字符串颜色属性
.LOOP_STR1:
    lodsb
    test al, al
    jz .LOOP_STR2
    cmp al, 0Ah
    jnz .LOOP_STR3      ;不是回车
    push eax
    mov eax, edi
    mov bl, 160
    div bl
    and eax, 0FFh
    inc eax
    mov bl, 160
    mul bl
    mov edi, eax
    pop eax
    jmp .LOOP_STR1
.LOOP_STR3:     ;不是回车显示一个字符
    mov [gs:edi], ax
    add edi, 2
    jmp .LOOP_STR1
.LOOP_STR2:
    mov [disp_pos], edi
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
;disp_str

;功能：往指定端口输出(写入)数据
;声明：void out_byte(u16 port, u8 value);
out_byte:
    push ebp
    mov ebp, esp
    mov edx, [ebp + 8]  ;port
    mov al, [ebp + 12]  ;value,两个参数值在栈中均占有4字节
    out dx, al          ;向指定端口输出
    pop ebp
    nop
    nop
    ret

;功能：从指定端口读取数据
;声明：u8 in_byte(u16 port);
in_byte:
    push ebp
    mov ebp, esp
    mov edx, [ebp + 8]
    in al, dx
    pop ebp
    nop
    nop
    ret

;功能：关闭某个i9259A中断源
;原型：void disable_irq(int irq);
;等价C语言逻辑：
;   if(irq < 8){
;       out_byte(INT_M_CTLMASK, int_byte(INT_M_CTLMASK) | (1 << irq));
;   }else{
;       out_byte(INT_S_CTLMASK, int_byte(INT_S_CTLMASK) | (1 << irq));
;   }
disable_irq:
    mov ecx, [esp + 4]      ;irq中断号
    pushf                   ;保存标志寄存器
    cli                     ;关中断
    ;目的操作数循环左移，源操作数决定移位的数目
    ;最高位复制到进位标志并同时送至最低位中.
    mov ah, 1
    rol ah, cl              ;cl中的中断号表示移位的位数,如0000,1000
    cmp cl, 8
    jae disable_8           ;irq >= 8
disable_0:
    in al, INT_M_CTLMASK
    test al, ah     ;al与ah进行与操作，是否为0只取决于irq对应的bit位
    jnz dis_already     ;是否已经关闭中断
    or al, ah           ;关闭是用或操作，or
    out INT_M_CTLMASK, al
    popf
    mov eax, 1      ;刚关闭返回1
    ret
disable_8:
    in al, INT_S_CTLMASK
    test al, ah     ;al与ah进行与操作，是否为0只取决于irq对应的bit位
    jnz dis_already     ;是否已经关闭中断
    or al, ah
    out INT_S_CTLMASK, al
    popf
    mov eax, 1      ;刚关闭返回1
    ret
dis_already:
    popf
    xor eax, eax            ;已经关闭了返回0
    ret

;功能：打开某个i9259A中断源
;原型：void enable_irq(int irq);
;等价C语言逻辑：
;   if(irq < 8){
;       out_byte(INT_M_CTLMASK, int_byte(INT_M_CTLMASK) & ~(1 << irq));
;   }else{
;       out_byte(INT_S_CTLMASK, int_byte(INT_S_CTLMASK) & ~(1 << irq));
;   }
enable_irq:
    mov ecx, [esp + 4]      ;irq中断号
    pushf                   ;保存标志寄存器
    cli                     ;关中断
    ;目的操作数循环左移，源操作数决定移位的数目
    ;最高位复制到进位标志并同时送至最低位中.
    mov ah, ~1              ;-1的内存补码为: 1111,1110
    rol ah, cl              ;cl中的中断号表示移位的位数
    cmp cl, 8
    jae enable_8           ;irq >= 8
enable_0:
    in al, INT_M_CTLMASK
    and al, ah          ;关闭对应bit位
    out INT_M_CTLMASK, al
    popf
    ret
enable_8:
    in al, INT_S_CTLMASK
    and al, ah
    out INT_S_CTLMASK, al
    popf
    ret

;功能：关中断
;原型：void disable_int();
disable_int:
    cli
    ret

;功能：开中断
;原型：void enable_int();
enable_int:
    sti
    ret
