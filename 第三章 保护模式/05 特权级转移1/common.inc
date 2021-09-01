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

;调用门描述符
;使用：Gate Selector, Offset, DCount, Attr
;       Selector:   dw
;       Offset:     dd
;       DCount:     db
;       Attr:       db
%macro Gate 4
    dw (%2 & 0FFFFh)    ;偏移1
    dw %1               ;段选择子
    dw (%3 & 1Fh) | ((%4 << 8) & 0FF00h)    ;属性
    dw ((%2 >>16) & 0FFFFH)         ;偏移2
%endmacro   ;共8字节

;描述符属性值
;描述符D/B位
DA_32           equ             4000h           ;32位段

;存储段/数据段描述符属性
DA_DR           equ             90h            ;存在的可读数据段
DA_DRW          equ             92h            ;存在的可读写数据段
DA_DRWA         equ             93h            ;存在的已访问可读写数据段
DA_DPL0         equ             00h            ;DPL=0
DA_DPL1         equ             20h            ;DPL=1
DA_DPL2         equ             40h            ;DPL=2
DA_DPL3         equ             60h            ;DPL=3

;代码段描述符属性
DA_C            equ             98h            ;存在的只执行(非一致)代码段

;系统段描述符属性
SA_TIL          equ             4h              ;局部描述符选择域TI位置为1
DA_LDT          equ             82h             ;存在的LDT系统段
DA_386CGate     equ             8Ch             ;存在的386调用门类型段

;选择子属性
SA_RPL0         equ             0               ;RPL=0
SA_RPL1         equ             1               ;RPL=1
SA_RPL2         equ             2               ;RPL=2
SA_RPL3         equ             3               ;RPL=3