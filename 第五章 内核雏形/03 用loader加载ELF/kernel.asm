[section .text]
global _start
_start:
    mov ah, 0Fh     ;黑底白字
    mov al, 'K'
    mov [gs:((80 * 1 + 39) * 2)], ax
    jmp $