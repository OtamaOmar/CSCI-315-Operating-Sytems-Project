#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("=== Scenario Test: Checkpoint at value 5 ===\n");

  int val = 1;
  printf("step 1: val = %d\n", val);

  val = 5;
  printf("step 2: val = %d\n", val);

  printf("checkpointing now (val should be 5)...\n");
  int r = checkpoint(0, "scenario.ckpt");

  if(r > 0){
    val = 10;
    printf("step 3: val = %d (after checkpoint, original continues)\n", val);
    printf("now restoring from checkpoint...\n");

    int pid = restore("scenario.ckpt");
    if(pid < 0){
      printf("FAIL: restore failed\n");
      exit(1);
    }
    int status;
    wait(&status);
    exit(0);
  }

  if(r < 0){
    printf("FAIL: checkpoint failed\n");
    exit(1);
  }

  printf("restored! checking val...\n");
  printf("val = %d\n", val);

  if(val == 5){
    printf("PASS: val is 5 -- checkpoint restored correctly\n");
  } else if(val == 1){
    printf("FAIL: val is 1 -- restored to wrong state (before checkpoint)\n");
  } else if(val == 10){
    printf("FAIL: val is 10 -- got post-checkpoint value instead\n");
  } else {
    printf("FAIL: val is %d -- unexpected value\n", val);
  }

  printf("=== Scenario Test Done ===\n");
  exit(0);
}
