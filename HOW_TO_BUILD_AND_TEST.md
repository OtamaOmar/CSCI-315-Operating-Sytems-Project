# How to Build and Test the Checkpoint/Restore Implementation

## Prerequisites

You need to have the RISC-V toolchain and QEMU installed on your system. The project was developed for Ubuntu with the following tools:
- riscv64-unknown-elf-gcc (RISC-V GNU Compiler)
- riscv64-unknown-elf-ld (RISC-V Linker)
- qemu-system-riscv64 (QEMU RISC-V Emulator)

### Installing on Ubuntu

```bash
# Update package list
sudo apt update

# Install RISC-V toolchain
sudo apt install gcc-riscv64-unknown-elf

# Install QEMU for RISC-V
sudo apt install qemu-system-misc

# Verify installation
riscv64-unknown-elf-gcc --version
qemu-system-riscv64 --version
```

## Building xv6 with Checkpoint/Restore Support

### Step 1: Navigate to the xv6 Directory

```bash
cd /path/to/CSCI-315-Operating-Systems-Project
```

### Step 2: Clean Previous Build

```bash
make clean
```

### Step 3: Build xv6

```bash
make
```

This will:
- Compile all kernel modules including the updated proc.c and fs.c
- Compile the filesystem image with the new ckptest program
- Link the kernel
- Create qemu image

### Step 4: Run xv6 in QEMU

```bash
make qemu
```

You should see output like:
```
xv6 kernel is booting

hart 2 starting
hart 1 starting
hart 0 starting
init: starting sh
$
```

## Testing the Checkpoint/Restore Implementation

### Test 1: Run the Comprehensive Test Program

Once the xv6 shell is running:

```
$ ckptest
```

This will execute the comprehensive test that:
1. Creates test files in the filesystem
2. Opens files for reading and writing
3. Creates a pipe and writes data
4. Changes to a subdirectory
5. **Checkpoints** the process state
6. **Restores** from the checkpoint
7. **Verifies** that all resources are correctly restored

**Expected Output:**
```
=== Checkpoint/Restore Test ===
Checkpoint saved. PID=...
FDs open: fd1=3, fd2=4, pfd[0]=5, pfd[1]=6
Modified value to 99
Attempting restore...
Process restored from checkpoint!
Restored PID=...

Verifying restored file descriptors...
Read from fd2: 'test data 2' (11 bytes)

Verifying pipe...
Read from pipe: 'pipe data' (9 bytes)

Verifying current working directory...
Successfully created file in current directory
SUCCESS: CWD correctly restored (can create files in ckpdir)!

=== Test Complete ===
$
```

### Test 2: Run the Original Checkpoint Test

You can also run the simpler original checkpoint test:

```
$ checkpointtest
```

This tests basic memory and trapframe restoration.

### Test 3: Manual Testing

You can manually test checkpoint/restore functionality:

```
$ cat > test.txt
Hello from checkpoint test
<Ctrl+D>

$ cat test.txt
Hello from checkpoint test

$ sh
$ (inside a new shell) checkpoint(0, "/tmp/myckpt");
```

## Troubleshooting

### Make Errors

**Error:** `make: riscv64-unknown-elf-gcc: command not found`
- **Solution:** Install the RISC-V toolchain as described in Prerequisites

**Error:** `make: qemu-system-riscv64: command not found`
- **Solution:** Install QEMU as described in Prerequisites

### Compilation Errors

**Error:** `undefined reference to 'iget_by_inum'`
- **Solution:** Make sure kernel/fs.c has the iget_by_inum function and defs.h exports it

**Error:** `undefined reference to 'pipealloc'`
- **Solution:** pipealloc should be defined in kernel/pipe.c - verify defs.h exports it

### QEMU Errors

**Error:** `QEMU binary not found`
- **Solution:** Ensure qemu-system-riscv64 is in your PATH

**Error:** `invalid RISC-V ISA string`
- **Solution:** Update QEMU to version 7.2 or later

## Understanding the Output

### From ckptest

1. **"Checkpoint saved"**: Process successfully created checkpoint file
2. **"FDs open"**: Lists the file descriptors that were open
3. **"Read from fd2"**: Verifies file was restored with correct content
4. **"Read from pipe"**: Verifies pipe data was accessible
5. **"CWD correctly restored"**: Confirms working directory was restored

## What's Being Tested

### 1. File Descriptor Table (FD)
- **Test**: Opens multiple files, saves to checkpoint
- **Verification**: Reads from restored files, checks data intact
- **Data**: File offsets, inode numbers, readable/writable flags

### 2. File Offsets
- **Test**: Files may have been read/written at specific offsets
- **Verification**: Seeking and reading returns correct data
- **Data**: Byte position in files (uint off field)

### 3. Current Working Directory (CWD)
- **Test**: Changes to subdirectory before checkpoint
- **Verification**: Can create files in that directory after restore
- **Data**: Inode number of directory (uint cwd_inum field)

### 4. Pipes
- **Test**: Creates pipe, writes data before checkpoint
- **Verification**: Can read pipe data after restore
- **Data**: Pipe state (both read and write ends), data buffer

### 5. Process Restoration
- **Test**: Modified value before restore
- **Verification**: Restored process has original values
- **Data**: User memory, registers, program counter

## Performance Considerations

- **Checkpoint Size**: Depends on process memory size + FD table
  - Basic test: ~32 KB (32 pages) + header
  - Large process: Can be several MB
  
- **Restore Time**: Proportional to number of FDs and memory pages
  - Basic test: < 1 second
  - Large process: Several seconds

## Cleanup

After testing, you can clean up generated files:

```bash
# Exit QEMU (Ctrl+A then X)

# Clean build artifacts
make clean
```

## Next Steps

After successful testing:

1. **Examine the code**: Look at the modified checkpoint_proc and restore_proc functions
2. **Understand the design**: Read CHECKPOINT_RESTORE_IMPLEMENTATION.md
3. **Extend functionality**: Add support for more file types or devices
4. **Integration testing**: Test with more complex process scenarios

## Support

If you encounter issues:

1. Check that all files were modified correctly:
   - kernel/proc.h (ckpt_header extended)
   - kernel/proc.c (checkpoint_proc and restore_proc)
   - kernel/fs.c (iget_by_inum added)
   - kernel/defs.h (iget_by_inum exported)

2. Verify RISC-V toolchain is installed:
   ```bash
   riscv64-unknown-elf-gcc --version
   riscv64-unknown-elf-objdump --version
   ```

3. Check that QEMU is properly installed:
   ```bash
   qemu-system-riscv64 --version
   which qemu-system-riscv64
   ```
