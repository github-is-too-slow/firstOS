org 0100h  ;为了扩展，加载到某个段内偏移0x100处

mov ax, 0B800h
mov gs, ax
mov ah, 0Dh     ;黑底白字
mov al, 'L'
mov [gs:((80 * 0 + 39) * 2)], ax
jmp $
