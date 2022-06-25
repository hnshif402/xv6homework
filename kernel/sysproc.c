#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  backtrace();
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_sigalarm(void)
{
  int interval;
  uint64 fn;

  if(argint(0, &interval) < 0)
    return -1;
  if(argaddr(1, &fn) < 0)
    return -1;

  //printf("interval = %d\n", interval);
  //printf("fn address: %p\n", fn);
  struct proc *p = myproc();
    /* printf("sigalarm:\n"); */
    /* printf("ra:%x,fp:%x,s1:%x,s2:%x,s3:%x,s4:%x,s5:%x,sp:%x,pc:%x\n", */
    /*      p->trapframe->ra, */
    /*      p->trapframe->s0, */
    /*      p->trapframe->s1, */
    /*      p->trapframe->s2, */
    /*      p->trapframe->s3, */
    /*      p->trapframe->s4, */
    /*      p->trapframe->s5, */
    /*      p->trapframe->sp, */
    /*      p->trapframe->epc); */
  p->interval = interval;
  p->expire_ticks = 0;
  p->handler = fn;

  //printf("ra=%p\n", p->trapframe->ra);
  
  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  /* printf("sigreturn:\n"); */
  /* printf("ra:%x,fp:%x,s1:%x,s2:%x,s3:%x,s4:%x,s5:%x,sp:%x,pc:%x\n", */
  /*        p->trapframe->ra, */
  /*        p->trapframe->s0, */
  /*        p->trapframe->s1, */
  /*        p->trapframe->s2, */
  /*        p->trapframe->s3, */
  /*        p->trapframe->s4, */
  /*        p->trapframe->s5, */
  /*        p->trapframe->sp, */
  /*        p->trapframe->epc); */
  p->trapframe->epc = p->uepc;
  p->trapframe->sp = p->usp;
  p->trapframe->s0 = p->ufp;
  p->trapframe->s1 = p->us1;
  p->trapframe->s2 = p->us2;
  p->trapframe->s3 = p->us3;
  p->trapframe->s4 = p->us4;
  p->trapframe->s5 = p->us5;
  p->trapframe->s6 = p->us6;
  p->trapframe->s7 = p->us7;
  p->trapframe->ra = p->ura;
  p->trapframe->a0 = p->ua0;
  p->trapframe->a1 = p->ua1;
  p->trapframe->a2 = p->ua2;
  p->trapframe->a3 = p->ua3;
  p->trapframe->a4 = p->ua4;
  p->expire_ticks = 0;

  //printf("sigreturn: ra=%p\n", p->trapframe->ra);
  
  return 0;
}
