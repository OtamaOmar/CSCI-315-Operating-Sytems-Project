#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int global_var = 42;
int global_array[5] = {10, 20, 30, 40, 50};

int
main(int argc, char *argv[])
{
  printf("=== Address Space Test ===\n");

  if(argc > 1 && strcmp(argv[1], "verify") == 0){
    int pid = restore("mem.ckpt");
    if(pid < 0){
      printf("FAIL: restore failed\n");
      exit(1);
    }
    wait(0);
    printf("PASS: restore created pid %d and it finished\n", pid);
    exit(0);
  }

  int stack_var = 123;
  int stack_array[3] = {100, 200, 300};
  char stack_str[16];
  strcpy(stack_str, "testdata");

  int r = checkpoint(0, "mem.ckpt");
  if(r > 0)
    exit(0);
  if(r < 0){
    printf("FAIL: checkpoint failed\n");
    exit(1);
  }

  int pass = 1;
  if(global_var != 42){ printf("FAIL: global_var=%d\n", global_var); pass = 0; }
  if(global_array[2] != 30){ printf("FAIL: global_array[2]=%d\n", global_array[2]); pass = 0; }
  if(stack_var != 123){ printf("FAIL: stack_var=%d\n", stack_var); pass = 0; }
  if(stack_array[1] != 200){ printf("FAIL: stack_array[1]=%d\n", stack_array[1]); pass = 0; }
  if(strcmp(stack_str, "testdata") != 0){ printf("FAIL: stack_str=%s\n", stack_str); pass = 0; }
  if(pass)
    printf("PASS: all memory values correct\n");

  global_var = 999;
  global_array[2] = 999;
  stack_var = 999;
  stack_array[1] = 999;
  strcpy(stack_str, "CORRUPT");

  printf("mutated all values\n");
  printf("now run: test_memory verify\n");
  printf("=== Address Space Test Done ===\n");
  exit(0);
}
