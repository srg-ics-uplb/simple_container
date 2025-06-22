# Simple Container Implementation (Generated with Claude AI)

A lightweight container implementation for Linux that demonstrates the fundamental concepts behind containerization using system calls like `chroot`, `clone`, and Linux namespaces.

## Overview

This project implements a basic container runtime that provides:
- Process isolation using PID namespaces
- Filesystem isolation using mount namespaces and chroot
- Hostname isolation using UTS namespaces
- IPC isolation using IPC namespaces
- Basic filesystem setup (proc, sys, tmp)

## Requirements

- Linux system (kernel 3.8+ recommended for full namespace support)
- GCC compiler
- Root privileges to run containers
- Basic development tools (make)

## Building the Project

### 1. Clone or Download Files

Ensure you have the following files:
- `simple_container.c` - Main container implementation
- `Makefile` - Build configuration
- `copy_libs.sh` - Script to copy shared libraries

### 2. Compile the Container Runtime

```bash
make
```

This will create the `simple_container` executable.

### 3. Optional: Install System-wide

```bash
make install
```

This installs the binary to `/usr/local/bin/` (requires sudo).

## Creating a Root Filesystem

Before running containers, you need a root filesystem. You can create a minimal one using the provided script:

### Automated Creation

```bash
# Make the script executable
chmod +x copy_libs.sh

# Create a minimal rootfs
make SHELL=/bin/bash create-rootfs
```

This creates a `rootfs` directory with essential binaries and their dependencies.

### Manual Creation

If you prefer to create a custom rootfs:

```bash
mkdir -p my_rootfs/{bin,sbin,etc,proc,sys,tmp,usr/bin,lib,lib64}

# Copy essential binaries
cp /bin/bash my_rootfs/bin/
cp /bin/ls my_rootfs/bin/
cp /bin/cat my_rootfs/bin/

# Copy required libraries (use ldd to find dependencies)
# For example: ldd /bin/bash
cp /lib/x86_64-linux-gnu/libc.so.6 my_rootfs/lib/
# ... copy other required libraries
```

## Usage

### Basic Syntax

```bash
sudo ./simple_container [OPTIONS] <rootfs_path> <command> [args...]
```

### Options

- `-h, --hostname <hostname>`: Set container hostname
- `--help`: Show help message

### Examples

#### 1. Run a Shell in Container

```bash
sudo ./simple_container ./rootfs /bin/bash
```

#### 2. Run a Command with Custom Hostname

```bash
sudo ./simple_container -h mycontainer ./rootfs /bin/sh -c "hostname && echo 'Hello from container'"
```

#### 3. List Files in Container

```bash
sudo ./simple_container ./rootfs /bin/ls -la /
```

#### 4. Check Process Isolation

```bash
# In one terminal
sudo ./simple_container ./rootfs /bin/bash

# Inside the container, check the PID
echo $$  # Should show PID 1

# Check process list
ps aux  # Should only show processes in container
```

## How It Works

### 1. Namespace Creation

The container uses Linux namespaces for isolation:

- **PID Namespace (`CLONE_NEWPID`)**: Container processes see their own PID space starting from 1
- **Mount Namespace (`CLONE_NEWNS`)**: Container has its own mount table
- **UTS Namespace (`CLONE_NEWUTS`)**: Container can have its own hostname
- **IPC Namespace (`CLONE_NEWIPC`)**: Container has isolated inter-process communication

### 2. Filesystem Isolation

- Uses `chroot()` to change the root directory to the specified rootfs
- Mounts essential filesystems:
  - `/proc` - Process information
  - `/sys` - System information
  - `/tmp` - Temporary files (tmpfs)

### 3. Process Creation

- Uses `clone()` system call to create a child process with new namespaces
- Parent waits for child completion and reports exit status

## Testing the Container

### Verify Isolation

1. **PID Isolation**:
```bash
sudo ./simple_container ./rootfs /bin/sh -c "echo 'Container PID: $$'"
```

2. **Hostname Isolation**:
```bash
sudo ./simple_container -h testhost ./rootfs /bin/sh -c "hostname"
```

3. **Filesystem Isolation**:
```bash
sudo ./simple_container ./rootfs /bin/ls /
# Should only show contents of rootfs, not host filesystem
```

### Common Issues and Solutions

#### "Operation not permitted" errors
- Ensure you're running as root: `sudo ./simple_container ...`
- Check that your kernel supports the required namespaces

#### "No such file or directory" when executing commands
- Verify the binary exists in rootfs: `ls -la rootfs/bin/`
- Check that required libraries are present: `ldd rootfs/bin/bash`
- Use the `copy_libs.sh` script to copy dependencies

#### Mount errors
- Ensure proc, sys, tmp directories exist in rootfs
- Check that you have sufficient privileges

## Cleaning Up

### Remove Build Artifacts

```bash
make clean
```

### Remove Test Rootfs

```bash
make clean-rootfs
```

### Uninstall System Binary

```bash
make uninstall
```

## Limitations

This is a basic implementation for educational purposes. It lacks many features of production container runtimes:

- No network namespace isolation
- No resource limits (cgroups)
- No security features (capabilities, seccomp)
- No image management
- No advanced mount options
- No user namespace support

## Security Considerations

- **Root Required**: This container requires root privileges
- **No Security Boundaries**: Don't use for isolation of untrusted code
- **Educational Purpose**: Not suitable for production environments

## Extending the Container

You can extend this implementation by adding:

1. **Network Namespaces**: Add `CLONE_NEWNET` flag
2. **User Namespaces**: Add `CLONE_NEWUSER` flag
3. **Cgroups**: Implement resource limiting
4. **Security**: Add capability dropping, seccomp filters
5. **Advanced Mounts**: Support bind mounts, overlayfs

## Troubleshooting

### Debugging

Add debug output by modifying the source code:

```c
printf("Debug: About to chroot to %s\n", config->rootfs_path);
```

### Checking Dependencies

```bash
# Check what libraries a binary needs
ldd rootfs/bin/bash

# Verify libraries exist in rootfs
ls -la rootfs/lib/
```

### Kernel Feature Support

```bash
# Check namespace support
ls /proc/*/ns/

# Check available namespaces
cat /proc/version
```

## References

- Linux namespaces documentation: `man 7 namespaces`
- chroot system call: `man 2 chroot`
- clone system call: `man 2 clone`
- mount system call: `man 2 mount`
