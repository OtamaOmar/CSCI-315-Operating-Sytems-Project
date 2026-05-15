#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  char buf[32];
  int fd, n;

  printf("=== FD Offset Test ===\n");

  if(argc > 1 && strcmp(argv[1], "verify") == 0){
    int pid = restore("fd.ckpt");
    if(pid < 0){
      printf("FAIL: restore failed\n");
      exit(1);
    }
    printf("PASS: restore returned pid %d\n", pid);
    wait(0);
    exit(0);
  }

  fd = open("fd_testfile", O_CREATE | O_WRONLY);
  if(fd < 0){ printf("FAIL: create\n"); exit(1); }
  write(fd, "ABCDEFGHIJ", 10);
  close(fd);

  fd = open("fd_testfile", O_RDONLY);
  if(fd < 0){ printf("FAIL: open\n"); exit(1); }

  n = read(fd, buf, 4);
  buf[n] = 0;
  printf("read before checkpoint: '%s'\n", buf);

  int r = checkpoint(0, "fd.ckpt");
  if(r < 0){ printf("FAIL: checkpoint\n"); close(fd); exit(1); }

  n = read(fd, buf, 3);
  buf[n] = 0;
  if(n == 3 && strcmp(buf, "EFG") == 0)
    printf("PASS: offset correct, read '%s'\n", buf);
  else
    printf("FAIL: expected 'EFG', got '%s'\n", buf);

  n = read(fd, buf, 3);
  buf[n] = 0;
  if(n == 3 && strcmp(buf, "HIJ") == 0)
    printf("PASS: continued read '%s'\n", buf);
  else
    printf("FAIL: expected 'HIJ', got '%s'\n", buf);

  close(fd);
  printf("=== FD Offset Test Done ===\n");
  exit(0);
}
