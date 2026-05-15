#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int r;

  printf("=== Syscall Plumbing Test ===\n");

  if(argc > 1 && strcmp(argv[1], "restore") == 0){
    r = restore("syscall_test.ckpt");
    if(r > 0){
      printf("PASS: restore returned pid %d\n", r);
      wait(0);
    } else {
      printf("FAIL: restore returned %d\n", r);
    }
    r = restore("nonexistent.ckpt");
    if(r == -1)
      printf("PASS: bad restore path returned -1\n");
    else
      printf("FAIL: bad restore path returned %d\n", r);
    printf("=== Restore Tests Done ===\n");
    exit(0);
  }

  r = checkpoint(0, "syscall_test.ckpt");
  if(r > 0)
    exit(0);  // restored child: exit silently so parent's wait(0) unblocks cleanly
  if(r == 0)
    printf("PASS: checkpoint returned 0\n");
  else
    printf("FAIL: checkpoint returned %d, expected 0\n", r);

  r = checkpoint(0, "");
  if(r == -1)
    printf("PASS: empty path returned -1\n");
  else
    printf("FAIL: empty path returned %d\n", r);

  r = checkpoint(9999, "bad_pid.ckpt");
  if(r == -1)
    printf("PASS: bad pid returned -1\n");
  else
    printf("FAIL: bad pid returned %d\n", r);

  printf("=== Checkpoint Tests Done ===\n");
  exit(0);
}
