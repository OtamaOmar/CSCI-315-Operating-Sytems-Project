#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pfd[2];
  int r;

  printf("=== Pipe Constraint Test ===\n");

  printf("\nTest 1: self-contained pipe\n");
  if(pipe(pfd) < 0){ printf("FAIL: pipe\n"); exit(1); }

  r = checkpoint(0, "pipe_ok.ckpt");
  if(r >= 0)
    printf("PASS: checkpoint succeeded\n");
  else{
    printf("FAIL: checkpoint failed\n");
    close(pfd[0]); close(pfd[1]); exit(1);
  }
  close(pfd[0]);
  close(pfd[1]);

  printf("\nTest 2: child with one pipe end\n");
  if(pipe(pfd) < 0){ printf("FAIL: pipe\n"); exit(1); }

  int pid = fork();
  if(pid < 0){ printf("FAIL: fork\n"); exit(1); }

  if(pid == 0){
    close(pfd[1]);
    r = checkpoint(0, "pipe_child.ckpt");
    if(r == -1)
      printf("PASS: child rejected\n");
    else
      printf("FAIL: child should fail, got %d\n", r);
    close(pfd[0]);
    exit(0);
  } else {
    wait(0);
    close(pfd[0]);

    printf("\nTest 3: parent with one pipe end\n");
    r = checkpoint(0, "pipe_parent.ckpt");
    if(r == -1)
      printf("PASS: parent rejected\n");
    else
      printf("FAIL: parent should fail, got %d\n", r);
    close(pfd[1]);
  }

  printf("\n=== Pipe Constraint Test Done ===\n");
  exit(0);
}
