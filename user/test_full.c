#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int global_val = 55;

int
main(int argc, char *argv[])
{
  printf("=== Full Integration Test ===\n");

  if(argc > 1 && strcmp(argv[1], "verify") == 0){
    int pid = restore("/full.ckpt");
    if(pid < 0){ printf("FAIL: restore failed\n"); exit(1); }
    wait(0);
    printf("PASS: restore created pid %d and it finished\n", pid);
    exit(0);
  }

  int stack_val = 88;
  char str_val[16];
  strcpy(str_val, "hello");

  int fd = open("integ_file", O_CREATE | O_WRONLY);
  write(fd, "0123456789", 10);
  close(fd);
  fd = open("integ_file", O_RDONLY);
  char buf[16];
  read(fd, buf, 5);

  mkdir("integ_dir");
  chdir("integ_dir");

  int r = checkpoint(0, "/full.ckpt");
  if(r < 0){ printf("FAIL: checkpoint\n"); exit(1); }

  int pass = 1;

  if(global_val != 55){ printf("FAIL: global_val=%d\n", global_val); pass = 0; }
  if(stack_val != 88){ printf("FAIL: stack_val=%d\n", stack_val); pass = 0; }
  if(strcmp(str_val, "hello") != 0){ printf("FAIL: str_val=%s\n", str_val); pass = 0; }

  int n = read(fd, buf, 5);
  buf[n] = 0;
  if(strcmp(buf, "56789") != 0){ printf("FAIL: fd='%s'\n", buf); pass = 0; }

  int tfd = open("cwd_proof", O_CREATE | O_WRONLY);
  if(tfd < 0){ printf("FAIL: cwd broken\n"); pass = 0; }
  else { close(tfd); unlink("cwd_proof"); }

  if(pass)
    printf("PASS: all integration checks passed!\n");
  else
    printf("SOME TESTS FAILED\n");

  close(fd);
  printf("=== Full Integration Test Done ===\n");
  exit(0);
}
