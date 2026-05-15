#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct stat st;
  int fd;

  printf("=== Serialization Test ===\n");

  if(argc > 1 && strcmp(argv[1], "check") == 0){
    if(stat("serial_test.ckpt", &st) < 0){
      printf("FAIL: file does not exist\n");
      exit(1);
    }
    printf("PASS: file exists\n");

    if(st.size == 0){
      printf("FAIL: file is empty\n");
      exit(1);
    }
    printf("PASS: file size = %d bytes\n", (int)st.size);

    fd = open("serial_test.ckpt", O_RDONLY);
    if(fd < 0){
      printf("FAIL: cannot open file\n");
      exit(1);
    }

    uint magic;
    int n = read(fd, (char*)&magic, sizeof(magic));
    if(n != sizeof(magic)){
      printf("FAIL: cannot read magic\n");
      close(fd);
      exit(1);
    }

    if(magic == 0x43484B50)
      printf("PASS: magic number correct (0x%x)\n", magic);
    else
      printf("FAIL: wrong magic 0x%x\n", magic);

    close(fd);
    printf("=== Serialization Test Done ===\n");
    exit(0);
  }

  int r = checkpoint(0, "serial_test.ckpt");
  if(r < 0){
    printf("FAIL: checkpoint failed\n");
    exit(1);
  }

  printf("PASS: checkpoint created\n");
  printf("now run: test_serial check\n");
  printf("=== Serialization Test Done ===\n");
  exit(0);
}
