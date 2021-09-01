;功能：16进制显示AL寄存器中的数字
;默认：数字已经在AL寄存器中；edi指向要显示的位置
DispAL:
    push ecx
    push edx
    push edi

    mov edi, [dwDispPos]

    mov ah, 0Fh     ;黑底白字
    mov dl, al
    shr al, 4       ;保留高位
    mov ecx, 2
DISP_BEGIN:
    and al, 01111b
    cmp al, 9   ;判断待显示的4位二进制是介于0-9之间还是10-15之间
    ja CHAR     ;按字母ASCII处理
    add al, '0' ;按数字ASCII处理
    jmp DISPLAY
CHAR:
    sub al, 0Ah  ;先减去10
    add al, 'A'
DISPLAY:
    mov [gs: edi], ax
    add edi, 2
    mov al, dl
    loop DISP_BEGIN

    mov [dwDispPos], edi

    pop edi
    pop edx
    pop ecx
    ret
;DispAL 函数结束

;功能：16进制显示一个整数
;默认：1.待显示字符串(换行符)偏移位置存在栈顶
;      2.显示的位置edi存在dwDispPos内存单元处
DispInt:
    mov eax, [esp + 4]
    shr eax, 24         ;显示高8位
    call DispAL

    mov eax, [esp + 4]
    shr eax, 16         ;显示高16位的低8位
    call DispAL

    mov eax, [esp + 4]
    shr eax, 8         ;显示低16位的高8位
    call DispAL

    mov eax, [esp + 4]     ;显示低8位
    call DispAL

    mov ah, 07h         ;黑底灰字
    mov al, 'h'
    push edi
    mov edi, [dwDispPos]
    mov [gs:edi], ax
    add edi, 4
    mov [dwDispPos], edi
    pop edi
    ret
;DispInt结束

;功能：显示一个字符串
;默认：待显示字符串在栈顶
DispStr:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    mov esi, [ebp + 8]  ;得到待显示字符串的内存首地址
    mov edi, [dwDispPos]
    mov ah, 0Fh
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
    mov [dwDispPos], edi
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
;DispStr结束

;功能：实现换行
;默认：1.待显示字符串(换行符)偏移位置存在栈顶
;      2.显示的位置edi存在dwDispPos内存单元处
DispReturn:
    push szReturn
    call DispStr
    add esp, 4
    ret
;DispReturn 函数结束