extern	main
extern	exit

[bits 32]
[section .text]

global _start
;ld链接程序真正识别的入口地址,它会去执行main函数，而main函数又去执行print函数
;而print函数又去执行write函数，而write又去执行send_recv函数
;而send_recv函数又去执行sendrec这个系统调用，
;而在系统调用中执行了一个 int INT_VECTOR_SYS_CALL这条命令，从而陷入到内核中执行
;从这里可以看出来，在运行时库函数中各种函数无非就是对各种int 0x90系统调用的封装
;最后都是陷入到内核中让操作系统帮助我们做事的
_start:
	push	eax     ;char *argv[]
	push	ecx     ;int argc
	call	main
	;; need not clean up the stack here

	push	eax      ;exit_status
	call	exit

	hlt	; should never arrive here