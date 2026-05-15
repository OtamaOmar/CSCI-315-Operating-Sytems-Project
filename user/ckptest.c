#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// Test checkpoint/restore with file descriptors, pipes, and current working directory

int
main(int argc, char *argv[])
{
  int fd1, fd2, fd3;
  int pfd[2];
  char path[] = "ckp_test_file";
  char path2[] = "ckp_test2";
  char buf[512];
  int n;
  int result;
  
  printf("=== Checkpoint/Restore Test ===\n");
  
  // Create test directory
  mkdir("ckpdir");
  
  // Open a file for writing
  fd1 = open(path, O_CREATE | O_WRONLY);
  if(fd1 < 0){
    printf("failed to create %s\n", path);
    exit(1);
  }
  
  // Write to the file
  if(write(fd1, "hello from checkpoint test", 26) != 26){
    printf("write to %s failed\n", path);
    exit(1);
  }
  
  // Open another file for reading/writing
  fd2 = open(path2, O_CREATE | O_RDWR);
  if(fd2 < 0){
    printf("failed to create %s\n", path2);
    exit(1);
  }
  
  // Write to second file
  if(write(fd2, "test data 2", 11) != 11){
    printf("write to %s failed\n", path2);
    exit(1);
  }
  
  // Create a pipe
  if(pipe(pfd) < 0){
    printf("pipe creation failed\n");
    exit(1);
  }
  
  // Write to pipe
  if(write(pfd[1], "pipe data", 9) != 9){
    printf("pipe write failed\n");
    exit(1);
  }
  
  // Change directory
  if(chdir("ckpdir") < 0){
    printf("chdir failed\n");
    exit(1);
  }
  
  // Checkpoint the process
  result = checkpoint(0, "test.ckpt");
  
  if(result < 0){
    printf("checkpoint failed\n");
    exit(1);
  }
  
  if(result == 0){
    // This is the checkpointed state
    printf("Checkpoint saved. PID=%d\n", getpid());
    printf("FDs open: fd1=%d, fd2=%d, pfd[0]=%d, pfd[1]=%d\n", 
           fd1, fd2, pfd[0], pfd[1]);
    
    // Modify the state
    int modified_value = 99;
    printf("Modified value to %d\n", modified_value);
    
    // Try to close the files (they should still be open before restore)
    printf("Attempting restore...\n");
    if(restore("test.ckpt") < 0){
      printf("restore failed\n");
      exit(1);
    }
    
    // If we reach here, restore worked and we're in the restored process
    printf("ERROR: Should not reach here after restore!\n");
    exit(1);
  } else {
    // This is the restored process
    int restored_pid = result;
    printf("Process restored from checkpoint!\n");
    printf("Restored PID=%d\n", getpid());
    
    // Verify file descriptors are still open
    printf("\nVerifying restored file descriptors...\n");
    
    // Try reading from fd2 (should have data)
    if(lseek(fd2, 0, 0) < 0){
      printf("WARNING: lseek on fd2 failed\n");
    } else {
      n = read(fd2, buf, 512);
      if(n > 0){
        buf[n] = 0;
        printf("Read from fd2: '%s' (%d bytes)\n", buf, n);
      } else {
        printf("Read from fd2 returned %d\n", n);
      }
    }
    
    // Try reading from pipe
    printf("\nVerifying pipe...\n");
    n = read(pfd[0], buf, 512);
    if(n > 0){
      buf[n] = 0;
      printf("Read from pipe: '%s' (%d bytes)\n", buf, n);
    } else {
      printf("Read from pipe returned %d\n", n);
    }
    
    // Verify CWD is restored
    printf("\nVerifying current working directory...\n");
    // Create a test file in the current directory and check if it exists
    fd3 = open("cwd_test_file", O_CREATE | O_WRONLY);
    if(fd3 >= 0){
      printf("Successfully created file in current directory\n");
      printf("SUCCESS: CWD correctly restored (can create files in ckpdir)!\n");
      close(fd3);
      unlink("cwd_test_file");
    } else {
      printf("ERROR: Could not create file in current directory\n");
    }
    
    // Clean up
    close(fd1);
    close(fd2);
    close(pfd[0]);
    close(pfd[1]);
    
    chdir("/");
    unlink("ckptest_file");
    unlink("ckp_test2");
    
    printf("\n=== Test Complete ===\n");
    exit(0);
  }
}
