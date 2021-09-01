#include "type.h"
#include "const.h"
#include "console.h"
#include "tty.h"
#include "string.h"
#include "protect.h"
#include "fs.h"
#include "stdio.h"
#include "process.h"
#include "hd.h"
#include "keyboard.h"
#include "global.h"
#include "elf.h"
#include "proto.h"

/*****************************************************************************
 *                                do_fork
 *****************************************************************************/
/**
 * Perform the fork() syscall.
 *
 * @return  Zero if success, otherwise -1.
 *****************************************************************************/
PUBLIC int do_fork()
{
	/* find a free slot in proc_table */
    //寻找一个空闲进程表
	PROCESS* p = proc_table;
	int i;
	for (i = 0; i < NR_TASKS + NR_PROCS; i++,p++)
		if (p->p_flags == FREE_SLOT)
			break;
    //进程表数组的下标就是子进程的pid
	int child_pid = i;
	assert(p == &proc_table[child_pid]);
	assert(child_pid >= NR_TASKS + NR_NATIVE_PROCS);
	if (i == NR_TASKS + NR_PROCS) /* no free slot没有找到返回-1 */
		return -1;
	assert(i < NR_TASKS + NR_PROCS);

	/* duplicate 复制进程表the process table */
    //发送fork消息的父进程pid
    //目前p指向空闲进程表，也即子进程表
	int pid = mm_msg.source;
    //子进程的局部描述符表在全局描述符中的选择子
	u16 child_ldt_sel = p->ldt_sel;
    //将父进程的进程表原封不动的复制给子进程表
	*p = proc_table[pid];
    //修改选择子
	p->ldt_sel = child_ldt_sel;
    //父进程pid
	p->p_parent = pid;
	p->pid = child_pid;
    //指定子进程名
	sprintf(p->p_name, "%s_%d", proc_table[pid].p_name, child_pid);

	/* duplicate the process: T, D & S */
	DESCRIPTOR * ppd;

	/* Text segment父进程代码段描述符 */
	ppd = &proc_table[pid].ldts[INDEX_LDT_C];
	/* base of T-seg, in bytes */
	int caller_T_base  = reassembly(ppd->base_high, 24,
					ppd->base_mid,  16,
					ppd->base_low);
	/* limit of T-seg, in 1 or 4096 bytes,
	   depending on the G bit of descriptor */
	int caller_T_limit = reassembly(0, 0,
					(ppd->limit_high_attr2 & 0xF), 16,
					ppd->limit_low);
	/* size of T-seg, in bytes */
    //父进程代码段大小，单位：字节
	int caller_T_size  = ((caller_T_limit + 1) *
			      ((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ?
			       4096 : 1));

	/* Data & Stack segments */
	ppd = &proc_table[pid].ldts[INDEX_LDT_RW];
	/* base of D&S-seg, in bytes */
	int caller_D_S_base  = reassembly(ppd->base_high, 24,
					  ppd->base_mid,  16,
					  ppd->base_low);
	/* limit of D&S-seg, in 1 or 4096 bytes,
	   depending on the G bit of descriptor */
	int caller_D_S_limit = reassembly((ppd->limit_high_attr2 & 0xF), 16,
					  0, 0,
					  ppd->limit_low);
	/* size of D&S-seg, in bytes */
    //父进程数据段段大小，单位：字节
	int caller_D_S_size  = ((caller_T_limit + 1) *
				((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ?
				 4096 : 1));

	/* we don't separate T, D & S segments, so we have: */
    //每个进程的代码段、数据段公用同一个段
	assert((caller_T_base  == caller_D_S_base ) &&
	       (caller_T_limit == caller_D_S_limit) &&
	       (caller_T_size  == caller_D_S_size ));

	/* base of child proc, T, D & S segments share the same space,
	   so we allocate memory just once */
    //为子进程分配一段内存地址，并返回内存首址
	int child_base = alloc_mem(child_pid, caller_T_size);
	/* int child_limit = caller_T_limit; */
	printf("{MM} 0x%x <- 0x%x (0x%x bytes)\n",
	       child_base, caller_T_base, caller_T_size);
	/* child is a copy of the parent */
    //将父进程的代码和数据原封不动的复制到子进程
	memcpy((void*)child_base, (void*)caller_T_base, caller_T_size);

	/* child's LDT */
    //更新子进程的ldt，使其指向子进程的空间
	init_descriptor(&p->ldts[INDEX_LDT_C],
		  child_base,
		  (PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHIFT,
		  DA_LIMIT_4K | DA_32 | DA_C | PRIVILEGE_USER << 5);
	init_descriptor(&p->ldts[INDEX_LDT_RW],
		  child_base,
		  (PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHIFT,
		  DA_LIMIT_4K | DA_32 | DA_DRW | PRIVILEGE_USER << 5);

	/* tell FS, see fs_fork() */
    //因为父进程的filp也复制给了子进程，需要FS进程解决文件共享的问题
	MESSAGE msg2fs;
	msg2fs.type = FORK;
	msg2fs.PID = child_pid;
	send_recv(BOTH, TASK_FS, &msg2fs);

	/* child PID will be returned to the parent proc */
	mm_msg.PID = child_pid;

	/* birth of the child */
    //因为进程表中关于进程间IPC的各种成员也复制给了子进程，需要向其发送消息以解除子进程的阻塞
	MESSAGE m;
	m.type = SYSCALL_RET;
	m.RETVAL = 0;
    //子进程的各种寄存器状态跟父进程一样，所以它解除阻塞后，会立即执行fork函数的返回赋值动作
    //子进程fork的返回值是0
	m.PID = 0;
	send_recv(SEND, child_pid, &m);
    //最终父进程的fork动作也完成，由mm进程向其发送消息
	return 0;
}