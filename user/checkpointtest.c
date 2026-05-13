#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int value = 10;

  int r = checkpoint(0, "test_checkpoint");

  if(r < 0){
    printf("checkpoint failed\n");
    exit(1);
  }

  if(r == 0){
    printf("checkpoint saved, value = %d\n", value);

    value = 99;
    printf("value changed to %d\n", value);

    if(restore("test_checkpoint") < 0){
      printf("restore failed\n");
      exit(1);
    }
  } else {
    printf("after restore, value = %d\n", value);

    if(value == 10)
      printf("checkpoint/restore test passed\n");
    else
      printf("checkpoint/restore test failed\n");
  }

  exit(0);
}