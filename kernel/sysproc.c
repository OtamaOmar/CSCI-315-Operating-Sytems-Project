#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
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
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    if(addr + n < addr)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

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
sys_checkpoint(void)
{
  int pid;
  int n;
  char path[MAXPATH];

  argint(0, &pid);
  if(pid == 0)
    pid = myproc()->pid;
  if(pid < 0)
    return -1;
  n = argstr(1, path, MAXPATH);
  if(n <= 1)
    return -1;

  extern struct proc proc[];
  struct proc *p = 0;
  for(struct proc *tp = proc; tp < &proc[NPROC]; tp++){
    acquire(&tp->lock);
    if(tp->pid == pid){
      p = tp;
      release(&tp->lock);
      break;
    }
    release(&tp->lock);
  }
  if(p == 0)
    return -1;

  return checkpoint_proc(p, path);
}

uint64
sys_restore(void)
{
  int n;
  char path[MAXPATH];

  n = argstr(0, path, MAXPATH);
  if(n <= 1)
    return -1;

  return restore_proc(path);
}
