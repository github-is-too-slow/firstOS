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

PRIVATE void cleanup(PROCESS * proc);

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
	// printf("{MM} child_base: 0x%x <- parent_base: 0x%x (0x%x bytes)\n",
	//        child_base, caller_T_base, caller_T_size);
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

/*****************************************************************************
 *                                do_exit
 *****************************************************************************/
/**
 * Perform the exit() syscall.
 *
 * If proc A calls exit(), then MM will do the following in this routine:
 *     <1> inform FS so that the fd-related things will be cleaned up
 *     <2> free A's memory
 *     <3> set A.exit_status, which is for the parent
 *     <4> depends on parent's status. if parent (say P) is:
 *           (1) WAITING
 *                 - clean P's WAITING bit, and
 *                 - send P a message to unblock it
 *                 - release A's proc_table[] slot
 *           (2) not WAITING
 *                 - set A's HANGING bit
 *     <5> iterate proc_table[], if proc B is found as A's child, then:
 *           (1) make INIT the new parent of B, and
 *           (2) if INIT is WAITING and B is HANGING, then:
 *                 - clean INIT's WAITING bit, and
 *                 - send INIT a message to unblock it
 *                 - release B's proc_table[] slot
 *               else
 *                 if INIT is WAITING but B is not HANGING, then
 *                     - B will call exit()
 *                 if B is HANGING but INIT is not WAITING, then
 *                     - INIT will call wait()
 *
 * TERMs:
 *     - HANGING: everything except the proc_table entry has been cleaned up.
 *     - WAITING: a proc has at least one child, and it is waiting for the
 *                child(ren) to exit()
 *     - zombie: say P has a child A, A will become a zombie if
 *         - A exit(), and
 *         - P does not wait(), neither does it exit(). that is to say, P just
 *           keeps running without terminating itself or its child
 *
 * @param status  Exiting status for parent.
 * 子进程调用exit()，会转到该方法处理
 *****************************************************************************/
PUBLIC void do_exit(int status)
{
	int i;
	int pid = mm_msg.source; /* PID of caller */
	int parent_pid = proc_table[pid].p_parent;
	PROCESS *p = &proc_table[pid];

	/* tell FS, see fs_exit() */
	//告诉文件系统进程处理文件共享操作
	MESSAGE msg2fs;
	msg2fs.type = EXIT;
	msg2fs.PID = pid;
	send_recv(BOTH, TASK_FS, &msg2fs);
	//释放子进程的内存空间
	free_mem(pid);
	//将用户设置的退出状态码放入进程表中
	p->exit_status = status;
	//通过判断父进程的状态决定下一步的处理动作
	if (proc_table[parent_pid].p_flags & WAITING) { /* parent is waiting */
		//父进程提前调用了wait(),此时父进程处于阻塞waiting状态和receiving状态
		//解除阻塞
		proc_table[parent_pid].p_flags &= ~WAITING;
		//清除子进程的进程表以及向父进程发送一个消息(该消息中携带有子进程的退出值)，以改变receiving状态
		cleanup(&proc_table[pid]);
	}
	else { /* parent is not waiting */
		//此后子进程的进程表还没有清理，处于僵尸状态，直至父进程调用wait()
		proc_table[pid].p_flags |= HANGING;
	}

	/* if the proc has any child, make INIT the new parent */
	//循环扫描进程表数组，将退出进程的子进程过继给父进程
	for (i = 0; i < NR_TASKS + NR_PROCS; i++) {
		if (proc_table[i].p_parent == pid) { /* is a child */
			//过继给父进程
			proc_table[i].p_parent = INIT;
			if ((proc_table[INIT].p_flags & WAITING) &&
			    (proc_table[i].p_flags & HANGING)) {
				//手动处理父进程waiting和子进程hanging的死锁状态
				proc_table[INIT].p_flags &= ~WAITING;
				cleanup(&proc_table[i]);
			}
		}
	}
}

/*****************************************************************************
 *                                cleanup
 *****************************************************************************/
/**
 * Do the last jobs to clean up a proc thoroughly:
 *     - Send proc's parent a message to unblock it, and
 *     - release proc's proc_table[] entry
 *	回收子进程的进程表，将进程表中的退出值发送给父进程，以解除父进程阻塞
 * @param proc  Process to clean up.
 *****************************************************************************/
PRIVATE void cleanup(PROCESS *proc)
{
	MESSAGE msg2parent;
	msg2parent.type = SYSCALL_RET;
	msg2parent.PID = proc2pid(proc);
	msg2parent.STATUS = proc->exit_status;
	send_recv(SEND, proc->p_parent, &msg2parent);
	//回收进程表
	proc->p_flags = FREE_SLOT;
}

/*****************************************************************************
 *                                do_wait
 *****************************************************************************/
/**
 * Perform the wait() syscall.
 *
 * If proc P calls wait(), then MM will do the following in this routine:
 *     <1> iterate proc_table[],
 *         if proc A is found as P's child and it is HANGING
 *           - reply to P (cleanup() will send P a messageto unblock it)
 *           - release A's proc_table[] entry
 *           - return (MM will go on with the next message loop)
 *     <2> if no child of P is HANGING
 *           - set P's WAITING bit
 *     <3> if P has no child at all
 *           - reply to P with error
 *     <4> return (MM will go on with the next message loop)
 * 父进程调用wait()会转到这里处理等待子进程的退出操作
 *****************************************************************************/
PUBLIC void do_wait()
{
	int pid = mm_msg.source;

	int i;
	int children = 0;
	PROCESS *p_proc = proc_table;
	for (i = 0; i < NR_TASKS + NR_PROCS; i++,p_proc++) {
		//假如有一个进程是该进程的子进程并且处于僵尸状态，等待提取它的退出值
		if (p_proc->p_parent == pid) {
			children++;
			if (p_proc->p_flags & HANGING) {
				//回收子进程的进程表，并且向该进程发送了一个消息
				//该处理过程是由MM管理模块的ring1->ring0,然后发送完消息后转到ring1,此时父进程已经能调度了，
				//此时必须返回由ring1->ring3，等父进程处理完该子进程的退出操作后，等待他再次主动发出wait，再寻找下一个退出子进程
				cleanup(p_proc);
				return;
			}
		}
	}

	if (children) {
		//有子进程但还没有退出，该进程就等待它退出
		/* has children, but no child is HANGING */
		proc_table[pid].p_flags |= WAITING;
	}
	else {
		/* no child at all */
		//没有子进程，什么也不做，解除阻塞即可
		MESSAGE msg;
		msg.type = SYSCALL_RET;
		msg.PID = NO_TASK;  //该值作为用户接口wait返回值的依据
		send_recv(SEND, pid, &msg);
	}
}