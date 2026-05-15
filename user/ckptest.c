#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static int
pipe_cross_process_should_fail(void)
{
  int pfd[2];
  int pid;
  int st = 0;

  if(pipe(pfd) < 0){
    printf("pipe-fail: could not create pipe\n");
    return -1;
  }

  pid = fork();
  if(pid < 0){
    close(pfd[0]);
    close(pfd[1]);
    printf("pipe-fail: fork failed\n");
    return -1;
  }

  if(pid == 0){
    // Child owns write end so parent has only one end.
    close(pfd[0]);
    pause(200);
    exit(0);
  }

  close(pfd[1]);
  if(checkpoint(0, "pipe_cross_fail.ckpt") != -1){
    close(pfd[0]);
    wait(&st);
    printf("pipe-fail: expected checkpoint failure, got success\n");
    return -1;
  }

  close(pfd[0]);
  wait(&st);
  return 0;
}

int
main(void)
{
  int fd;
  int pfd[2];
  int r;
  int n;
  int pid;
  int ok = 1;
  char buf[32];
  int value = 42;

  printf("ckptest: start\n");

  if(pipe_cross_process_should_fail() < 0){
    printf("ckptest: cross-process pipe constraint test failed\n");
    exit(1);
  }
  printf("ckptest: cross-process pipe constraint test passed\n");

  mkdir("ckpdir");
  fd = open("fd_offset_test", O_CREATE | O_RDWR);
  if(fd < 0){
    printf("ckptest: open failed\n");
    exit(1);
  }
  if(write(fd, "AAAA", 4) != 4){
    printf("ckptest: initial write failed\n");
    exit(1);
  }
  if(pipe(pfd) < 0){
    printf("ckptest: self-contained pipe create failed\n");
    exit(1);
  }
  if(chdir("ckpdir") < 0){
    printf("ckptest: chdir failed\n");
    exit(1);
  }

  r = checkpoint(0, "state.ckpt");
  if(r < 0){
    printf("ckptest: checkpoint failed\n");
    exit(1);
  }

  if(r == 0){
    // Original process mutates state after checkpoint.
    value = 99;
    if(write(fd, "BBBB", 4) != 4){
      printf("ckptest: mutation write failed\n");
      exit(1);
    }

    pid = restore("state.ckpt");
    if(pid < 0){
      printf("ckptest: restore failed\n");
      exit(1);
    }
    printf("ckptest: restore created pid %d\n", pid);
    exit(0);
  }

  // Restored process path (checkpoint return value forced to 1).
  if(value != 42){
    printf("ckptest: memory restore failed (value=%d)\n", value);
    ok = 0;
  } else {
    printf("ckptest: memory restore passed\n");
  }

  // FD offset test:
  // At checkpoint offset was 4; original wrote "BBBB" after checkpoint.
  // Restored write should happen at offset 4 and overwrite "BBBB".
  if(write(fd, "CCCC", 4) != 4){
    printf("ckptest: restored write failed\n");
    ok = 0;
  }
  close(fd);

  fd = open("/fd_offset_test", O_RDONLY);
  if(fd < 0){
    printf("ckptest: reopen failed\n");
    ok = 0;
  } else {
    n = read(fd, buf, 8);
    close(fd);
    if(n != 8 || memcmp(buf, "AAAACCCC", 8) != 0){
      printf("ckptest: fd offset restore failed\n");
      ok = 0;
    } else {
      printf("ckptest: fd offset restore passed\n");
    }
  }

  // Pipe usability test: pipe content is not persisted, but restored ends must work.
  if(write(pfd[1], "P", 1) != 1 || read(pfd[0], buf, 1) != 1 || buf[0] != 'P'){
    printf("ckptest: self-contained pipe restore failed\n");
    ok = 0;
  } else {
    printf("ckptest: self-contained pipe restore passed\n");
  }

  // CWD restore test: checkpoint was taken inside /ckpdir.
  fd = open("cwd_ok", O_CREATE | O_WRONLY);
  if(fd < 0){
    printf("ckptest: cwd restore failed\n");
    ok = 0;
  } else {
    close(fd);
    unlink("cwd_ok");
    printf("ckptest: cwd restore passed\n");
  }

  if(ok){
    printf("ckptest: ALL TESTS PASSED\n");
    exit(0);
  }

  printf("ckptest: TESTS FAILED\n");
  exit(1);
}
