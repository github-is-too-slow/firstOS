extern disp_pos

[section .text]
;功能：显示一个字符串
;默认：void disp_str(char *info),待显示字符串地址在栈顶
global disp_str
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
global disp_color_str
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
global out_byte
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
global in_byte
in_byte:
    push ebp
    mov ebp, esp
    mov edx, [ebp + 8]
    in al, dx
    pop ebp
    nop
    nop
    ret