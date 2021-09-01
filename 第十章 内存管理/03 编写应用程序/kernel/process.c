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
#include "proto.h"

/*返回时钟中断数*/
PUBLIC int sys_get_ticks(){
    return ticks;
}

/*进程调度算法*/
PUBLIC void schedule(){
    PROCESS *p;
    int greatest_ticks = 0;
    while(!greatest_ticks){
        for(p = proc_table; p < proc_table + NR_TASKS + NR_PROCS; p++){
            if(p->p_flags == 0){
                //只有不阻塞时才会调度它
                //目前进程表数组中还有一些空闲插槽，因为p->p_flags等于 FREE_SLOT
                //所以也不会参与调度
                if(p->ticks > greatest_ticks){
                    greatest_ticks = p->ticks;
                    //切换进程
                    p_proc_ready = p;
                }
            }
        }
        if(!greatest_ticks){
            for(p = proc_table; p < proc_table + NR_TASKS + NR_PROCS; p++){
                if(p->p_flags == 0){
                    p->ticks = p->priority;
                }
            }
        }
    }
}

/******************************************************
 * 以下是进程间通信部分IPC
 ******************************************************/
PRIVATE int msg_send(PROCESS *current, int dest, MESSAGE *m);
PUBLIC int msg_receive(PROCESS *current, int src, MESSAGE *m);
/*系统级系统调用函数*/
//根据function决定发送还是接受消息,成功返回0
PUBLIC int sys_sendrec(int function, int src_dest, MESSAGE *m, PROCESS *p){
    assert(k_reenter == 0);   //确保是从非ring0进程调用了该系统调用,ring0 不能发送或接受消息
    assert((src_dest >= 0 && src_dest < NR_TASKS + NR_PROCS) ||
            src_dest == ANY || src_dest == INTERRUPT);       //消息来源或目的进程的断言
    int ret = 0;
    //获取调用者进程pid,也就是发送者或接受者
    int caller = proc2pid(p);
    //获取消息的线性地址，因为此时是在内核中运行，段地址为0
    MESSAGE *mla = (MESSAGE *)va2la(caller, m);
    mla->source = caller;   //消息归属
    assert(mla->source != src_dest);  //消息不能发给自己或者不能接受来自于自己的消息
    if(function == SEND){
        //发送只需指定发送者，接受者和消息
        ret = msg_send(p, src_dest, m);
        if(ret != 0){
            return ret;
        }
    }else if(function == RECEIVE){
        //发送只需指定发送者，接受者和消息
        ret = msg_receive(p, src_dest, m);
        if(ret != 0){
            return ret;
        }
    }else {
        //无效函数，CPU陷入停止状态
        panic("{sys_sendrec} invalid function: ""%d ,(SEND: %d, RECEIVE: %d).", function, SEND, RECEIVE);
    }
    return 0;  //成功
}

//计算给定进程给定段的段基址
PUBLIC int ldt_seg_linear(PROCESS *p, int idx){
    DESCRIPTOR *d = &p->ldts[idx];      //获取进程表ldt表中的指定描述符
    return d->base_high << 24 | d->base_mid << 16 | d->base_low;
}

//计算一个虚拟地址基于指定段的线性地址
PUBLIC void *va2la(int pid, void *va){
    PROCESS *p = &proc_table[pid];
    u32 seg_base = ldt_seg_linear(p, INDEX_LDT_RW);  //得到数据段基址
    u32 la = seg_base + (u32)va;
    if(pid < NR_TASKS + NR_PROCS){
        //assert(la == (u32)va);  //断言段基址为0
    }
    return (void*)la;
}

//得到一个空消息体
PUBLIC void reset_msg(MESSAGE *p){
    memset(p, 0, sizeof(MESSAGE));
}

//当设置完进程的p_flags为非0后，调用该函数阻塞进程
//其实就是挂起该进程，转去调度了另外一个进程
PRIVATE void block(PROCESS *p){
    assert(p->p_flags);
    schedule();
}

//在重置p_flags后，调用该函数，只是起到说明作用：该进程下次可以参与到调度之中
PRIVATE void unblock(PROCESS *p){
    assert(p->p_flags == 0);
}

//检测是否存在着死锁，即从src到dest发送消息是否是安全的
//具体来说，检测消息发送接受图是否存在着环cycle
//返回0说明不存在死锁
PRIVATE int deadlock(int src, int dest){
    PROCESS *p = proc_table + dest; //目标进程
    while(1){
        if(p->p_flags & SENDING){//目标进程现在也处理发送阻塞状态
            if(p->p_sendto == src){//并且目标进程也发送给源进程消息，那么存在死锁
                //打印出循环链
                p = proc_table + dest;  //重置p指针
                printf("=_=%s", p->p_name); //正常使用printf打印信息
                do {
                    assert(p->p_msg);       //消息体不空
                    p = proc_table + p->p_sendto;
                    printf("->%s", p->p_name);
                }while (p != proc_table + src);
                printf("=_=");
                return 1;  //检测出并且打印完毕直接返回了
            }
            p = proc_table + p->p_sendto; //检测接收进程
        }else {
            break;
        }
    }
    return 0;
}

//真正发送一个消息给一个进程，只需当前进程、目标进程pid、消息体
//如果目标进程正在等待接收当前进程的消息，将消息交给它，并解除阻塞即可
//否则，阻塞当前进程，并加入到目标进程的发送队列中
PRIVATE int msg_send(PROCESS *current, int dest, MESSAGE *m){
    PROCESS *sender = current;
    PROCESS *p_dest = proc_table + dest;
    assert(proc2pid(sender) != dest);
    //1.检测死锁
    if(deadlock(proc2pid(sender), dest)){//返回1表示存在死锁
        panic(">>DEADLOCK<< %s->%s", sender->p_name, p_dest->p_name);
    }
    //2.检测目标进程是否正在等待消息
    if((p_dest->p_flags & RECEIVING) &&
        (p_dest->p_recvfrom == proc2pid(sender) ||
        p_dest->p_recvfrom ==  ANY)){
        //当目标进程正在等待源进程的消息时或等待任意进程消息时，
        //直接把消息给它，并解除阻塞
        assert(p_dest->p_msg);  //此时目标进程准备好了一个空消息体
        assert(m);  //发送消息不为空
        //将消息传递给目标进程
        memcpy(va2la(dest, p_dest->p_msg),
                    va2la(proc2pid(sender), m),
                    sizeof(MESSAGE));
        p_dest->p_msg = 0;   //当不阻塞时应该将进程体中的消息指针清空
        p_dest->p_flags &= ~RECEIVING;  //当消息复制到目标进程的消息体中时，就意味值目标进程已经接收到了消息，可以处理了
        p_dest->p_recvfrom = NO_TASK;
        unblock(p_dest);  //解除阻塞，可以参与下次调度
        assert(p_dest->p_flags == 0);
        assert(p_dest->p_msg == 0);
        assert(p_dest->p_recvfrom == NO_TASK);
        assert(p_dest->p_sendto == NO_TASK);      //本身处理等待消息状态，也不会向其他人发送消息
        assert(sender->p_flags == 0);
        assert(sender->p_msg == 0);  //立刻向其他进程发送了消息，不会阻塞
        assert(sender->p_recvfrom == NO_TASK);
        assert(sender->p_sendto == NO_TASK);
    }else {
        //3.其他情况：目标进程要么没有阻塞，要么阻塞却等待的是另外一个进程的消息
        //此时，当前发送进程阻塞自己，并加入到对方的发送队列中
        sender->p_flags |= SENDING;  //阻塞
        assert(sender->p_flags == SENDING);
        sender->p_sendto = dest;
        sender->p_msg = m;
        //加入发送队列中
        PROCESS *p;
        if(p_dest->q_sending){
            p = p_dest->q_sending;
            while(p->next_sending){
                p = p->next_sending;
            }
            p->next_sending = sender;
        }else{
            p_dest->q_sending = sender;
        }
        sender->next_sending = 0;
        block(sender);
        assert(sender->p_flags == SENDING);
        assert(sender->p_msg != 0);
        assert(sender->p_recvfrom == NO_TASK);
        assert(sender->p_sendto == dest);
    }
    return 0;
}

/**
 * 尝试从一个源进程中获取一个消息，假如源进程正在等待该进程接收消息，
 * 直接接收消息并恢复源进程，否则，阻塞该进程。
 **/
PUBLIC int msg_receive(PROCESS *current, int src, MESSAGE *m){
    PROCESS *p_who_wanna_recv = current;
    PROCESS *p_from = 0;
    PROCESS *prev = 0;
    int copyok = 0;
    assert(proc2pid(p_who_wanna_recv) != src);  //不能从自身获取消息
    //1.判断是否有中断消息或可以接收中断消息或任意进程消息
    if((p_who_wanna_recv->has_int_msg) &&
        ((src == ANY) || (src == INTERRUPT))){
        MESSAGE msg;    //准备消息体，CPU只会产生中断信号，并不会产生消息体
        reset_msg(&msg);
        msg.source = INTERRUPT;
        msg.type = HARD_INT;   //来自于硬件中断
        assert(m);
        memcpy(va2la(proc2pid(p_who_wanna_recv), m),
            &msg, sizeof(MESSAGE));  //已经接收到消息
        p_who_wanna_recv->has_int_msg = 0;
        assert(p_who_wanna_recv->p_flags == 0);   //进程能以进入到这里，表明自身没有阻塞
        assert(p_who_wanna_recv->p_msg == 0);
        assert(p_who_wanna_recv->p_sendto == NO_TASK);
        assert(p_who_wanna_recv->has_int_msg == 0);
        return 0;
    }
    //2.可以接收任意进程的消息，则会从发送队列中选择第一个接收
    if(src == ANY){
        if(p_who_wanna_recv->q_sending){//发送队列不为空
            p_from = p_who_wanna_recv->q_sending;
            copyok = 1;
            assert(p_who_wanna_recv->p_flags == 0);
            assert(p_who_wanna_recv->p_msg == 0);
            assert(p_who_wanna_recv->p_recvfrom == NO_TASK);   //此时没有阻塞，p_recvfrom没有任务
            assert(p_who_wanna_recv->p_sendto == NO_TASK);
            assert(p_from->p_flags == SENDING);
            assert(p_from->p_msg != 0);
            assert(p_from->p_recvfrom == NO_TASK);
            assert(p_from->p_sendto == proc2pid(p_who_wanna_recv));
        }
    }else {
        //3.想要从一个特定进程中接收消息,并且该进程正在阻塞
        p_from = &proc_table[src];
        if((p_from->p_flags & SENDING) &&
            (p_from->p_sendto == proc2pid(p_who_wanna_recv))){
            copyok = 1;
            //此时，p_from因为已经阻塞了，所以它应该已经加入到了当前进程的发送队列中
            //所以应该将其从发送队列中取出
            PROCESS *p = p_who_wanna_recv->q_sending;
            assert(p);  //至少有一个进程
            while(p){
                assert(p_from->p_flags & SENDING);
                if (proc2pid(p) == src)
                {
                    p_from = p;
                    break;
                }
                prev = p;
                p = p->next_sending;
            }
            assert(p_who_wanna_recv->p_flags == 0);
            assert(p_who_wanna_recv->p_msg == 0);
            assert(p_who_wanna_recv->p_recvfrom == NO_TASK);
            assert(p_who_wanna_recv->p_sendto == NO_TASK);
            assert(p_who_wanna_recv->q_sending != 0);
            assert(p_from->p_flags == SENDING);
            assert(p_from->p_msg != 0);
            assert(p_from->p_recvfrom == NO_TASK);
            assert(p_from->p_sendto == proc2pid(p_who_wanna_recv));
        }
    }
    //4. 判断是否是可以接收任意进程或特定进程的消息，此时，消息可被复制
    if(copyok){
        //更新发送队列
        if(p_from == p_who_wanna_recv->q_sending){
            assert(prev == 0);
            p_who_wanna_recv->q_sending = p_from->next_sending;
            p_from->next_sending = 0;
        }else {
            assert(prev);
            prev->next_sending = p_from->next_sending;
            p_from->next_sending = 0;
        }
        assert(m);
        assert(p_from->p_msg);
        memcpy(va2la(proc2pid(p_who_wanna_recv), m),
                va2la(proc2pid(p_from), p_from->p_msg),
                sizeof(MESSAGE));
        p_from->p_msg = 0;
        p_from->p_sendto = NO_TASK;
        p_from->p_flags &= ~SENDING;
        unblock(p_from);
    }else{
        //5.没有可以接收的消息，阻塞该进程
        p_who_wanna_recv->p_flags |= RECEIVING;
        p_who_wanna_recv->p_msg = m;     //准备好空消息(形参空间)
        if(src == ANY){
            p_who_wanna_recv->p_recvfrom = ANY;
        }else{//就是特定进程，只不过特定进程还没有加入到发送队列中
            p_who_wanna_recv->p_recvfrom = proc2pid(p_from);
        }
        block(p_who_wanna_recv);
        assert(p_who_wanna_recv->p_flags == RECEIVING);
        assert(p_who_wanna_recv->p_msg != 0);
        assert(p_who_wanna_recv->p_recvfrom != NO_TASK);
        assert(p_who_wanna_recv->p_sendto == NO_TASK);
        assert(p_who_wanna_recv->has_int_msg == 0);
    }
    return 0;
}

/**
 * 通知一个进程，中断产生了
 **/
PUBLIC void inform_int(int task_nr){
    //被通知进程表
    PROCESS *p = proc_table + task_nr;
    if((p->p_flags & RECEIVING) &&
        ((p->p_recvfrom == INTERRUPT) || (p->p_recvfrom == ANY))){
        //假如一个进程正在等待一个中断,此时它已经准备好消息体
        p->p_msg->source = INTERRUPT;  //消息源是中断，不再是pid
        p->p_msg->type = HARD_INT;      //消息类型：硬件中断
        p->p_msg = 0;
        p->has_int_msg = 0;
        p->p_flags &= ~RECEIVING;
        p->p_recvfrom = NO_TASK;
        assert(p->p_flags == 0);    //已经可以参与调度了
        unblock(p);
        assert(p->p_flags == 0);
        assert(p->p_msg == 0);
        assert(p->p_recvfrom == NO_TASK);
        assert(p->p_sendto == NO_TASK);
    }else {
        p->has_int_msg = 1;  //等待进程需要接受消息时直接处理
    }
}