;段描述符
;使用: Descriptor Base, Limit, Attr
;      Base:   dd双字4字节
;      Limit:  dd双字4字节
;      Attr:   dw字2字节
%macro Descriptor 3
    dw %2 & 0FFFFh   ;段界限1
    dw %1 & 0FFFFh   ;段基址1
    db (%1 >> 16) & 0FFh   ;段基址2
    dw ((%2 >> 8) & 0F00h) | (%3 & 0F0FFh)   ;属性1 + 段界限2 + 属性2
    db (%1 >> 24) & 0FFh   ;段基址3
%endmacro  ;描述符共8字节

;描述符属性值
;描述符D/B位
DA_32           equ             4000h           ;32位段

;存储段/数据段描述符属性
DA_DR           equ             90h            ;存在的可读数据段
DA_DRW          equ             92h            ;存在的可读写数据段
DA_DRWA         equ             93h            ;存在的已访问可读写数据段

;代码段描述符属性
DA_C            equ             98h            ;存在的只执行(非一致)代码段

;系统段描述符属性
SA_TIL          equ             4h              ;局部描述符选择域TI位置为1
DA_LDT          equ             82h             ;存在的LDT系统段