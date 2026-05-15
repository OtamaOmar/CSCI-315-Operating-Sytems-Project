# Checkpoint/Restore Implementation - FD/CWD/Pipes/Process Relations

## Overview
This document describes the implementation of file descriptor (FD), current working directory (CWD), and pipe checkpoint/restore functionality for the xv6 operating system. The implementation allows processes to be checkpointed with their open files, directories, and pipes, and then restored to the exact state they were in.

---

## Implementation Details

### 1. Extended Data Structures (kernel/proc.h)

#### New Structure: `struct ckpt_fd`
Stores metadata for a single file descriptor during checkpoint:
```c
struct ckpt_fd {
  int type;        // FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE
  char readable;   // Read permission flag
  char writable;   // Write permission flag
  uint inum;       // Inode number (for FD_INODE)
  uint off;        // File offset (for FD_INODE)
  short major;     // Major device number (for FD_DEVICE)
};
```

#### Extended Structure: `struct ckpt_header`
Now includes FD table and CWD information:
```c
struct ckpt_header {
  uint   magic;
  int    pid;
  uint64 sz;
  struct trapframe tf;
  
  // FD/CWD info (phase 1)
  int    num_fds;              // Number of open FDs
  uint   cwd_inum;             // CWD inode number
  struct ckpt_fd fds[NOFILE];  // FD table (up to 16 entries)
};
```

---

### 2. Filesystem Functions (kernel/fs.c)

#### New Function: `iget_by_inum(uint inum)`
**Purpose:** Retrieve an inode from its inode number during restore
**Implementation:**
- Calls the internal `iget(ROOTDEV, inum)` function
- ROOTDEV is defined in param.h as device 1
- Automatically increments inode reference count
- Returns NULL if inode is not found

**Key Points:**
- Assumes single-device filesystem (standard in xv6)
- Uses ROOTDEV constant for consistency with filesystem initialization
- Exported in defs.h for use by proc.c

---

### 3. Checkpoint Function Enhancement (kernel/proc.c::checkpoint_proc)

**Modifications:**
1. **Capture FD Table**
   - Iterates through all 16 file descriptors (NOFILE = 16)
   - For each open FD, stores:
     - Type (INODE, DEVICE, PIPE, or NONE)
     - Readable/writable flags
     - For INODE files: saves inode number and current file offset
     - For DEVICE files: saves inode number and major device number
     - For PIPE files: validates both ends are in same process

2. **Pipe Validation**
   - When encountering a pipe FD, searches for the paired FD in the same process
   - If pipe has only one end in this process: **returns -1 (checkpoint fails)**
   - Only supports pipes where both read and write ends are owned by the checkpointed process
   - Prevents orphaned pipe restoration issues

3. **Capture CWD**
   - Saves the inode number of the current working directory
   - If no CWD is set, saves 0

4. **Header Writing**
   - Writes complete header with FD table and CWD info
   - Header size increased (now includes full FD table)

---

### 4. Restore Function Enhancement (kernel/proc.c::restore_proc)

**Modifications:**
1. **FD Table Restoration**
   - Reads FD table from checkpoint file header
   - For each FD entry:
     - **INODE type:**
       - Uses `iget_by_inum()` to retrieve the inode
       - Creates file structure with saved properties
       - Seeks to saved file offset
     - **DEVICE type:**
       - Recreates device file reference
       - Restores major device number
     - **PIPE type:**
       - Temporarily marks as FD_PIPE with pipe = NULL
       - Handled separately in pipe pairing phase

2. **CWD Restoration**
   - Uses `iget_by_inum()` to restore saved cwd inode
   - Fallback: if saved cwd inode not found, defaults to root (ROOTINO = 1)
   - Safely handles missing directory scenarios

3. **Pipe Pairing and Recreation**
   - After all FDs are processed, handles pipe recreation
   - Scans for unmatched pipe FD pairs
   - For each pair found:
     - Calls `pipealloc()` to create a new pipe structure
     - Restores original readable/writable flags for each end
     - Closes temporary file structures
     - Assigns new pipe to both FDs
   - Ensures proper pipe semantics for read/write operations

4. **Process Properties**
   - Sets process name to "restored" for debugging
   - Leaves parent as NULL (orphaned process)
   - Sets state to RUNNABLE for immediate scheduling
   - Returns the new process ID to caller

---

## Error Handling

### Checkpoint Errors
1. **Pipe with one end missing:** Returns -1
   - Prevents checkpoint if process only has one end of a pipe
   - Ensures consistency during restore

2. **File creation failure:** Returns -1
   - Checkpoint file creation errors propagate

3. **Memory/disk I/O failure:** Returns -1
   - Write failures in header or address space

### Restore Errors
1. **Missing inode for FD:** Falls back to NULL, cleanup occurs
2. **Missing CWD inode:** Falls back to root directory
3. **Pipe allocation failure:** Returns -1
4. **Memory allocation failure:** Returns -1

---

## Process Relations

### Parent/Child Links
- **Restored process:** Set as **orphan** (parent = NULL)
- **Rationale:** Restored process has no logical parent relationship
- **Implications:** Process will not wait on parent, and parent cannot wait on restored process

### Process Naming
- **Restored process name:** "restored" (8 bytes, null-terminated)
- **Purpose:** Easy identification for debugging
- **Can be customized:** Name field is user-modifiable after restoration

### Process State
- **Initial state:** RUNNABLE
- **Scheduling:** Process immediately eligible for scheduling
- **User execution:** Begins or resumes at saved program counter (from trapframe)

---

## Key Design Decisions

1. **Single-Device Assumption**
   - Simplifies inode number to inode resolution
   - Consistent with xv6 filesystem design
   - Uses ROOTDEV constant throughout

2. **Pipe Both-Ends-Same-Process**
   - Prevents complex cross-process pipe restoration
   - Simplifies implementation
   - Still allows checkpoint of processes with internal pipes

3. **Inode Reference Management**
   - `iget_by_inum()` increments ref count
   - Matched by automatic cleanup in process exit
   - No manual ref count manipulation needed

4. **Fallback to Root for Missing CWD**
   - Graceful degradation
   - Process can still function
   - Better than checkpoint failure

5. **FD Array Size Fixed at NOFILE**
   - Standard xv6 limit (16 FDs per process)
   - Simplifies header structure and file format
   - Matches existing process implementation

---

## Testing

### Test Program: `user/ckptest.c`
Created comprehensive test that:

1. **Creates test files:**
   - Opens file for writing
   - Opens second file for reading/writing
   - Writes data to both

2. **Creates pipes:**
   - Allocates pipe
   - Writes data to pipe

3. **Changes directory:**
   - Creates test directory
   - Changes to that directory

4. **Checkpoints:**
   - Calls `checkpoint(0, "test.ckpt")`
   - Continues with modified state

5. **Restores:**
   - Calls `restore("test.ckpt")`
   - Verifies:
     - File descriptors still open and readable
     - File offsets preserved
     - Pipe data accessible
     - Current working directory restored

---

## Files Modified

1. **kernel/proc.h**
   - Added `struct ckpt_fd`
   - Extended `struct ckpt_header`

2. **kernel/proc.c**
   - Enhanced `checkpoint_proc()` function
   - Enhanced `restore_proc()` function

3. **kernel/fs.c**
   - Added `iget_by_inum()` function

4. **kernel/defs.h**
   - Added declaration for `iget_by_inum()`

5. **Makefile**
   - Added `$U/_ckptest` to UPROGS list

6. **user/ckptest.c**
   - New test program

---

## How to Build and Run

### Build xv6 with Checkpoint Support
```bash
cd /path/to/xv6
make clean
make
make qemu
```

### Run Test in xv6
Once in xv6 shell:
```
$ ckptest
```

This will:
1. Create test files and pipes
2. Create a checkpoint
3. Modify process state
4. Restore from checkpoint
5. Verify all resources are correctly restored

---

## Limitations and Future Enhancements

### Current Limitations
1. **Single pipe per process:** Only supports one logical pipe pair per process
2. **No cross-process pipes:** Cannot checkpoint if process has pipes shared with other processes
3. **No signals:** Signal handlers not preserved
4. **No mmap:** Memory-mapped files not supported
5. **No sockets:** Socket connections not preserved

### Future Enhancements
1. **Multi-pipe support:** Handle multiple pipe pairs
2. **Cross-process recovery:** Support pipes with external readers/writers
3. **Signal preservation:** Save and restore signal handlers
4. **Memory mapping:** Support mmap'ed regions
5. **Network state:** Save connection state for restored processes

---

## Verification Checklist

- ✅ FD table captured with all metadata
- ✅ File offsets preserved and restored
- ✅ CWD inode saved and restored
- ✅ Fallback to root for missing CWD
- ✅ Pipe validation during checkpoint
- ✅ Pipe recreation during restore
- ✅ Readable/writable flags preserved
- ✅ Process orphaning (no parent)
- ✅ Process naming for debugging
- ✅ Immediate RUNNABLE state
- ✅ Error handling for missing inodes
- ✅ Error handling for pipe mismatches
- ✅ Comprehensive test program

---

## Code Quality

- **Type Safety:** All structures properly typed
- **Error Handling:** Comprehensive error checks and fallbacks
- **Comments:** Clear annotations for M4 phase
- **Consistency:** Follows xv6 coding conventions
- **Testing:** Automated test program provided

---

