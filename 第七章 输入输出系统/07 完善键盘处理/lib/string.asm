global memcpy
global memset
global strcpy

[section .text]
;功能：内存拷贝
;默认：仿void *memcpy(void *es:pDest, void *ds:pSrc, int iSize)
;      复制长度ISize、源偏移地址(相对于32位代码段)、目标偏移地址(相对于页表段)依次压栈,
;      返回值为pDest,存在eax中
memcpy:
    push ebp
    mov ebp, esp   ;此时ebp/esp均指向存储ebp的内存单元
    push esi
    push edi
    push ecx
    mov edi, [ebp + 8]  ;Destination
    mov esi, [ebp + 12]  ;Source
    mov ecx, [ebp + 16]   ;Counter
.LOOP_MEMCPY1:
    cmp ecx, 0
    jz .LOOP_MEMCPY2
    mov al, [ds:esi]
    inc esi
    mov [es:edi], al
    inc edi
    dec ecx
    jmp .LOOP_MEMCPY1
.LOOP_MEMCPY2:
    mov eax, [ebp + 8]  ;返回目标地址
    pop ecx
    pop edi
    pop esi
    pop ebp
    ret
;memcpy内存拷贝完毕

;功能：内存重置
;原型：void memset(void *p_dest, char ch, int size);
memset:
    push ebp
    mov ebp, esp
    push edi
    push edx
    push ecx
    mov edi, [ebp + 8]  ;Destination
    mov edx, [ebp + 12]  ;value to be putted
    mov ecx, [ebp + 16]   ;Counter
.LOOP_MEMSET1:
    cmp ecx, 0
    jz .LOOP_MEMSET2
    mov byte [edi], dl
    inc edi
    dec ecx
    jmp .LOOP_MEMSET1
.LOOP_MEMSET2:
    pop ecx
    pop edx
    pop edi
    pop ebp
    ret
;memset内存重置完毕

;功能：字符串拷贝
;原型：void *strcpy(void *p_dst, void *p_src);
strcpy:
    push ebp
    mov ebp, esp
    mov esi, [ebp + 12] ;source
    mov edi, [ebp + 8]  ;destination
.strcpy1:
    mov al, [esi]
    inc esi
    mov byte [edi], al
    inc edi
    cmp al, 0
    jnz .strcpy1
    mov eax, [ebp + 8]      ;返回值
    pop ebp
    ret
;strcpy字符串拷贝结束