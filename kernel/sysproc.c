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
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
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
sys_checkpoint(void)
{
  int pid;
  int n;
  char path[MAXPATH];
  struct proc *p = myproc();

  argint(0, &pid);
  if(pid == 0)
    pid = p->pid;
  if(pid < 0 || pid != p->pid)
    return -1;

  n = argstr(1, path, MAXPATH);
  if(n <= 1)
    return -1;

  int npages = (p->sz + PGSIZE - 1) / PGSIZE;
  if(npages > CKPT_MAX_PAGES)
    return -1;

  for(int i = 0; i < p->checkpoint_npages; i++){
    if(p->checkpoint_pages[i]){
      kfree(p->checkpoint_pages[i]);
      p->checkpoint_pages[i] = 0;
    }
  }

  p->checkpoint_npages = npages;
  p->checkpoint_sz = p->sz;
  p->checkpoint_tf = *(p->trapframe);

  for(int i = 0; i < npages; i++){
    p->checkpoint_pages[i] = kalloc();
    if(p->checkpoint_pages[i] == 0)
      return -1;

    memset(p->checkpoint_pages[i], 0, PGSIZE);

    uint64 va = i * PGSIZE;
    uint64 bytes = PGSIZE;

    if(va + bytes > p->sz)
      bytes = p->sz - va;

    if(copyin(p->pagetable, p->checkpoint_pages[i], va, bytes) < 0)
      return -1;
  }

  p->checkpoint_valid = 1;

  printf("checkpoint saved for pid %d at %s\n", pid, path);
  return 0;
}

uint64
sys_restore(void)
{
  int n;
  char path[MAXPATH];
  struct proc *p = myproc();

  n = argstr(0, path, MAXPATH);
  if(n <= 1)
    return -1;

  if(p->checkpoint_valid == 0)
    return -1;

  if(p->sz < p->checkpoint_sz){
    if(growproc(p->checkpoint_sz - p->sz) < 0)
      return -1;
  }

  for(int i = 0; i < p->checkpoint_npages; i++){
    uint64 va = i * PGSIZE;
    uint64 bytes = PGSIZE;

    if(va + bytes > p->checkpoint_sz)
      bytes = p->checkpoint_sz - va;

    if(copyout(p->pagetable, va, p->checkpoint_pages[i], bytes) < 0)
      return -1;
  }

  *(p->trapframe) = p->checkpoint_tf;

  printf("process restored from %s\n", path);
  return 1;
}